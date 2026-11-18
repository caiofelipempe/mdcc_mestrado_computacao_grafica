// winged_edge.hpp — B-rep winged-edge com operadores de Euler e de adjacência (C++20)
//
// Convenção: cada aresta e vai de v[0] a v[1]. O lado s=0 é a face à ESQUERDA (percorrida
// v0->v1), o lado s=1 é a face à DIREITA (percorrida v1->v0). Em ambos os lados a face é
// percorrida em sentido anti-horário visto de fora. As 4 "asas" são next[s]/prev[s]: a aresta
// seguinte/anterior no ciclo da face f[s].
//
// Dart = (aresta, lado) é só um auxiliar interno para saber em que sentido uma aresta está
// sendo percorrida; a estrutura armazenada continua sendo a winged-edge clássica.
#pragma once

#include "mesh.hpp"
#include "rotator3.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <compare>
#include <numbers>
#include <optional>
#include <stdexcept>
#include <vector>

namespace geometry::winged
{
    template <class Tag> struct Id // índice tipado: VId, EId e FId não se misturam
    {
        int i = -1;
        constexpr bool valid() const { return i >= 0; }
        auto operator<=>(const Id &) const = default;
    };
    using VId = Id<struct VTag>;
    using EId = Id<struct ETag>;
    using FId = Id<struct FTag>;

    struct Vertex { Vec3f p; EId e; }; // e: uma aresta incidente (invalid se isolado)
    struct Edge
    {
        std::array<VId, 2> v;          // v[0] -> v[1]
        std::array<FId, 2> f;          // f[0] esquerda, f[1] direita
        std::array<EId, 2> next, prev; // asas, por lado
        bool alive = true;
    };

    struct Dart
    {
        EId e;
        int s;
        bool operator==(const Dart &) const = default;
    };
    struct Face { std::vector<Dart> loops; };

    struct Corner { FId f; VId v; EId via{}; };

    struct NewVF { VId v; FId f; };
    struct NewEV { EId e; VId v; };
    struct NewEF { EId e; FId f; };

    class Solid
    {
    public:
        std::vector<Vertex> V;
        std::vector<Edge> E;
        std::vector<Face> F;

        Solid() = default;

        // ───────────────────────── operadores de Euler ─────────────────────────

        // Make Vertex Face Solid: cria o sólido mínimo (1 vértice, 1 face sem arestas).
        NewVF MVFS(const Vec3f &p)
        {
            if (!V.empty()) throw std::logic_error("MVFS: sólido já iniciado");
            return {addVertex(p), addFace()};
        }

        // Make Edge Vertex: cria um novo vértice em p ligado ao vértice do canto por uma aresta pendente.
        NewEV MEV(Corner c, const Vec3f &p)
        {
            VId w = addVertex(p);
            EId e = addEdge(c.v, w, c.f, c.f);
            Dart a{e, 0}, b{e, 1}; // a: c.v -> w, b: w -> c.v
            if (!at(c.v).e.valid()) // vértice isolado (vindo do MVFS)
            {
                link(a, b);
                link(b, a);
                at(c.f).loops.push_back(a);
                at(c.v).e = e;
            }
            else
            {
                auto hit = find(c);
                if (!hit) throw std::invalid_argument("MEV: canto inexistente");
                Dart in = hit->in, out = next(in);
                link(in, a);
                link(a, b);
                link(b, out);
            }
            at(w).e = e;
            return {e, w};
        }

