#pragma once

#include "mesh.hpp"
#include "rotator3.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

namespace geometry
{
    class AABBShape;

    // ============================================================================
    // Shape
    // ============================================================================

    class Shape
    {
    public:
        virtual ~Shape() = default;

        [[nodiscard]]
        virtual float area() const = 0;

        [[nodiscard]]
        virtual float volume() const = 0;

        [[nodiscard]]
        virtual AABBShape aabbLimits() const = 0;

        [[nodiscard]]
        virtual Mesh3f toMesh() const = 0;
    };

    // ============================================================================
    // ShapeCircular
    // ============================================================================

    class ShapeCircular : public Shape
    {
    public:
        static constexpr int DefaultSegments = 32;

        [[nodiscard]]
        Mesh3f toMesh() const final
        {
            return toMesh(DefaultSegments);
        }

        [[nodiscard]]
        virtual Mesh3f toMesh(int segments) const = 0;

    protected:
        // Anel em torno do eixo Y, com theta crescente (sentido -Y visto de cima).
        static std::vector<std::size_t> addRing(
            Mesh3f &mesh,
            float radius,
            float y,
            int segments)
        {
            std::vector<std::size_t> ring;
            ring.reserve(static_cast<std::size_t>(segments));

            for (int i = 0; i < segments; ++i)
            {
                const float angle =
                    2.0f *
                    std::numbers::pi_v<float> *
                    static_cast<float>(i) /
                    static_cast<float>(segments);

                ring.push_back(
                    mesh.addVertex({radius * std::cos(angle), y, radius * std::sin(angle)}));
            }

            return ring;
        }

        // `lower` deve estar abaixo de `upper`; gera faces com normal para fora.
        static void connectRings(
            Mesh3f &mesh,
            const std::vector<std::size_t> &lower,
            const std::vector<std::size_t> &upper)
        {
            for (std::size_t i = 0; i < lower.size(); ++i)
            {
                const std::size_t next =
                    (i + 1) % lower.size();

                mesh.addFace(
                    lower[i],
                    upper[i],
                    upper[next]);

                mesh.addFace(
                    lower[i],
                    upper[next],
                    lower[next]);
            }
        }

        // Leque entre um vértice único (ápice ou centro de tampa) e um anel.
        static void connectApex(
            Mesh3f &mesh,
            std::size_t apex,
            const std::vector<std::size_t> &ring,
            bool apexAbove)
        {
            for (std::size_t i = 0; i < ring.size(); ++i)
            {
                const std::size_t next =
                    (i + 1) % ring.size();

                if (apexAbove)
                {
                    mesh.addFace(apex, ring[next], ring[i]);
                }
                else
                {
                    mesh.addFace(apex, ring[i], ring[next]);
                }
            }
        }
    };

    // ============================================================================
    // AABBShape
    // ============================================================================

    class AABBShape : public Shape
    {
    public:
        AABBShape(
            const Point3f &min = {-1.0f, -1.0f, -1.0f},
            const Point3f &max = {1.0f, 1.0f, 1.0f})
            : m_min(min),
              m_max(max)
        {
        }

        [[nodiscard]]
        const Point3f &min() const
        {
            return m_min;
        }

        [[nodiscard]]
        const Point3f &max() const
        {
            return m_max;
        }

        [[nodiscard]]
        Vec3f size() const
        {
            return m_max - m_min;
        }

        [[nodiscard]]
        Vec3f extent() const
        {
            return (m_max - m_min) * 0.5f;
        }

        [[nodiscard]]
        Point3f center() const
        {
            return m_min.midpoint(m_max);
        }

        [[nodiscard]]
        float width() const
        {
            return m_max[0] - m_min[0];
        }

        [[nodiscard]]
        float height() const
        {
            return m_max[1] - m_min[1];
        }

        [[nodiscard]]
        float depth() const
        {
            return m_max[2] - m_min[2];
        }

        [[nodiscard]]
        float area() const override
        {
            const auto s = size();

            return 2.0f *
                   (s[0] * s[1] +
                    s[0] * s[2] +
                    s[1] * s[2]);
        }

        [[nodiscard]]
        float volume() const override
        {
            const auto s = size();

            return s[0] *
                   s[1] *
                   s[2];
        }

