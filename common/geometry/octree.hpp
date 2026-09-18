#pragma once

#include "mesh.hpp"
#include "shape.hpp"

#include <array>
#include <memory>
#include <algorithm>

namespace geometry
{
    struct AABB
    {
        Point3f minimum;
        Point3f maximum;

        [[nodiscard]]
        Point3f center() const
        {
            return minimum.midpoint(
                maximum);
        }
    };

    enum class OctreeState
    {
        Empty,
        Filled,
        Branch
    };

    enum class OctreeOrientation
    {
        CounterClockwiseBottomToTop,
        CounterClockwiseTopToBottom,

        ClockwiseBottomToTop,
        ClockwiseTopToBottom
    };

    struct OctreeNode
    {
        OctreeState state =
            OctreeState::Empty;

        std::array<
            std::unique_ptr<OctreeNode>,
            8>
            children;

        [[nodiscard]]
        bool isEmpty() const
        {
            return state ==
                   OctreeState::Empty;
        }

        [[nodiscard]]
        bool isFilled() const
        {
            return state ==
                   OctreeState::Filled;
        }

        [[nodiscard]]
        bool isBranch() const
        {
            return state ==
                   OctreeState::Branch;
        }

        void makeBranch()
        {
            state =
                OctreeState::Branch;

            for (auto &child : children)
            {
                if (!child)
                {
                    child =
                        std::make_unique<
                            OctreeNode>();
                }
            }
        }
    };

    class Octree
    {
    public:
        Octree(
            const Point3f &minimum,
            const Point3f &maximum)
            : m_bounds{
                  minimum,
                  maximum}
        {
        }

        template <typename Classifier>
        void build(
            Classifier &&classifier)
        {
            buildRecursive(
                m_root,
                m_bounds,
                0,
                classifier);
        }

        [[nodiscard]]
        Mesh3f toMesh(
            OctreeOrientation orientation =
                OctreeOrientation::
                    CounterClockwiseBottomToTop) const
        {
            Mesh3f mesh;

            buildMesh(
                mesh,
                m_root,
                m_bounds,
                orientation);

            return mesh;
        }

    private:
        template <typename Classifier>
        static void buildRecursive(
            OctreeNode &node,
            const AABB &bounds,
            std::size_t depth,
            Classifier &&classifier)
        {
            node.state =
                classifier(
                    bounds,
                    depth);

            if (!node.isBranch())
            {
                return;
            }

            node.makeBranch();

            for (std::size_t i = 0;
                 i < 8;
                 ++i)
            {
                buildRecursive(
                    *node.children[i],
                    childBounds(
                        bounds,
                        i),
                    depth + 1,
                    classifier);
            }
        }

        static std::size_t remapIndex(
            std::size_t index,
            OctreeOrientation orientation)
        {
            std::size_t x =
                index & 1;

            std::size_t y =
                (index >> 1) & 1;

            std::size_t z =
                (index >> 2) & 1;

            switch (orientation)
            {
            case OctreeOrientation::
                CounterClockwiseBottomToTop:
                break;

            case OctreeOrientation::
                CounterClockwiseTopToBottom:
                z = 1 - z;
                break;

            case OctreeOrientation::
                ClockwiseBottomToTop:
                std::swap(x, y);
                break;

            case OctreeOrientation::
                ClockwiseTopToBottom:
                std::swap(x, y);
                z = 1 - z;
                break;
            }

            return x |
                   (y << 1) |
                   (z << 2);
        }

        static AABB childBounds(
            const AABB &parent,
            std::size_t index)
        {
            const auto center =
                parent.center();

            AABB child;

            child.minimum[0] =
                (index & 1)
                    ? center[0]
                    : parent.minimum[0];

            child.maximum[0] =
                (index & 1)
                    ? parent.maximum[0]
                    : center[0];

            child.minimum[1] =
                (index & 2)
                    ? center[1]
                    : parent.minimum[1];

            child.maximum[1] =
                (index & 2)
                    ? parent.maximum[1]
                    : center[1];

            child.minimum[2] =
                (index & 4)
                    ? center[2]
                    : parent.minimum[2];

            child.maximum[2] =
                (index & 4)
                    ? parent.maximum[2]
                    : center[2];

            return child;
        }

        static void appendMesh(
            Mesh3f &dst,
            const Mesh3f &src)
        {
            const auto offset =
                dst.vertexCount();

            for (const auto &vertex :
                 src.getVertices())
            {
                (void)dst.addVertex(
                    vertex);
            }

            for (const auto &edge :
                 src.getEdges())
            {
                dst.addEdge(
                    edge.v1 + offset,
                    edge.v2 + offset);
            }

            for (const auto &face :
                 src.getFaces())
            {
                dst.addFace(
                    face.indices[0] + offset,
                    face.indices[1] + offset,
                    face.indices[2] + offset);
            }
        }

        static void buildMesh(
            Mesh3f &mesh,
            const OctreeNode &node,
            const AABB &bounds,
            OctreeOrientation orientation)
        {
            if (node.isEmpty())
            {
                return;
            }

            if (node.isFilled())
            {
                const auto center =
                    bounds.center();

                const auto size =
                    bounds.maximum -
                    bounds.minimum;

                Mesh3f cube =
                    Cube(size[0])
                        .toMesh();

                cube.translate(
                    center.to_vector());

                appendMesh(
                    mesh,
                    cube);

                return;
            }

            for (std::size_t i = 0;
                 i < 8;
                 ++i)
            {
                const auto octant =
                    remapIndex(
                        i,
                        orientation);

                buildMesh(
                    mesh,
                    *node.children[i],
                    childBounds(
                        bounds,
                        octant),
                    orientation);
            }
        }

    private:
        AABB m_bounds;

        OctreeNode m_root;
    };

} // namespace geometry