        // Make Edge Face: liga os vértices dos dois cantos (mesmo ciclo de uma mesma face) e divide
        // a face em duas. A nova face recebe o trecho out1..in2; a original fica com o resto
        // (inclusive os anéis).
        NewEF MEF(Corner c1, Corner c2)
        {
            if (c1.f != c2.f || c1.v == c2.v) throw std::invalid_argument("MEF: cantos inválidos");
            auto h1 = find(c1), h2 = find(c2);
            if (!h1 || !h2 || h1->loop != h2->loop) throw std::invalid_argument("MEF: cantos fora do mesmo ciclo");
            Dart in1 = h1->in, out1 = next(in1), in2 = h2->in, out2 = next(in2);

            FId f = c1.f, g = addFace();
            EId e = addEdge(c1.v, c2.v, f, g);
            Dart a{e, 0}, b{e, 1}; // a: v1 -> v2 (face f), b: v2 -> v1 (face g)
            link(in2, b);
            link(b, out1); // ciclo novo:   out1 ... in2, b
            link(in1, a);
            link(a, out2); // ciclo antigo: out2 ... in1, a
            for (Dart d : loop(b)) at(d.e).f[d.s] = g;
            at(f).loops[h1->loop] = a;
            at(g).loops = {b};
            return {e, g};
        }

        // Kill Edge Make Ring: remove uma ponte (aresta com a mesma face dos dois lados), o que parte
        // o ciclo em dois. O ciclo que passa por ringVertex vira anel; o outro mantém a posição.
        void KEMR(EId e, VId ringVertex)
        {
            Edge &ed = at(e);
            if (ringVertex != ed.v[0] && ringVertex != ed.v[1]) throw std::invalid_argument("KEMR: vértice não pertence à aresta");
            Dart a{e, 0}, b{e, 1};
            FId f = ed.f[0];
            int l = loopIndex(f, a);
            if (l < 0 || loopIndex(f, b) != l) throw std::invalid_argument("KEMR: a aresta não é ponte");
            Dart pa = prev(a), na = next(a), pb = prev(b), nb = next(b);
            if (na == b || nb == a) throw std::invalid_argument("KEMR: aresta pendente (caso do KEV)");

            link(pb, na); // ciclo em torno de v[1]
            link(pa, nb); // ciclo em torno de v[0]
            at(ed.v[0]).e = pa.e;
            at(ed.v[1]).e = pb.e;
            ed.alive = false;

            bool ringAtEnd = ringVertex == ed.v[1];
            auto &loops = at(f).loops;
            loops[l] = ringAtEnd ? nb : na;
            loops.push_back(ringAtEnd ? na : nb);
        }

        // ──────────────── operações de construção (compostas por MVFS/MEV/MEF) ────────────────

        struct Lamina { FId front, back; };

        // Lâmina: polígono plano com duas faces (MVFS + cadeia de MEVs + um MEF que fecha o ciclo).
        // `front` percorre pts na ordem dada (normal pela regra da mão direita); `back`, ao contrário.
        Lamina lamina(const std::vector<Vec3f> &pts)
        {
            if (pts.size() < 3) throw std::invalid_argument("lamina: são necessários >= 3 pontos");
            auto [v0, back] = MVFS(pts[0]);
            std::vector<VId> vs{v0};
            for (std::size_t i = 1; i < pts.size(); ++i)
                vs.push_back(MEV({back, vs[i - 1]}, pts[i]).v);
            const FId front = MEF({back, vs.front()}, {back, vs.back()}).f;
            return {front, back};
        }

        // Varredura: de cada vértice do contorno de f ergue uma aresta (MEV) até map(posição) e fecha
        // cada lado com uma face quadrilateral (MEF). f vira uma das faces laterais; a face devolvida
        // é a nova tampa, percorrida no mesmo sentido que f.
        template <class Map>
        FId sweep(FId f, Map map)
        {
            if (at(f).loops.size() != 1) throw std::invalid_argument("sweep: a face deve ter um único ciclo");
            const std::vector<VId> q = FV(f);
            const std::size_t n = q.size();
            std::vector<VId> t;
            for (VId v : q)
            {
                const Vec3f p = map(at(v).p);
                t.push_back(MEV({f, v}, p).v);
            }
            for (std::size_t j = 0; j + 1 < n; ++j)
                MEF({f, t[j]}, {f, t[j + 1]});
            return MEF({f, t[0]}, {f, t[n - 1]}).f;
        }