        [[nodiscard]]
        AABBShape aabbLimits() const override
        {
            return *this;
        }

        // Única implementação de caixa do arquivo: Cube e Block delegam aqui.
        [[nodiscard]]
        Mesh3f toMesh() const override
        {
            Mesh3f mesh;

            const auto v0 = mesh.addVertex({m_min[0], m_min[1], m_min[2]});
            const auto v1 = mesh.addVertex({m_max[0], m_min[1], m_min[2]});
            const auto v2 = mesh.addVertex({m_max[0], m_max[1], m_min[2]});
            const auto v3 = mesh.addVertex({m_min[0], m_max[1], m_min[2]});

            const auto v4 = mesh.addVertex({m_min[0], m_min[1], m_max[2]});
            const auto v5 = mesh.addVertex({m_max[0], m_min[1], m_max[2]});
            const auto v6 = mesh.addVertex({m_max[0], m_max[1], m_max[2]});
            const auto v7 = mesh.addVertex({m_min[0], m_max[1], m_max[2]});

            // Arestas

            mesh.addEdge(v0, v1);
            mesh.addEdge(v1, v2);
            mesh.addEdge(v2, v3);
            mesh.addEdge(v3, v0);

            mesh.addEdge(v4, v5);
            mesh.addEdge(v5, v6);
            mesh.addEdge(v6, v7);
            mesh.addEdge(v7, v4);

            mesh.addEdge(v0, v4);
            mesh.addEdge(v1, v5);
            mesh.addEdge(v2, v6);
            mesh.addEdge(v3, v7);

            // Faces (CCW vistas de fora)

            mesh.addFace(v0, v3, v2); // -Z
            mesh.addFace(v0, v2, v1);

            mesh.addFace(v4, v5, v6); // +Z
            mesh.addFace(v4, v6, v7);

            mesh.addFace(v0, v1, v5); // -Y
            mesh.addFace(v0, v5, v4);

            mesh.addFace(v3, v7, v6); // +Y
            mesh.addFace(v3, v6, v2);

            mesh.addFace(v1, v2, v6); // +X
            mesh.addFace(v1, v6, v5);

            mesh.addFace(v0, v4, v7); // -X
            mesh.addFace(v0, v7, v3);

            return mesh;
        }

    private:
        Point3f m_min;
        Point3f m_max;
    };

    // ============================================================================
    // Cube
    // ============================================================================

    class Cube : public Shape
    {
    public:
        explicit Cube(float size = 1.0f)
            : m_size(size)
        {
        }

        [[nodiscard]]
        float size() const
        {
            return m_size;
        }

        [[nodiscard]]
        float area() const override
        {
            return 6.0f * m_size * m_size;
        }

        [[nodiscard]]
        float volume() const override
        {
            return m_size * m_size * m_size;
        }

        [[nodiscard]]
        AABBShape aabbLimits() const override
        {
            const float h = m_size * 0.5f;

            return {
                {-h, -h, -h},
                {h, h, h}};
        }

        [[nodiscard]]
        Mesh3f toMesh() const override
        {
            return aabbLimits().toMesh();
        }

    private:
        float m_size;
    };

    // ============================================================================
    // Block
    // ============================================================================

    class Block : public Shape
    {
    public:
        Block(
            float width = 1.0f,
            float height = 1.0f,
            float depth = 1.0f)
            : m_width(width), m_height(height), m_depth(depth)
        {
        }

        [[nodiscard]]
        float area() const override
        {
            return 2.0f *
                   (m_width * m_height +
                    m_width * m_depth +
                    m_height * m_depth);
        }

        [[nodiscard]]
        float volume() const override
        {
            return m_width *
                   m_height *
                   m_depth;
        }

        [[nodiscard]]
        AABBShape aabbLimits() const override
        {
            return {
                {-m_width * 0.5f,
                 -m_height * 0.5f,
                 -m_depth * 0.5f},
                {m_width * 0.5f,
                 m_height * 0.5f,
                 m_depth * 0.5f}};
        }

        [[nodiscard]]
        Mesh3f toMesh() const override
        {
            return aabbLimits().toMesh();
        }

    private:
        float m_width;
        float m_height;
        float m_depth;
    };

    // ============================================================================
    // RegularTetrahedron
    // ============================================================================

