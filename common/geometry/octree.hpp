#pragma once

#include "mesh.hpp"
#include "bounding_volume.hpp"

#include <array>
#include <memory>
#include <algorithm>

namespace geometry
{
    enum class OctreeState
    {
        Empty,
        Branch,
        Filled
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
        Octree &build(
            Classifier &&classifier,
            OctreeOrientation orientation =
                OctreeOrientation::
                    CounterClockwiseBottomToTop)
        {
            m_orientation =
                orientation;

            m_root =
                OctreeNode();

            buildRecursive(
                m_root,
                m_bounds,
                0,
                classifier,
                orientation);

            return *this;
        }

        template <typename Visitor>
        void read(
            Visitor &&visitor) const
        {
            readRecursive(
                m_root,
                m_bounds,
                0,
                visitor,
                m_orientation);
        }

        [[nodiscard]]
        Mesh3f toMesh() const
        {
            Mesh3f mesh;

            buildMesh(
                mesh,
                m_root,
                m_bounds,
                m_orientation);

            return mesh;
        }

        Octree &unite(const Octree &other)
        {
            return apply(other, [](bool a, bool b)
                         { return a || b; });
        }

        Octree &intersect(const Octree &other)
        {
            return apply(other, [](bool a, bool b)
                         { return a && b; });
        }

        Octree &subtract(const Octree &other)
        {
            return apply(other, [](bool a, bool b)
                         { return a && !b; });
        }

    private:
        template <typename Op>
        Octree &apply(const Octree &other, Op op)
        {
            // As duas árvores precisam cobrir o mesmo AABB e ter a mesma
            // orientação, senão o filho i não é a mesma região do espaço.
            assert(m_orientation == other.m_orientation);

            OctreeNode result;
            combine(result, m_root, other.m_root, op);
            m_root = std::move(result); // result separado: seguro mesmo com other == *this

            return *this;
        }

        // Folha age como um ramo com 8 filhos iguais a ela mesma.
        static const OctreeNode &childOrLeaf(
            const OctreeNode &node,
            std::size_t index)
        {
            if (!node.isBranch())
            {
                return node;
            }

            static const OctreeNode empty;

            return node.children[index]
                       ? *node.children[index]
                       : empty;
        }

        template <typename Op>
        static void combine(
            OctreeNode &out,
            const OctreeNode &a,
            const OctreeNode &b,
            Op op)
        {
            const auto leaf = [](bool filled)
            {
                return filled ? OctreeState::Filled
                              : OctreeState::Empty;
            };

            if (!a.isBranch() && !b.isBranch())
            {
                out.state = leaf(op(a.isFilled(), b.isFilled()));
                return;
            }

            // Absorção: a folha decide sozinha, sem olhar o outro lado.
            if (!a.isBranch())
            {
                const bool f = a.isFilled();
                if (op(f, false) == op(f, true))
                {
                    out.state = leaf(op(f, false));
                    return;
                }
            }

            if (!b.isBranch())
            {
                const bool f = b.isFilled();
                if (op(false, f) == op(true, f))
                {
                    out.state = leaf(op(false, f));
                    return;
                }
            }

            out.makeBranch();

            for (std::size_t i = 0; i < 8; ++i)
            {
                combine(
                    *out.children[i],
                    childOrLeaf(a, i),
                    childOrLeaf(b, i),
                    op);
            }

            collapse(out);
        }

        // Ramo com 8 filhos folha de mesmo estado vira folha.
        static void collapse(OctreeNode &node)
        {
            const auto first = node.children[0]->state;

            if (first == OctreeState::Branch)
            {
                return;
            }

            for (const auto &child : node.children)
            {
                if (child->state != first)
                {
                    return;
                }
            }

            node.state = first;

            for (auto &child : node.children)
            {
                child.reset();
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

            child.minimum()[0] =
                (index & 1)
                    ? center[0]
                    : parent.minimum()[0];

            child.maximum()[0] =
                (index & 1)
                    ? parent.maximum()[0]
                    : center[0];

            child.minimum()[1] =
                (index & 2)
                    ? center[1]
                    : parent.minimum()[1];

            child.maximum()[1] =
                (index & 2)
                    ? parent.maximum()[1]
                    : center[1];

            child.minimum()[2] =
                (index & 4)
                    ? center[2]
                    : parent.minimum()[2];

            child.maximum()[2] =
                (index & 4)
                    ? parent.maximum()[2]
                    : center[2];

            return child;
        }

        template <typename Classifier>
        static void buildRecursive(
            OctreeNode &node,
            const AABB &bounds,
            std::size_t depth,
            Classifier &&classifier,
            OctreeOrientation orientation)
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
                        remapIndex(
                            i,
                            orientation)),
                    depth + 1,
                    classifier,
                    orientation);
            }
        }

        template <typename Visitor>
        static void readRecursive(
            const OctreeNode &node,
            const AABB &bounds,
            std::size_t depth,
            Visitor &&visitor,
            OctreeOrientation orientation)
        {
            visitor(
                bounds,
                depth,
                node);

            if (!node.isBranch())
            {
                return;
            }

            for (std::size_t i = 0;
                 i < 8;
                 ++i)
            {
                if (!node.children[i])
                {
                    continue;
                }

                readRecursive(
                    *node.children[i],
                    childBounds(
                        bounds,
                        remapIndex(
                            i,
                            orientation)),
                    depth + 1,
                    visitor,
                    orientation);
            }
        }

        static void appendMesh(
            Mesh3f &dst,
            const Mesh3f &src)
        {
            const auto offset =
                dst.vertexCount();

            for (const auto &vertex :
                 src.vertices())
            {
                (void)dst.addVertex(
                    vertex);
            }

            for (const auto &edge :
                 src.edges())
            {
                dst.addEdge(
                    edge.v1 + offset,
                    edge.v2 + offset);
            }

            for (const auto &face :
                 src.faces())
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
                    bounds.maximum() -
                    bounds.minimum();

                Mesh3f cube =
                    Block(
                        size[0],
                        size[1],
                        size[2])
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
                if (!node.children[i])
                {
                    continue;
                }

                buildMesh(
                    mesh,
                    *node.children[i],
                    childBounds(
                        bounds,
                        remapIndex(
                            i,
                            orientation)),
                    orientation);
            }
        }

    private:
        AABB m_bounds;

        OctreeOrientation m_orientation =
            OctreeOrientation::
                CounterClockwiseBottomToTop;

        OctreeNode m_root;

    public:
        const AABB &bounds() const { return m_bounds; }
        AABB &bounds() { return m_bounds; }

        inline Octree &moveBounds(Vec3f move)
        {
            m_bounds.maximum() += move;
            m_bounds.minimum() += move;

            return *this;
        }

        inline Octree &scaleBounds(Vec3f const scale)
        {
            m_bounds.maximum()[0] *= scale[0];
            m_bounds.maximum()[1] *= scale[1];
            m_bounds.maximum()[2] *= scale[2];
            m_bounds.minimum()[0] *= scale[0];
            m_bounds.minimum()[1] *= scale[1];
            m_bounds.minimum()[2] *= scale[2];

            return *this;
        }
    };

} // namespace geometry