        // Extrusão (varredura translacional) de f ao longo de d.
        FId extrude(FId f, const Vec3f &d)
        {
            return sweep(f, [&](const Vec3f &p) { return p + d; });
        }

        // Cone: liga todos os vértices do contorno de f a um vértice novo em apex (MEV + MEFs),
        // substituindo f por um leque de triângulos. Devolve o vértice do ápice.
        VId cone(FId f, const Vec3f &apex)
        {
            if (at(f).loops.size() != 1) throw std::invalid_argument("cone: a face deve ter um único ciclo");
            const std::vector<VId> p = FV(f);
            const VId a = MEV({f, p[0]}, apex).v;
            FId big = f;
            for (std::size_t j = p.size() - 1; j >= 1; --j)
                big = MEF({big, a}, {big, p[j]}).f;
            return a;
        }

        // Aplica rotação e translação a todos os vértices (a topologia não muda).
        void transform(const Vec3f &origin, const Rot3f &rotator)
        {
            for (Vertex &v : V)
                v.p = origin + rotator.rotateVector(v.p);
        }

        // ───────────────────── primitivas (centradas na origem, eixo y) ─────────────────────

        // Bloco: lâmina retangular na base, extrudada até o topo.
        static Solid block(float w, float h, float d)
        {
            const float hx = w / 2, hy = h / 2, hz = d / 2;
            Solid s;
            auto [bottom, top] = s.lamina({{-hx, -hy, -hz}, {hx, -hy, -hz}, {hx, -hy, hz}, {-hx, -hy, hz}});
            s.extrude(top, {0.f, h, 0.f});
            return s;
        }

        // Cilindro: lâmina circular na base, extrudada até o topo.
        static Solid cylinder(float r, float h, std::size_t n)
        {
            Solid s;
            auto [bottom, top] = s.lamina(circle(r, -h / 2, n));
            s.extrude(top, {0.f, h, 0.f});
            return s;
        }

        // Cone: lâmina circular na base e cone até o ápice.
        static Solid cone(float r, float h, std::size_t n)
        {
            Solid s;
            auto [bottom, top] = s.lamina(circle(r, -h / 2, n));
            s.cone(top, {0.f, h / 2, 0.f});
            return s;
        }

        // Esfera: sólido de revolução com stacks - 1 paralelos.
        static Solid sphere(float r, std::size_t n)
        {
            constexpr float pi = std::numbers::pi_v<float>;
            const std::size_t stacks = std::max<std::size_t>(2, n / 2);
            std::vector<Parallel> ps;
            for (std::size_t k = 1; k < stacks; ++k)
                ps.push_back(latitude(r, -pi / 2 + pi * float(k) / float(stacks), 0.f));
            return revolution(ps, -r, r, n);
        }

        // ──────────────────────── operadores de adjacência ─────────────────────
        // Resultados sem repetição, na ordem de percurso.

        std::array<VId, 2> EV(EId e) const { return at(e).v; }
        std::array<FId, 2> EF(EId e) const { return at(e).f; } // {esquerda, direita}
        std::vector<EId> EE(EId e) const // arestas que compartilham vértice com e
        {
            auto ds = around(at(e).v[0]), d1 = around(at(e).v[1]);
            ds.insert(ds.end(), d1.begin(), d1.end());
            auto r = gather<EId>(ds, [](Dart d) { return d.e; });
            std::erase(r, e);
            return r;
        }

        std::vector<EId> VE(VId v) const { return gather<EId>(around(v), [](Dart d) { return d.e; }); }
        std::vector<VId> VV(VId v) const { return gather<VId>(around(v), [&](Dart d) { return end(d); }); }
        std::vector<FId> VF(VId v) const { return gather<FId>(around(v), [&](Dart d) { return face(d); }); }

