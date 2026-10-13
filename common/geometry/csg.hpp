#pragma once

#include "rotator3.hpp"
#include "shape.hpp"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
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
    } // namespace detail

    class CsgNode
    {
    public:
        using Data = std::variant<CsgPrimitive, CsgOperation>;

        explicit CsgNode(CsgPrimitive primitive) : m_data(std::move(primitive)) {}
        explicit CsgNode(CsgOperation operation) : m_data(std::move(operation)) {}

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

} // namespace geometry