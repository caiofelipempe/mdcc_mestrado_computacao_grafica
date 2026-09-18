#pragma once

#include "mesh.hpp"
#include "point.hpp"
#include "vector.hpp"

#include <algorithm>

namespace geometry
{
    // ============================================================================
    // BoundingVolume
    // ============================================================================

    class BoundingVolume
    {
    public:
        virtual ~BoundingVolume() = default;

        [[nodiscard]]
        virtual Point3f center() const = 0;

        [[nodiscard]]
        virtual Vec3f size() const = 0;

        [[nodiscard]]
        virtual float volume() const = 0;

        [[nodiscard]]
        virtual Mesh3f toMesh() const = 0;
    };

    // ============================================================================
    // AABB
    // ============================================================================

    class AABB final : public BoundingVolume
    {
    public:
        AABB() = default;

        AABB(
            const Point3f& minimum,
            const Point3f& maximum)
            : m_minimum(minimum),
              m_maximum(maximum)
        {
        }

        [[nodiscard]]
        Point3f center() const override
        {
            return m_minimum.midpoint(m_maximum);
        }

        [[nodiscard]]
        Vec3f size() const override
        {
            return m_maximum - m_minimum;
        }

        [[nodiscard]]
        float volume() const override
        {
            const auto s = size();

            return
                s[0] *
                s[1] *
                s[2];
        }

        [[nodiscard]]
        bool contains(
            const Point3f& point) const
        {
            return
                point[0] >= m_minimum[0] &&
                point[0] <= m_maximum[0] &&

                point[1] >= m_minimum[1] &&
                point[1] <= m_maximum[1] &&

                point[2] >= m_minimum[2] &&
                point[2] <= m_maximum[2];
        }

        [[nodiscard]]
        bool intersects(
            const AABB& other) const
        {
            return
                m_minimum[0] <= other.m_maximum[0] &&
                m_maximum[0] >= other.m_minimum[0] &&

                m_minimum[1] <= other.m_maximum[1] &&
                m_maximum[1] >= other.m_minimum[1] &&

                m_minimum[2] <= other.m_maximum[2] &&
                m_maximum[2] >= other.m_minimum[2];
        }

        void expand(const Point3f& point)
        {
            for (std::size_t i = 0; i < 3; ++i)
            {
                m_minimum[i] =
                    std::min(
                        m_minimum[i],
                        point[i]);

                m_maximum[i] =
                    std::max(
                        m_maximum[i],
                        point[i]);
            }
        }

        [[nodiscard]]
        Mesh3f toMesh() const override
        {
            Mesh3f mesh;

            const auto v0 =
                mesh.addVertex(
                    {m_minimum[0], m_minimum[1], m_minimum[2]});

            const auto v1 =
                mesh.addVertex(
                    {m_maximum[0], m_minimum[1], m_minimum[2]});

            const auto v2 =
                mesh.addVertex(
                    {m_maximum[0], m_maximum[1], m_minimum[2]});

            const auto v3 =
                mesh.addVertex(
                    {m_minimum[0], m_maximum[1], m_minimum[2]});

            const auto v4 =
                mesh.addVertex(
                    {m_minimum[0], m_minimum[1], m_maximum[2]});

            const auto v5 =
                mesh.addVertex(
                    {m_maximum[0], m_minimum[1], m_maximum[2]});

            const auto v6 =
                mesh.addVertex(
                    {m_maximum[0], m_maximum[1], m_maximum[2]});

            const auto v7 =
                mesh.addVertex(
                    {m_minimum[0], m_maximum[1], m_maximum[2]});

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

            return mesh;
        }

        [[nodiscard]]
        static AABB fromMesh(
            const Mesh3f& mesh)
        {
            const auto& vertices =
                mesh.getVertices();

            if (vertices.empty())
            {
                return {};
            }

            Point3f minimum =
                vertices.front();

            Point3f maximum =
                vertices.front();

            for (const auto& vertex : vertices)
            {
                for (std::size_t i = 0; i < 3; ++i)
                {
                    minimum[i] =
                        std::min(
                            minimum[i],
                            vertex[i]);

                    maximum[i] =
                        std::max(
                            maximum[i],
                            vertex[i]);
                }
            }

            return {
                minimum,
                maximum};
        }

    public:
        Point3f m_minimum{};
        Point3f m_maximum{};
    };

} // namespace geometry