        std::vector<EId> FE(FId f) const { return gather<EId>(boundary(f), [](Dart d) { return d.e; }); }
        std::vector<VId> FV(FId f) const { return gather<VId>(boundary(f), [&](Dart d) { return start(d); }); }
        std::vector<FId> FF(FId f) const
        {
            auto r = gather<FId>(boundary(f), [&](Dart d) { return face(twin(d)); });
            std::erase(r, f);
            return r;
        }

        // ───────────────────────────── conversão ───────────────────────────────

        // Vértices e arestas vivas são copiados como estão (os índices dos vértices coincidem com
        // os VId). Cada face vira um leque de triângulos a partir do primeiro vértice do contorno
        // (correto para faces convexas); lightNormal recebe a normal unitária do triângulo.
        // Faces com anéis não são trianguladas: lança std::logic_error.
        [[nodiscard]] Mesh3f toMesh() const
        {
            Mesh3f m;
            for (const Vertex &v : V)
                m.vertices().push_back(v.p);

            for (const Edge &e : E)
                if (e.alive)
                    m.addEdge(std::size_t(e.v[0].i), std::size_t(e.v[1].i));

            for (const Face &f : F)
            {
                if (f.loops.size() != 1) throw std::logic_error("toMesh: face com anéis ou sem arestas");
                const auto ds = loop(f.loops[0]);
                for (std::size_t j = 1; j + 1 < ds.size(); ++j)
                {
                    const auto a = std::size_t(start(ds[0]).i), b = std::size_t(start(ds[j]).i), c = std::size_t(start(ds[j + 1]).i);
                    m.addFace(a, b, c);
                    Vec3f n = (V[b].p - V[a].p).cross(V[c].p - V[a].p);
                    if (n.sqrNorm() > 0.f) n = n.normalized();
                    m.faces().back().lightNormal = n;
                }
            }
            return m;
        }

        // ───────────────────────────── verificação ─────────────────────────────

        // V - E + F - R; vale 2 para um sólido fechado de gênero 0.
        int euler() const
        {
            int ne = int(std::ranges::count_if(E, [](const Edge &e) { return e.alive; }));
            int rings = 0;
            for (const Face &f : F)
                rings += f.loops.empty() ? 0 : int(f.loops.size()) - 1;
            return int(V.size()) - ne + int(F.size()) - rings;
        }

        // Consistência das asas: prev(next(d)) == d, mesma face e vértices encadeados.
        bool consistent() const
        {
            for (int i = 0; i < int(E.size()); ++i)
            {
                if (!E[i].alive) continue;
                for (int s = 0; s < 2; ++s)
                {
                    Dart d{EId{i}, s}, n = next(d);
                    if (!(prev(n) == d) || face(n) != face(d) || end(d) != start(n)) return false;
                }
            }
            return true;
        }

    private:
        struct Found { int loop; Dart in; };

        struct Parallel { float r, y; }; // círculo horizontal de raio r na altura y

        static std::vector<Vec3f> circle(float r, float y, std::size_t n)
        {
            if (n < 3) throw std::invalid_argument("Solid: segments deve ser >= 3");
            constexpr float pi = std::numbers::pi_v<float>;
            std::vector<Vec3f> ring;
            for (std::size_t i = 0; i < n; ++i)
            {
                const float t = 2 * pi * float(i) / float(n);
                ring.push_back({r * std::cos(t), y, r * std::sin(t)});
            }
            return ring;
        }

        // paralelo na latitude phi de uma esfera de raio R centrada em y = yc
        static Parallel latitude(float R, float phi, float yc) { return {R * std::cos(phi), yc + R * std::sin(phi)}; }