    class RegularTetrahedron : public Shape
    {
    public:
        explicit RegularTetrahedron(float edgeLength = 1.0f)
            : m_edgeLength(edgeLength)
        {
        }

        [[nodiscard]]
        float area() const override
        {
            return std::numbers::sqrt3_v<float> *
                   m_edgeLength *
                   m_edgeLength;
        }

        [[nodiscard]]
        float volume() const override
        {
            return (m_edgeLength *
                    m_edgeLength *
                    m_edgeLength) /
                   (6.0f * std::numbers::sqrt2_v<float>);
        }

        [[nodiscard]]
        AABBShape aabbLimits() const override
        {
            const float s = scale();

            return {
                {-s, -s, -s},
                {s, s, s}};
        }

        [[nodiscard]]
        Mesh3f toMesh() const override
        {
            Mesh3f mesh;

            const float s = scale();

            const auto v0 = mesh.addVertex({s, s, s});
            const auto v1 = mesh.addVertex({-s, -s, s});
            const auto v2 = mesh.addVertex({-s, s, -s});
            const auto v3 = mesh.addVertex({s, -s, -s});

            // Arestas
            mesh.addEdge(v0, v1);
            mesh.addEdge(v0, v2);
            mesh.addEdge(v0, v3);

            mesh.addEdge(v1, v2);
            mesh.addEdge(v2, v3);
            mesh.addEdge(v3, v1);

            // Faces
            mesh.addFace(v0, v1, v2);
            mesh.addFace(v0, v3, v1);
            mesh.addFace(v0, v2, v3);
            mesh.addFace(v1, v3, v2);

            return mesh;
        }

    private:
        [[nodiscard]]
        float scale() const
        {
            // aresta = 2*sqrt(2)*s para vértices alternados do cubo [-s, s]^3
            constexpr float edgeBasis =
                2.0f * std::numbers::sqrt2_v<float>;

            return m_edgeLength / edgeBasis;
        }

        float m_edgeLength;
    };

    // ============================================================================
    // Sphere
    // ============================================================================

    class Sphere : public ShapeCircular
    {
    public:
        using ShapeCircular::toMesh;

        explicit Sphere(float radius = 1.0f)
            : m_radius(radius)
        {
        }

        [[nodiscard]]
        float area() const override
        {
            return 4.0f *
                   std::numbers::pi_v<float> *
                   m_radius *
                   m_radius;
        }

        [[nodiscard]]
        float volume() const override
        {
            return 4.0f / 3.0f *
                   std::numbers::pi_v<float> *
                   m_radius *
                   m_radius *
                   m_radius;
        }

        [[nodiscard]]
        AABBShape aabbLimits() const override
        {
            return {
                {-m_radius, -m_radius, -m_radius},
                {m_radius, m_radius, m_radius}};
        }

        [[nodiscard]]
        Mesh3f toMesh(int segments) const override
        {
            Mesh3f mesh;

            const int rings = std::max(2, segments / 2);

            // Polos são vértices únicos: nada de anel degenerado de raio zero.
            const auto bottom =
                mesh.addVertex({0.0f, -m_radius, 0.0f});

            std::vector<std::vector<std::size_t>> ringIndices;
            ringIndices.reserve(static_cast<std::size_t>(rings - 1));

            for (int r = 1; r < rings; ++r)
            {
                // phi = pi no polo inferior, 0 no superior
                const float phi =
                    std::numbers::pi_v<float> *
                    (1.0f -
                     static_cast<float>(r) /
                         static_cast<float>(rings));

                ringIndices.push_back(
                    addRing(
                        mesh,
                        m_radius * std::sin(phi),
                        m_radius * std::cos(phi),
                        segments));
            }

            const auto top =
                mesh.addVertex({0.0f, m_radius, 0.0f});

            connectApex(mesh, bottom, ringIndices.front(), false);

            for (std::size_t i = 0;
                 i + 1 < ringIndices.size();
                 ++i)
            {
                connectRings(
                    mesh,
                    ringIndices[i],
                    ringIndices[i + 1]);
            }

            connectApex(mesh, top, ringIndices.back(), true);

            return mesh;
        }

    private:
        float m_radius;
    };

    // ============================================================================
    // Cylinder
    // ============================================================================

