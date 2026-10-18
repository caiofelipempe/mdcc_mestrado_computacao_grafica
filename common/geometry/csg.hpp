#pragma once

#include "rotator3.hpp"
#include "result.hpp"
#include "shape.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cmath>
#include <concepts>
#include <limits>
#include <memory>
#include <numbers>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace geometry
{
    class CsgNode;
    using CsgPtr = std::unique_ptr<CsgNode>;

    enum class CsgOp
    {
        Union,
        Intersection,
        Difference
    };

    // Nó folha: shape posicionado em `origin` e orientado por `rotator`.
    struct CsgPrimitive
    {
        std::unique_ptr<Shape> shape;
        Vec3f origin;
        Rot3f rotator;
    };

    // Nó interno: combina os dois filhos. Difference = left - right.
    struct CsgOperation
    {
        CsgOp op;
        CsgPtr left;
        CsgPtr right;
    };

    // Resultado do raio: primeira entrada no sólido e a saída correspondente.
    // As normais são unitárias, externas ao sólido, em coordenadas de mundo.
    // `normalEnter` é zero se a origem do raio já está dentro do sólido.
    struct RayHit
    {
        float tEnter;
        float tExit;
        Vec3f normalEnter;
        Vec3f normalExit;
    };

    namespace detail
    {
        inline const Vec3f kZero{0.0f, 0.0f, 0.0f};

        inline Vec3f safeNormalize(const Vec3f &v)
        {
            return v.sqrNorm() < 1e-20f ? kZero : v.normalized();
        }

        // Teste de pertinência no espaço local do shape (centrado na origem,
        // eixo Y como eixo de revolução).
        inline bool inside(const Shape &shape, const Vec3f &p)
        {
            using std::abs;

            // Cube herda de Block, então também cai aqui.
            if (dynamic_cast<const Block *>(&shape))
            {
                const Vec3f h = shape.boundSize() * 0.5f;
                return abs(p[0]) <= h[0] && abs(p[1]) <= h[1] && abs(p[2]) <= h[2];
            }

            if (const auto *s = dynamic_cast<const Sphere *>(&shape))
                return p.sqrNorm() <= s->radius() * s->radius();

            if (const auto *c = dynamic_cast<const Cylinder *>(&shape))
            {
                return p[0] * p[0] + p[2] * p[2] <= c->radius() * c->radius() &&
                       abs(p[1]) <= c->height() * 0.5f;
            }

            // Base em y = -h/2, ponta em y = +h/2.
            if (const auto *c = dynamic_cast<const Cone *>(&shape))
            {
                const float half = c->height() * 0.5f;
                if (abs(p[1]) > half)
                    return false;

                const float k = c->radius() * (half - p[1]); // raio local * altura
                return (p[0] * p[0] + p[2] * p[2]) * c->height() * c->height() <= k * k;
            }

            if (const auto *c = dynamic_cast<const Capsule *>(&shape))
            {
                const float half = c->height() * 0.5f;
                const float dy = p[1] - std::clamp(p[1], -half, half);
                return p[0] * p[0] + dy * dy + p[2] * p[2] <= c->radius() * c->radius();
            }

            // Vértices (s,s,s), (-s,-s,s), (-s,s,-s), (s,-s,-s): cada face fica
            // no plano n . p = -s, com n o sinal do vértice oposto.
            if (dynamic_cast<const RegularTetrahedron *>(&shape))
            {
                const float s = shape.boundSize()[0] * 0.5f;
                constexpr float n[4][3] = {{1, 1, 1}, {-1, -1, 1}, {-1, 1, -1}, {1, -1, -1}};

                return std::ranges::all_of(n, [&](const auto &f)
                                           { return f[0] * p[0] + f[1] * p[1] + f[2] * p[2] >= -s; });
            }

            throw std::logic_error("Csg: shape sem teste de pertinência");
        }

        /* ---- Raytest: intervalos de t em que o raio está dentro do sólido ---- */

        // [t0, t1] dentro do sólido, com as normais externas do sólido em cada
        // ponta (n0 na entrada, n1 na saída). Normal zero onde t é infinito.
        struct Interval
        {
            float t0, t1;
            Vec3f n0, n1;
        };

        using Intervals = std::vector<Interval>; // ordenados e disjuntos

        inline constexpr float kInf = std::numeric_limits<float>::infinity();

        // Conjunto { t : A t² + B t + C <= 0 } = { t : f(o + t d) <= 0 }.
        // `grad(t)` devolve o gradiente espacial de f no ponto o + t d, que
        // aponta para fora do sólido; ele define a normal em cada fronteira.
        template <class Grad>
        Intervals quadraticLE0(float A, float B, float C, Grad grad)
        {
            constexpr float eps = 1e-12f;

            auto normalAt = [&](float t)
            { return std::isfinite(t) ? safeNormalize(grad(t)) : kZero; };
            auto make = [&](float a, float b)
            { return Interval{a, b, normalAt(a), normalAt(b)}; };

            if (std::abs(A) < eps) // linear
            {
                if (std::abs(B) < eps)
                    return C <= 0.0f ? Intervals{make(-kInf, kInf)} : Intervals{};

                const float t = -C / B;
                return B > 0.0f ? Intervals{make(-kInf, t)} : Intervals{make(t, kInf)};
            }

            const float disc = B * B - 4.0f * A * C;
            if (disc < 0.0f)
                return A > 0.0f ? Intervals{} : Intervals{make(-kInf, kInf)};

            const float sq = std::sqrt(disc);
            float t0 = (-B - sq) / (2.0f * A);
            float t1 = (-B + sq) / (2.0f * A);
            if (t0 > t1)
                std::swap(t0, t1);

            return A > 0.0f ? Intervals{make(t0, t1)}
                            : Intervals{make(-kInf, t0), make(t1, kInf)};
        }

        // Semiespaço { p : n . p <= c }; a normal externa é n.
        inline Intervals halfSpace(const Vec3f &o, const Vec3f &d, const Vec3f &n, float c)
        {
            return quadraticLE0(0.0f, n.dot(d), n.dot(o) - c, [&](float)
                                { return n; });
        }

        inline Intervals intersect(const Intervals &a, const Intervals &b)
        {
            Intervals out;

            for (std::size_t i = 0, j = 0; i < a.size() && j < b.size();)
            {
                const Interval &x = a[i];
                const Interval &y = b[j];

                Interval r{};
                if (x.t0 >= y.t0) { r.t0 = x.t0; r.n0 = x.n0; }
                else              { r.t0 = y.t0; r.n0 = y.n0; }

                if (x.t1 <= y.t1) { r.t1 = x.t1; r.n1 = x.n1; }
                else              { r.t1 = y.t1; r.n1 = y.n1; }

                if (r.t0 < r.t1)
                    out.push_back(r);

                (x.t1 < y.t1 ? i : j)++;
            }

            return out;
        }

        inline Intervals unite(const Intervals &a, const Intervals &b)
        {
            Intervals all = a;
            all.insert(all.end(), b.begin(), b.end());
            std::ranges::sort(all, [](const Interval &x, const Interval &y)
                              { return x.t0 < y.t0; });

            Intervals out;
            for (const auto &iv : all)
            {
                if (out.empty() || iv.t0 > out.back().t1)
                    out.push_back(iv);
                else if (iv.t1 > out.back().t1)
                {
                    out.back().t1 = iv.t1;
                    out.back().n1 = iv.n1;
                }
            }

            return out;
        }

        // A - B. Onde a fronteira vem de B, a normal do resultado é -n de B.
        inline Intervals subtract(const Intervals &a, const Intervals &b)
        {
            Intervals out;

            for (Interval cur : a)
            {
                for (const Interval &cut : b)
                {
                    if (cut.t1 <= cur.t0 || cut.t0 >= cur.t1)
                        continue;

                    if (cut.t0 > cur.t0)
                        out.push_back({cur.t0, cut.t0, cur.n0, cut.n0 * -1.0f});

                    cur.t0 = cut.t1;
                    cur.n0 = cut.n1 * -1.0f;
                }

                if (cur.t0 < cur.t1)
                    out.push_back(cur);
            }

            return out;
        }

        // |y| <= half
        inline Intervals slabY(const Vec3f &o, const Vec3f &d, float half)
        {
            return intersect(halfSpace(o, d, Vec3f{0.0f, 1.0f, 0.0f}, half),
                             halfSpace(o, d, Vec3f{0.0f, -1.0f, 0.0f}, half));
        }

        // Esfera de raio r centrada em (0, cy, 0).
        inline Intervals sphereAt(const Vec3f &o, const Vec3f &d, float cy, float r)
        {
            const Vec3f c{0.0f, cy, 0.0f};
            const Vec3f oc = o - c;
            return quadraticLE0(d.dot(d), 2.0f * oc.dot(d), oc.dot(oc) - r * r,
                                [&](float t)
                                { return o + d * t - c; });
        }

        // Cilindro infinito em torno do eixo Y: x² + z² <= r².
        inline Intervals lateralCylinder(const Vec3f &o, const Vec3f &d, float r)
        {
            return quadraticLE0(d[0] * d[0] + d[2] * d[2],
                                2.0f * (o[0] * d[0] + o[2] * d[2]),
                                o[0] * o[0] + o[2] * o[2] - r * r,
                                [&](float t)
                                {
                                    const Vec3f p = o + d * t;
                                    return Vec3f{p[0], 0.0f, p[2]};
                                });
        }

        // Interseção do raio (no espaço local do shape) com o sólido. Todos os
        // shapes são convexos, mas a capsule e o cone são montados por união e
        // recorte, então o resultado é sempre uma lista.
        inline Intervals shapeIntervals(const Shape &shape, const Vec3f &o, const Vec3f &d)
        {
            if (dynamic_cast<const Block *>(&shape))
            {
                const Vec3f h = shape.boundSize() * 0.5f;
                Intervals r{{-kInf, kInf, kZero, kZero}};

                for (std::size_t i = 0; i < 3; ++i)
                {
                    Vec3f axis{0.0f, 0.0f, 0.0f};
                    axis[i] = 1.0f;

                    r = intersect(r, halfSpace(o, d, axis, h[i]));
                    r = intersect(r, halfSpace(o, d, axis * -1.0f, h[i]));
                }

                return r;
            }

            if (const auto *s = dynamic_cast<const Sphere *>(&shape))
                return sphereAt(o, d, 0.0f, s->radius());

            if (const auto *c = dynamic_cast<const Cylinder *>(&shape))
                return intersect(slabY(o, d, c->height() * 0.5f),
                                 lateralCylinder(o, d, c->radius()));

            // Raio local m (half - y), com m = r / h; o recorte em |y| <= half
            // descarta a outra folha do cone duplo.
            if (const auto *c = dynamic_cast<const Cone *>(&shape))
            {
                const float half = c->height() * 0.5f;
                const float m2 = (c->radius() / c->height()) * (c->radius() / c->height());
                const float h0 = half - o[1];

                const auto lateral = quadraticLE0(
                    d[0] * d[0] + d[2] * d[2] - m2 * d[1] * d[1],
                    2.0f * (o[0] * d[0] + o[2] * d[2] + m2 * h0 * d[1]),
                    o[0] * o[0] + o[2] * o[2] - m2 * h0 * h0,
                    [&](float t)
                    {
                        // gradiente de x² + z² - m2 (half - y)²  (dividido por 2)
                        const Vec3f p = o + d * t;
                        return Vec3f{p[0], m2 * (half - p[1]), p[2]};
                    });

                return intersect(slabY(o, d, half), lateral);
            }

            // Cilindro entre as duas esferas das pontas.
            if (const auto *c = dynamic_cast<const Capsule *>(&shape))
            {
                const float r = c->radius();
                const float half = c->height() * 0.5f;

                return unite(unite(intersect(slabY(o, d, half), lateralCylinder(o, d, r)),
                                   sphereAt(o, d, -half, r)),
                             sphereAt(o, d, half, r));
            }

            // Mesmos quatro semiespaços de inside(): n . p >= -s, ou seja,
            // (-n) . p <= s, com normal externa -n.
            if (dynamic_cast<const RegularTetrahedron *>(&shape))
            {
                const float s = shape.boundSize()[0] * 0.5f;
                constexpr float n[4][3] = {{1, 1, 1}, {-1, -1, 1}, {-1, 1, -1}, {1, -1, -1}};
                Intervals r{{-kInf, kInf, kZero, kZero}};

                for (const auto &f : n)
                    r = intersect(r, halfSpace(o, d, Vec3f{-f[0], -f[1], -f[2]}, s));

                return r;
            }

            throw std::logic_error("Csg: shape sem interseção com raio");
        }

        /* ---- Texto: serialização ---- */

        inline void appendFloats(std::string &out, std::span<const float> values)
        {
            char buf[32];
            bool first = true;

            for (const float v : values)
            {
                if (!first)
                    out += ',';
                first = false;

                // Representação mais curta que faz round-trip (inteiros saem sem ".0").
                out.append(buf, std::to_chars(buf, buf + sizeof buf, v).ptr);
            }
        }

        // Tag(Ps(x,y,z),Tr(x,y,z,w),args...)
        inline std::string primitiveToString(const CsgPrimitive &p)
        {
            const Shape &s = *p.shape;
            const Vec3f b = s.boundSize();
            std::string_view tag;
            std::vector<float> args;

            // Cube herda de Block, então vem primeiro.
            if (dynamic_cast<const Cube *>(&s))
            {
                tag = "Cb";
                args = {b[0]};
            }
            else if (dynamic_cast<const Block *>(&s))
            {
                tag = "Cx";
                args = {b[1], b[0], b[2]}; // altura, largura, profundidade
            }
            else if (const auto *sp = dynamic_cast<const Sphere *>(&s))
            {
                tag = "Es";
                args = {sp->radius()};
            }
            else if (const auto *cy = dynamic_cast<const Cylinder *>(&s))
            {
                tag = "Ci";
                args = {cy->radius(), cy->height()};
            }
            else if (const auto *co = dynamic_cast<const Cone *>(&s))
            {
                tag = "Cn";
                args = {co->radius(), co->height()};
            }
            else if (const auto *ca = dynamic_cast<const Capsule *>(&s))
            {
                tag = "Cp";
                args = {ca->radius(), ca->height()};
            }
            else if (dynamic_cast<const RegularTetrahedron *>(&s))
            {
                tag = "Te";
                args = {b[0] * std::numbers::sqrt2_v<float>}; // aresta
            }
            else
                throw std::logic_error("Csg: shape sem representação textual");

            const Rot3f &q = p.rotator;

            std::string out{tag};
            out += "(Ps(";
            appendFloats(out, std::array{p.origin[0], p.origin[1], p.origin[2]});
            out += "),Tr(";
            appendFloats(out, std::array{q.x(), q.y(), q.z(), q.w()});
            out += "),";
            appendFloats(out, args);
            out += ')';
            return out;
        }
    } // namespace detail

    class CsgNode
    {
    public:
        using Data = std::variant<CsgPrimitive, CsgOperation>;

        explicit CsgNode(CsgPrimitive primitive) : m_data(std::move(primitive)) {}
        explicit CsgNode(CsgOperation operation) : m_data(std::move(operation)) {}

        // Constrói a partir da expressão textual (ver toString). Lança
        // std::invalid_argument se o texto for inválido.
        explicit CsgNode(std::string_view text);

        // Versão sem exceção: o erro é a mensagem do parser.
        [[nodiscard]] static Result<CsgPtr, std::string> parse(std::string_view text);

        // Expressão textual do sólido, ex.: U(Es(Ps(0,0,0),Tr(1,0,0,0),2),Cx(...)).
        // U = união, D = diferença, I = interseção; Ps = posição, Tr = rotação
        // (quaternion x,y,z,w, como no Rot3f).
        [[nodiscard]] std::string toString() const
        {
            return std::visit(
                [](const auto &node) -> std::string
                {
                    if constexpr (std::is_same_v<std::decay_t<decltype(node)>, CsgPrimitive>)
                        return detail::primitiveToString(node);
                    else
                    {
                        constexpr const char *tags = "UID"; // ordem de CsgOp
                        return std::string{tags[static_cast<int>(node.op)]} + '(' +
                               node.left->toString() + ',' + node.right->toString() + ')';
                    }
                },
                m_data);
        }

        [[nodiscard]] const Data &data() const noexcept { return m_data; }
        [[nodiscard]] bool isPrimitive() const noexcept { return std::holds_alternative<CsgPrimitive>(m_data); }

        // Ponto (em coordenadas de mundo) pertence ao sólido?
        [[nodiscard]] bool contains(const Vec3f &point) const
        {
            return std::visit(
                [&](const auto &node) -> bool
                {
                    if constexpr (std::is_same_v<std::decay_t<decltype(node)>, CsgPrimitive>)
                    {
                        // Mundo -> local: desfaz a translação e a rotação (conjugado).
                        const Vec3f local =
                            (point - node.origin).rotated(node.rotator.quaternion().conjugated());
                        return detail::inside(*node.shape, local);
                    }
                    else
                    {
                        const bool l = node.left->contains(point);
                        const bool r = node.right->contains(point);

                        switch (node.op)
                        {
                        case CsgOp::Union:
                            return l || r;
                        case CsgOp::Intersection:
                            return l && r;
                        case CsgOp::Difference:
                            return l && !r;
                        }
                        return false;
                    }
                },
                m_data);
        }

        // Primeira passagem do raio pelo sólido (t >= 0), com as normais
        // externas de entrada e saída, ou nullopt se o raio não o atinge.
        // Se a origem já está dentro, tEnter = 0 e normalEnter = zero.
        // `direction` não precisa ser unitária: t é medido em unidades dela.
        [[nodiscard]] std::optional<RayHit> raycast(const Vec3f &origin, const Vec3f &direction) const
        {
            for (const auto &iv : rayIntervals(origin, direction))
            {
                if (iv.t1 > 0.0f)
                {
                    RayHit hit{std::max(iv.t0, 0.0f), iv.t1, iv.n0, iv.n1};
                    if (iv.t0 < 0.0f)
                        hit.normalEnter = detail::kZero;
                    return hit;
                }
            }

            return std::nullopt;
        }

        // Menor t >= 0 tal que origin + t * direction está dentro do sólido
        // (0 se a origem já está dentro), ou nullopt se o raio não o atinge.
        [[nodiscard]] std::optional<float> raytest(const Vec3f &origin, const Vec3f &direction) const
        {
            if (const auto hit = raycast(origin, direction))
                return hit->tEnter;

            return std::nullopt;
        }

    private:
        // Intervalos de t (ordenados, disjuntos) em que o raio está dentro do
        // sólido, em coordenadas de mundo.
        [[nodiscard]] detail::Intervals rayIntervals(const Vec3f &o, const Vec3f &d) const
        {
            return std::visit(
                [&](const auto &node) -> detail::Intervals
                {
                    if constexpr (std::is_same_v<std::decay_t<decltype(node)>, CsgPrimitive>)
                    {
                        // Mundo -> local. A transformação é rígida, então t não muda.
                        const auto forwardRot = node.rotator.quaternion();
                        const auto inverse = forwardRot.conjugated();

                        auto intervals = detail::shapeIntervals(*node.shape,
                                                                (o - node.origin).rotated(inverse),
                                                                d.rotated(inverse));

                        // Normais: local -> mundo.
                        for (auto &iv : intervals)
                        {
                            iv.n0 = iv.n0.rotated(forwardRot);
                            iv.n1 = iv.n1.rotated(forwardRot);
                        }

                        return intervals;
                    }
                    else
                    {
                        const auto l = node.left->rayIntervals(o, d);
                        const auto r = node.right->rayIntervals(o, d);

                        switch (node.op)
                        {
                        case CsgOp::Union:
                            return detail::unite(l, r);
                        case CsgOp::Intersection:
                            return detail::intersect(l, r);
                        case CsgOp::Difference:
                            return detail::subtract(l, r);
                        }
                        return {};
                    }
                },
                m_data);
        }

        Data m_data;
    };

    /* ================= CONSTRUÇÃO ================= */

    template <std::derived_from<Shape> S>
    [[nodiscard]] CsgPtr csgPrimitive(S shape, const Vec3f &origin = {}, const Rot3f &rotator = {})
    {
        return std::make_unique<CsgNode>(
            CsgPrimitive{std::make_unique<S>(std::move(shape)), origin, rotator});
    }

    [[nodiscard]] inline CsgPtr csgOperation(CsgOp op, CsgPtr left, CsgPtr right)
    {
        return std::make_unique<CsgNode>(CsgOperation{op, std::move(left), std::move(right)});
    }

    [[nodiscard]] inline CsgPtr csgUnion(CsgPtr left, CsgPtr right)
    {
        return csgOperation(CsgOp::Union, std::move(left), std::move(right));
    }

    [[nodiscard]] inline CsgPtr csgIntersection(CsgPtr left, CsgPtr right)
    {
        return csgOperation(CsgOp::Intersection, std::move(left), std::move(right));
    }

    [[nodiscard]] inline CsgPtr csgDifference(CsgPtr left, CsgPtr right)
    {
        return csgOperation(CsgOp::Difference, std::move(left), std::move(right));
    }

    /* ================= TEXTO: PARSER ================= */

    namespace detail
    {
        struct ShapeSpec
        {
            std::string_view tag;
            std::size_t arity;                           // argumentos depois de Ps e Tr
            std::unique_ptr<Shape> (*make)(const float *);
        };

        inline const std::array<ShapeSpec, 7> kShapeSpecs{{
            {"Es", 1, [](const float *a) -> std::unique_ptr<Shape>
             { return std::make_unique<Sphere>(a[0]); }},
            {"Cx", 3, [](const float *a) -> std::unique_ptr<Shape> // altura, largura, profundidade
             { return std::make_unique<Block>(a[1], a[0], a[2]); }},
            {"Cb", 1, [](const float *a) -> std::unique_ptr<Shape>
             { return std::make_unique<Cube>(a[0]); }},
            {"Ci", 2, [](const float *a) -> std::unique_ptr<Shape>
             { return std::make_unique<Cylinder>(a[0], a[1]); }},
            {"Cn", 2, [](const float *a) -> std::unique_ptr<Shape>
             { return std::make_unique<Cone>(a[0], a[1]); }},
            {"Cp", 2, [](const float *a) -> std::unique_ptr<Shape>
             { return std::make_unique<Capsule>(a[0], a[1]); }},
            {"Te", 1, [](const float *a) -> std::unique_ptr<Shape> // aresta
             { return std::make_unique<RegularTetrahedron>(a[0]); }},
        }};

        // Descida recursiva; lança std::invalid_argument com a posição do erro.
        class CsgParser
        {
        public:
            explicit CsgParser(std::string_view text) : m_text(text) {}

            CsgPtr parse()
            {
                CsgPtr root = node();
                skipSpaces();
                if (m_pos != m_text.size())
                    fail("texto após o fim da expressão");
                return root;
            }

        private:
            [[noreturn]] void fail(std::string_view message) const
            {
                throw std::invalid_argument("Csg: " + std::string{message} + " (posição " +
                                            std::to_string(m_pos) + ")");
            }

            void skipSpaces()
            {
                while (m_pos < m_text.size() && std::isspace(static_cast<unsigned char>(m_text[m_pos])))
                    ++m_pos;
            }

            void expect(char c)
            {
                skipSpaces();
                if (m_pos >= m_text.size() || m_text[m_pos] != c)
                    fail(std::string{"esperava '"} + c + "'");
                ++m_pos;
            }

            std::string_view tag()
            {
                skipSpaces();
                const std::size_t start = m_pos;
                while (m_pos < m_text.size() && std::isalpha(static_cast<unsigned char>(m_text[m_pos])))
                    ++m_pos;
                return m_text.substr(start, m_pos - start);
            }

            float number()
            {
                skipSpaces();
                const char *first = m_text.data() + m_pos;
                const char *last = m_text.data() + m_text.size();

                float v{};
                const auto [end, ec] = std::from_chars(first, last, v);
                if (ec != std::errc{} || !std::isfinite(v))
                    fail("número inválido");

                m_pos += static_cast<std::size_t>(end - first);
                return v;
            }

            // name(n1,...,nN), com N <= 4
            std::array<float, 4> tagged(std::string_view name, std::size_t n)
            {
                if (tag() != name)
                    fail(std::string{"esperava "} + std::string{name});

                std::array<float, 4> v{};
                expect('(');
                for (std::size_t i = 0; i < n; ++i)
                {
                    if (i > 0)
                        expect(',');
                    v[i] = number();
                }
                expect(')');
                return v;
            }

            CsgPtr node()
            {
                const std::string_view t = tag();
                expect('(');

                CsgPtr n;
                if (t == "U" || t == "I" || t == "D")
                {
                    CsgPtr left = node();
                    expect(',');
                    CsgPtr right = node();

                    const CsgOp op = t == "U"   ? CsgOp::Union
                                     : t == "I" ? CsgOp::Intersection
                                                : CsgOp::Difference;
                    n = csgOperation(op, std::move(left), std::move(right));
                }
                else
                    n = primitive(t);

                expect(')');
                return n;
            }

            CsgPtr primitive(std::string_view t)
            {
                const auto spec = std::ranges::find(kShapeSpecs, t, &ShapeSpec::tag);
                if (spec == kShapeSpecs.end())
                    fail("shape ou operação desconhecida '" + std::string{t} + "'");

                const auto pos = tagged("Ps", 3);
                expect(',');
                const auto rot = tagged("Tr", 4);
                if (rot == std::array<float, 4>{})
                    fail("quaternion nulo em Tr");

                std::array<float, 3> args{};
                for (std::size_t i = 0; i < spec->arity; ++i)
                {
                    expect(',');
                    args[i] = number();
                }

                return std::make_unique<CsgNode>(CsgPrimitive{
                    spec->make(args.data()),
                    Vec3f{pos[0], pos[1], pos[2]},
                    Rot3f{Rot3f::Quat{rot[0], rot[1], rot[2], rot[3]}}});
            }

            std::string_view m_text;
            std::size_t m_pos = 0;
        };
    } // namespace detail

    inline CsgNode::CsgNode(std::string_view text)
        : m_data(std::move(detail::CsgParser{text}.parse()->m_data))
    {
    }

    inline Result<CsgPtr, std::string> CsgNode::parse(std::string_view text)
    {
        try
        {
            return detail::CsgParser{text}.parse();
        }
        catch (const std::exception &e)
        {
            return result::err(std::string{e.what()});
        }
    }

} // namespace geometry