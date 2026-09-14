#pragma once

#include "mesh.hpp"
#include "rotator3.hpp"

#include <cmath>
#include <numbers>
#include <vector>

namespace geometry
{

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
        virtual Mesh3f toMesh(
            const Point3f &origin = {},
            const Rot3f &rotation = {}) const = 0;

    protected:
        [[nodiscard]]
        static Point3f transformPoint(
            const Point3f &point,
            const Point3f &origin,
            const Rot3f &rotation)
        {
            return origin +
                   rotation.rotateVector(
                       point.to_vector());
        }
    };

    // ============================================================================
    // ShapeCircular
    // ============================================================================

    class ShapeCircular : public Shape
    {
    public:
        static constexpr int DefaultSegments = 32;

        [[nodiscard]]
        Mesh3f toMesh(
            const Point3f &origin = {},
            const Rot3f &rotation = {}) const final
        {
            return toMesh(
                DefaultSegments,
                origin,
                rotation);
        }

        [[nodiscard]]
        virtual Mesh3f toMesh(
            int segments,
            const Point3f &origin = {},
            const Rot3f &rotation = {}) const = 0;

    protected:
        static std::vector<std::size_t> addRing(
            Mesh3f &mesh,
            float radius,
            float y,
            int segments,
            const Point3f &origin,
            const Rot3f &rotation)
        {
            std::vector<std::size_t> ring;

            for (int i = 0; i < segments; ++i)
            {
                const float angle =
                    2.0f *
                    std::numbers::pi_v<float> *
                    static_cast<float>(i) /
                    static_cast<float>(segments);

                ring.push_back(
                    mesh.addVertex(
                        Shape::transformPoint(
                            {radius * std::cos(angle),
                             y,
                             radius * std::sin(angle)},
                            origin,
                            rotation)));
            }

            return ring;
        }

        static void connectRings(
            Mesh3f &mesh,
            const std::vector<std::size_t> &a,
            const std::vector<std::size_t> &b)
        {
            for (std::size_t i = 0; i < a.size(); ++i)
            {
                const std::size_t next =
                    (i + 1) % a.size();

                mesh.addFace(
                    a[i],
                    a[next],
                    b[next]);

                mesh.addFace(
                    a[i],
                    b[next],
                    b[i]);
            }
        }
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
        Mesh3f toMesh(
            const Point3f &origin = {},
            const Rot3f &rotation = {}) const override
        {
            Mesh3f mesh;

            const float hx = m_width * 0.5f;
            const float hy = m_height * 0.5f;
            const float hz = m_depth * 0.5f;

            const auto v0 = mesh.addVertex(transformPoint({-hx, -hy, -hz}, origin, rotation));
            const auto v1 = mesh.addVertex(transformPoint({hx, -hy, -hz}, origin, rotation));
            const auto v2 = mesh.addVertex(transformPoint({hx, hy, -hz}, origin, rotation));
            const auto v3 = mesh.addVertex(transformPoint({-hx, hy, -hz}, origin, rotation));

            const auto v4 = mesh.addVertex(transformPoint({-hx, -hy, hz}, origin, rotation));
            const auto v5 = mesh.addVertex(transformPoint({hx, -hy, hz}, origin, rotation));
            const auto v6 = mesh.addVertex(transformPoint({hx, hy, hz}, origin, rotation));
            const auto v7 = mesh.addVertex(transformPoint({-hx, hy, hz}, origin, rotation));

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

            mesh.addFace(v0, v1, v2);
            mesh.addFace(v0, v2, v3);

            mesh.addFace(v4, v6, v5);
            mesh.addFace(v4, v7, v6);

            mesh.addFace(v0, v4, v5);
            mesh.addFace(v0, v5, v1);

            mesh.addFace(v1, v5, v6);
            mesh.addFace(v1, v6, v2);

            mesh.addFace(v2, v6, v7);
            mesh.addFace(v2, v7, v3);

            mesh.addFace(v3, v7, v4);
            mesh.addFace(v3, v4, v0);

            return mesh;
        }

    private:
        float m_width;
        float m_height;
        float m_depth;
    };

    // ============================================================================
    // HalfBlock
    // ============================================================================

    class HalfBlock : public Shape
    {
    public:
        HalfBlock(
            float width = 1.0f,
            float height = 1.0f,
            float depth = 1.0f)
            : m_width(width), m_height(height), m_depth(depth)
        {
        }

        [[nodiscard]]
        float area() const override
        {
            const float w = m_width;
            const float h = m_height;
            const float d = m_depth;

            const float diagonal =
                std::sqrt(
                    w * w +
                    h * h);

            return w * d +               // base
                   h * d +               // lado
                   0.5f * w * h * 2.0f + // frente e trás
                   diagonal * d;         // face inclinada
        }

        [[nodiscard]]
        float volume() const override
        {
            return (
                       m_width *
                       m_height *
                       m_depth) *
                   0.5f;
        }

        [[nodiscard]]
        Mesh3f toMesh(
            const Point3f &origin = {},
            const Rot3f &rotation = {}) const override
        {
            Mesh3f mesh;

            const float w = m_width * 0.5f;
            const float h = m_height * 0.5f;
            const float d = m_depth * 0.5f;

            const auto v0 =
                mesh.addVertex(transformPoint({-w, -h, -d}, origin, rotation));

            const auto v1 =
                mesh.addVertex(transformPoint({w, -h, -d}, origin, rotation));

            const auto v2 =
                mesh.addVertex(transformPoint({-w, h, -d}, origin, rotation));

            const auto v3 =
                mesh.addVertex(transformPoint({-w, -h, d}, origin, rotation));

            const auto v4 =
                mesh.addVertex(transformPoint({w, -h, d}, origin, rotation));

            const auto v5 =
                mesh.addVertex(transformPoint({-w, h, d}, origin, rotation));

            mesh.addEdge(v0, v1);
            mesh.addEdge(v1, v4);
            mesh.addEdge(v4, v3);
            mesh.addEdge(v3, v0);

            mesh.addEdge(v0, v2);
            mesh.addEdge(v2, v5);
            mesh.addEdge(v5, v3);

            mesh.addEdge(v2, v1);
            mesh.addEdge(v5, v4);

            mesh.addFace(v0, v1, v2);
            mesh.addFace(v3, v5, v4);

            mesh.addFace(v0, v3, v4);
            mesh.addFace(v0, v4, v1);

            mesh.addFace(v0, v2, v5);
            mesh.addFace(v0, v5, v3);

            mesh.addFace(v2, v1, v4);
            mesh.addFace(v2, v4, v5);

            return mesh;
        }

    private:
        float m_width;
        float m_height;
        float m_depth;
    };

    // ============================================================================
    // Sphere
    // ============================================================================

    class Sphere : public ShapeCircular
    {
    public:
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
        Mesh3f toMesh(
            int segments,
            const Point3f &origin = {},
            const Rot3f &rotation = {}) const override
        {
            Mesh3f mesh;

            const int rings = segments / 2;

            std::vector<std::vector<std::size_t>> ringIndices;

            for (int r = 0; r <= rings; ++r)
            {
                const float v =
                    static_cast<float>(r) /
                    static_cast<float>(rings);

                const float phi =
                    v * std::numbers::pi_v<float>;

                const float y =
                    m_radius *
                    std::cos(phi);

                const float radius =
                    m_radius *
                    std::sin(phi);

                ringIndices.push_back(
                    addRing(
                        mesh,
                        radius,
                        y,
                        segments,
                        origin,
                        rotation));
            }

            for (std::size_t i = 0;
                 i + 1 < ringIndices.size();
                 ++i)
            {
                connectRings(
                    mesh,
                    ringIndices[i],
                    ringIndices[i + 1]);
            }

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
                   (m_radius +
                    m_height);
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
        Mesh3f toMesh(
            int segments,
            const Point3f &origin = {},
            const Rot3f &rotation = {}) const override
        {
            Mesh3f mesh;

            auto bottom =
                addRing(
                    mesh,
                    m_radius,
                    -m_height * 0.5f,
                    segments,
                    origin,
                    rotation);

            auto top =
                addRing(
                    mesh,
                    m_radius,
                    m_height * 0.5f,
                    segments,
                    origin,
                    rotation);

            connectRings(
                mesh,
                bottom,
                top);

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
                   (slant +
                    m_radius);
        }

        [[nodiscard]]
        float volume() const override
        {
            return (
                       std::numbers::pi_v<float> *
                       m_radius *
                       m_radius *
                       m_height) /
                   3.0f;
        }

        [[nodiscard]]
        Mesh3f toMesh(
            int segments,
            const Point3f &origin = {},
            const Rot3f &rotation = {}) const override
        {
            Mesh3f mesh;

            const auto tip =
                mesh.addVertex(
                    transformPoint(
                        {0.0f,
                         m_height * 0.5f,
                         0.0f},
                        origin,
                        rotation));

            auto ring =
                addRing(
                    mesh,
                    m_radius,
                    -m_height * 0.5f,
                    segments,
                    origin,
                    rotation);

            for (int i = 0;
                 i < segments;
                 ++i)
            {
                const std::size_t next =
                    (i + 1) %
                    segments;

                mesh.addFace(
                    tip,
                    ring[i],
                    ring[next]);
            }

            return mesh;
        }

    private:
        float m_radius;
        float m_height;
    };

} // namespace geometry