    class Cylinder : public ShapeCircular
    {
    public:
        using ShapeCircular::toMesh;

        Cylinder(
            float radius = 1.0f,
            float height = 1.0f)
            : m_radius(radius), m_height(height)
        {
        }

        [[nodiscard]]
        float area() const override
        {
            return 2.0f *
                   std::numbers::pi_v<float> *
                   m_radius *
                   (m_radius + m_height);
        }

        [[nodiscard]]
        float volume() const override
        {
            return std::numbers::pi_v<float> *
                   m_radius *
                   m_radius *
                   m_height;
        }

        [[nodiscard]]
        AABBShape aabbLimits() const override
        {
            return {
                {-m_radius, -m_height * 0.5f, -m_radius},
                {m_radius, m_height * 0.5f, m_radius}};
        }

        [[nodiscard]]
        Mesh3f toMesh(int segments) const override
        {
            Mesh3f mesh;

            const float h = m_height * 0.5f;

            const auto bottom =
                addRing(mesh, m_radius, -h, segments);

            const auto top =
                addRing(mesh, m_radius, h, segments);

            connectRings(mesh, bottom, top);

            // Tampas: area() já as considera, a malha precisa fechá-las.
            const auto bottomCenter =
                mesh.addVertex({0.0f, -h, 0.0f});

            const auto topCenter =
                mesh.addVertex({0.0f, h, 0.0f});

            connectApex(mesh, bottomCenter, bottom, false);
            connectApex(mesh, topCenter, top, true);

            return mesh;
        }

    private:
        float m_radius;
        float m_height;
    };

    // ============================================================================
    // Cone
    // ============================================================================

    class Cone : public ShapeCircular
    {
    public:
        using ShapeCircular::toMesh;

        Cone(
            float radius = 1.0f,
            float height = 1.0f)
            : m_radius(radius), m_height(height)
        {
        }

        [[nodiscard]]
        float area() const override
        {
            const float slant =
                std::sqrt(
                    m_radius * m_radius +
                    m_height * m_height);

            return std::numbers::pi_v<float> *
                   m_radius *
                   (slant + m_radius);
        }

        [[nodiscard]]
        float volume() const override
        {
            return (std::numbers::pi_v<float> *
                    m_radius *
                    m_radius *
                    m_height) /
                   3.0f;
        }

        [[nodiscard]]
        AABBShape aabbLimits() const override
        {
            return {
                {-m_radius, -m_height * 0.5f, -m_radius},
                {m_radius, m_height * 0.5f, m_radius}};
        }

        [[nodiscard]]
        Mesh3f toMesh(int segments) const override
        {
            Mesh3f mesh;

            const float h = m_height * 0.5f;

            const auto ring =
                addRing(mesh, m_radius, -h, segments);

            const auto tip =
                mesh.addVertex({0.0f, h, 0.0f});

            const auto baseCenter =
                mesh.addVertex({0.0f, -h, 0.0f});

            connectApex(mesh, tip, ring, true);
            connectApex(mesh, baseCenter, ring, false);

            return mesh;
        }

    private:
        float m_radius;
        float m_height;
    };

    // ============================================================================
    // PolarConvexHullShape
    // ============================================================================

    // Representação polar: o sólido é descrito apenas pelo incentro e por um
    // vetor por face, com normal = v/|v| e distância ao incentro |v|. O sólido
    // é a interseção dos semiespaços { p : normal . (p - incenter) <= |v| }.
    //
    // Nenhuma topologia é armazenada: vértices e polígonos são derivados dos
    // vetores de face sempre que pedidos.
    class PolarConvexHullShape : public Shape
    {
    public:
        using SupportVector = Vec3f;

        static constexpr float DefaultTolerance = 1e-4f;

        PolarConvexHullShape() = default;

        explicit PolarConvexHullShape(const Point3f &incenter)
            : m_incenter(incenter)
        {
        }

        //==========================================================================
        // Acessores
        //==========================================================================

        [[nodiscard]]
        const Point3f &incenter() const
        {
            return m_incenter;
        }

        void setIncenter(const Point3f &incenter)
        {
            m_incenter = incenter;
        }

        [[nodiscard]]
        const std::vector<SupportVector> &supportVectors() const
        {
            return m_supportVectors;
        }

        void addFace(const SupportVector &support)
        {
            m_supportVectors.push_back(support);
        }