        // Sólido de revolução em torno do eixo y: lâmina do primeiro paralelo, cone até o polo sul,
        // uma varredura (escala radial + translação) para cada paralelo seguinte e cone até o polo
        // norte.
        static Solid revolution(const std::vector<Parallel> &ps, float yBottom, float yTop, std::size_t n)
        {
            Solid s;
            auto [bottom, top] = s.lamina(circle(ps[0].r, ps[0].y, n));
            s.cone(bottom, {0.f, yBottom, 0.f});
            for (std::size_t k = 1; k < ps.size(); ++k)
            {
                const float scale = ps[k].r / ps[k - 1].r, y = ps[k].y;
                top = s.sweep(top, [=](const Vec3f &p) { return Vec3f{p[0] * scale, y, p[2] * scale}; });
            }
            s.cone(top, {0.f, yTop, 0.f});
            return s;
        }
        VId addVertex(const Vec3f &p) { V.push_back({p, {}}); return {int(V.size()) - 1}; }
        FId addFace() { F.emplace_back(); return {int(F.size()) - 1}; }
        EId addEdge(VId a, VId b, FId l, FId r) { E.push_back({.v = {a, b}, .f = {l, r}, .next = {}, .prev = {}}); return {int(E.size()) - 1}; }

        Vertex &at(VId id) { return V[id.i]; }
        Edge &at(EId id) { return E[id.i]; }
        Face &at(FId id) { return F[id.i]; }
        const Vertex &at(VId id) const { return V[id.i]; }
        const Edge &at(EId id) const { return E[id.i]; }
        const Face &at(FId id) const { return F[id.i]; }

        VId start(Dart d) const { return at(d.e).v[d.s]; }
        VId end(Dart d) const { return at(d.e).v[1 - d.s]; }
        FId face(Dart d) const { return at(d.e).f[d.s]; }
        Dart twin(Dart d) const { return {d.e, 1 - d.s}; }
        Dart next(Dart d) const // o lado do vizinho é o que começa onde d termina
        {
            EId n = at(d.e).next[d.s];
            return {n, at(n).v[0] == end(d) ? 0 : 1};
        }
        Dart prev(Dart d) const
        {
            EId p = at(d.e).prev[d.s];
            return {p, at(p).v[1] == start(d) ? 0 : 1};
        }
        void link(Dart a, Dart b) { at(a.e).next[a.s] = b.e; at(b.e).prev[b.s] = a.e; } // end(a) == start(b)

        std::vector<Dart> loop(Dart d0) const
        {
            std::vector<Dart> r;
            Dart d = d0;
            do { r.push_back(d); d = next(d); } while (!(d == d0));
            return r;
        }
        std::vector<Dart> boundary(FId f) const
        {
            std::vector<Dart> r;
            for (Dart l : at(f).loops)
                for (Dart d : loop(l))
                    r.push_back(d);
            return r;
        }
        // darts que saem de v, em ordem anti-horária
        std::vector<Dart> around(VId v) const
        {
            std::vector<Dart> r;
            EId e0 = at(v).e;
            if (!e0.valid()) return r;
            Dart d0{e0, at(e0).v[0] == v ? 0 : 1}, d = d0;
            do { r.push_back(d); d = twin(prev(d)); } while (!(d == d0));
            return r;
        }
        int loopIndex(FId f, Dart d) const
        {
            const auto &ls = at(f).loops;
            for (int i = 0; i < int(ls.size()); ++i)
            {
                auto L = loop(ls[i]);
                if (std::ranges::find(L, d) != L.end()) return i;
            }
            return -1;
        }
        std::optional<Found> find(Corner c) const
        {
            const auto &ls = at(c.f).loops;
            for (int i = 0; i < int(ls.size()); ++i)
                for (Dart d : loop(ls[i]))
                    if (end(d) == c.v && (!c.via.valid() || d.e == c.via)) return Found{i, d};
            return std::nullopt;
        }
        template <class T, class Fn> static std::vector<T> gather(const std::vector<Dart> &ds, Fn fn)
        {
            std::vector<T> r;
            for (Dart d : ds)
                if (T x = fn(d); std::ranges::find(r, x) == r.end()) r.push_back(x);
            return r;
        }
    };

} // namespace geometry::winged