        void clearFaces()
        {
            m_supportVectors.clear();
        }

        //==========================================================================
        // Utilidades de face
        //==========================================================================

        [[nodiscard]]
        static Vec3f faceNormal(const SupportVector &support)
        {
            return support.normalized();
        }

        [[nodiscard]]
        static float faceDistance(const SupportVector &support)
        {
            return support.norm();
        }

        //==========================================================================
        // Consultas espaciais
        //==========================================================================

        [[nodiscard]]
        bool contains(
            const Point3f &point,
            float tolerance = DefaultTolerance) const
        {
            const Vec3f local = point - m_incenter;

            for (const auto &support : m_supportVectors)
            {
                if (faceNormal(support).dot(local) >
                    faceDistance(support) + tolerance)
                {
                    return false;
                }
            }

            return true;
        }

        //==========================================================================
        // Reconstrução de vértices
        //==========================================================================

        // Interseção de cada tripla de planos, mantida apenas se cair dentro
        // de todos os semiespaços.
        [[nodiscard]]
        std::vector<Point3f> vertices(
            float tolerance = DefaultTolerance) const
        {
            std::vector<Point3f> result;

            const std::size_t count = m_supportVectors.size();

            if (count < 4)
            {
                return result;
            }

            for (std::size_t i = 0; i < count; ++i)
            {
                for (std::size_t j = i + 1; j < count; ++j)
                {
                    for (std::size_t k = j + 1; k < count; ++k)
                    {
                        Point3f vertex;

                        if (!intersectPlanes(i, j, k, vertex))
                        {
                            continue;
                        }

                        if (!contains(vertex, tolerance))
                        {
                            continue;
                        }

                        const bool duplicate =
                            std::any_of(
                                result.begin(),
                                result.end(),
                                [&](const Point3f &existing)
                                {
                                    return existing.distance_to(vertex) < tolerance;
                                });

                        if (!duplicate)
                        {
                            result.push_back(vertex);
                        }
                    }
                }
            }

            return result;
        }

        //==========================================================================
        // Shape
        //==========================================================================

        [[nodiscard]]
        float area() const override
        {
            const auto verts = vertices();

            float total = 0.0f;

            forEachFace(
                verts,
                DefaultTolerance,
                [&](const std::vector<std::size_t> &ring)
                {
                    for (std::size_t i = 1; i + 1 < ring.size(); ++i)
                    {
                        const Vec3f ab = verts[ring[i]] - verts[ring[0]];
                        const Vec3f ac = verts[ring[i + 1]] - verts[ring[0]];

                        total += 0.5f * ab.cross(ac).norm();
                    }
                });

            return total;
        }

        [[nodiscard]]
        float volume() const override
        {
            const auto verts = vertices();

            float total = 0.0f;

            // Soma dos tetraedros (incentro, a, b, c). O incentro é interno,
            // logo todo termo é positivo para faces orientadas para fora.
            forEachFace(
                verts,
                DefaultTolerance,
                [&](const std::vector<std::size_t> &ring)
                {
                    const Vec3f pa = verts[ring[0]] - m_incenter;

                    for (std::size_t i = 1; i + 1 < ring.size(); ++i)
                    {
                        const Vec3f pb = verts[ring[i]] - m_incenter;
                        const Vec3f pc = verts[ring[i + 1]] - m_incenter;

                        total += pa.dot(pb.cross(pc)) / 6.0f;
                    }
                });

            return total;
        }

        [[nodiscard]]
        AABBShape aabbLimits() const override
        {
            const auto verts = vertices();

            if (verts.empty())
            {
                return {};
            }

            Point3f min = verts.front();
            Point3f max = verts.front();

            for (const auto &v : verts)
            {
                for (int axis = 0; axis < 3; ++axis)
                {
                    min[axis] = std::min(min[axis], v[axis]);
                    max[axis] = std::max(max[axis], v[axis]);
                }
            }

            return {min, max};
        }

        [[nodiscard]]
        Mesh3f toMesh() const override
        {
            Mesh3f mesh;

            const auto verts = vertices();

            std::vector<std::size_t> ids;
            ids.reserve(verts.size());

            for (const auto &vertex : verts)
            {
                ids.push_back(
                    mesh.addVertex(
                        vertex));
            }

            forEachFace(
                verts,
                DefaultTolerance,
                [&](const std::vector<std::size_t> &ring)
                {
                    // Leque a partir do primeiro vértice: o polígono é convexo.
                    for (std::size_t i = 1; i + 1 < ring.size(); ++i)
                    {
                        mesh.addFace(
                            ids[ring[0]],
                            ids[ring[i]],
                            ids[ring[i + 1]]);
                    }

                    // Cada aresta aparece em duas faces com orientações
                    // opostas, então a < b a insere exatamente uma vez.
                    for (std::size_t i = 0; i < ring.size(); ++i)
                    {
                        const std::size_t a = ring[i];
                        const std::size_t b = ring[(i + 1) % ring.size()];

                        if (a < b)
                        {
                            mesh.addEdge(ids[a], ids[b]);
                        }
                    }
                });

            return mesh;
        }

    private:
        //==========================================================================
        // Derivação das faces poligonais
        //==========================================================================

        // Para cada vetor de face, os vértices que repousam sobre aquele plano
        // formam o polígono da face, ordenado CCW visto de fora.
        template <typename Visitor>
        void forEachFace(
            const std::vector<Point3f> &verts,
            float tolerance,
            Visitor &&visit) const
        {
            std::vector<std::size_t> ring;

            for (const auto &support : m_supportVectors)
            {
                const Vec3f normal = faceNormal(support);
                const float distance = faceDistance(support);

                ring.clear();

                for (std::size_t v = 0; v < verts.size(); ++v)
                {
                    const float signedDistance =
                        normal.dot(verts[v] - m_incenter);

                    if (std::abs(signedDistance - distance) <= tolerance)
                    {
                        ring.push_back(v);
                    }
                }

                if (ring.size() < 3)
                {
                    continue;
                }

                sortFaceRing(ring, normal, verts);

                visit(ring);
            }
        }

        // Ordena os índices por ângulo em torno da normal da face.
        static void sortFaceRing(
            std::vector<std::size_t> &ring,
            const Vec3f &normal,
            const std::vector<Point3f> &verts)
        {
            const Point3f &anchor = verts[ring.front()];

            Vec3f offset{};

            for (const std::size_t index : ring)
            {
                offset = offset + (verts[index] - anchor);
            }

            const Point3f center =
                anchor + offset / static_cast<float>(ring.size());

            const Vec3f u = perpendicularTo(normal);
            const Vec3f v = normal.cross(u); // (u, v, normal) é dextrogiro

            std::sort(
                ring.begin(),
                ring.end(),
                [&](std::size_t lhs, std::size_t rhs)
                {
                    const Vec3f a = verts[lhs] - center;
                    const Vec3f b = verts[rhs] - center;

                    return std::atan2(a.dot(v), a.dot(u)) <
                           std::atan2(b.dot(v), b.dot(u));
                });
        }

        [[nodiscard]]
        static Vec3f perpendicularTo(const Vec3f &normal)
        {
            const Vec3f axis =
                std::abs(normal[0]) < 0.9f
                    ? Vec3f{1.0f, 0.0f, 0.0f}
                    : Vec3f{0.0f, 1.0f, 0.0f};

            return normal.cross(axis).normalized();
        }

        //==========================================================================
        // Interseção de planos
        //==========================================================================

        [[nodiscard]]
        bool intersectPlanes(
            std::size_t ia,
            std::size_t ib,
            std::size_t ic,
            Point3f &result) const
        {
            const Vec3f n1 = faceNormal(m_supportVectors[ia]);
            const Vec3f n2 = faceNormal(m_supportVectors[ib]);
            const Vec3f n3 = faceNormal(m_supportVectors[ic]);

            const float det = n1.dot(n2.cross(n3));

            if (std::abs(det) < 1e-6f)
            {
                return false;
            }

            const Vec3f position =
                (n2.cross(n3) * faceDistance(m_supportVectors[ia]) +
                 n3.cross(n1) * faceDistance(m_supportVectors[ib]) +
                 n1.cross(n2) * faceDistance(m_supportVectors[ic])) /
                det;

            result = m_incenter + position;

            return true;
        }

        Point3f m_incenter;
        std::vector<SupportVector> m_supportVectors;
    };

} // namespace geometry