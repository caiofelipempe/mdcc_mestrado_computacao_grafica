#pragma once

#include "mesh.hpp"
#include "bounding_volume.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

namespace geometry
{
    class Octree
    {
    public:
        enum class NodeState
        {
            Empty,
            Branch,
            Filled
        };

        enum class Orientation
        {
            CounterClockwiseBottomToTop,
            CounterClockwiseTopToBottom,
            ClockwiseBottomToTop,
            ClockwiseTopToBottom
        };

        struct Node
        {
            NodeState state = NodeState::Empty;
            std::array<std::unique_ptr<Node>, 8> children;

            [[nodiscard]] bool isEmpty() const { return state == NodeState::Empty; }
            [[nodiscard]] bool isFilled() const { return state == NodeState::Filled; }
            [[nodiscard]] bool isBranch() const { return state == NodeState::Branch; }

            void makeBranch()
            {
                state = NodeState::Branch;

                for (auto &child : children)
                {
                    if (!child)
                    {
                        child = std::make_unique<Node>();
                    }
                }
            }
        };

        [[nodiscard]] static AABB unitSpace()
        {
            return AABB{Point3f{-1.0f, -1.0f, -1.0f}, Point3f{1.0f, 1.0f, 1.0f}};
        }

        template <typename Classifier>
        Octree &build(
            Classifier &&classifier,
            Orientation orientation = Orientation::CounterClockwiseBottomToTop)
        {
            m_orientation = orientation;
            m_root = Node();

            buildRecursive(m_root, unitSpace(), 0, classifier);

            return *this;
        }

        template <typename Visitor>
        void read(Visitor &&visitor, const AABB &space = unitSpace()) const
        {
            readRecursive(m_root, space, 0, visitor);
        }

        [[nodiscard]] Mesh3f toMesh(const AABB &space = unitSpace()) const
        {
            Mesh3f mesh;
            buildMesh(mesh, m_root, space);
            return mesh;
        }

        [[nodiscard]] float volume(const AABB &space = unitSpace()) const
        {
            return volumeRecursive(m_root, space);
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
        Orientation m_orientation = Orientation::CounterClockwiseBottomToTop;
        Node m_root;

        static std::size_t remapIndex(std::size_t index, Orientation orientation)
        {
            std::size_t x = index & 1;
            std::size_t y = (index >> 1) & 1;
            std::size_t z = (index >> 2) & 1;

            switch (orientation)
            {
            case Orientation::CounterClockwiseBottomToTop:
                break;
            case Orientation::CounterClockwiseTopToBottom:
                z = 1 - z;
                break;
            case Orientation::ClockwiseBottomToTop:
                std::swap(x, y);
                break;
            case Orientation::ClockwiseTopToBottom:
                std::swap(x, y);
                z = 1 - z;
                break;
            }

            return x | (y << 1) | (z << 2);
        }

        static AABB childBounds(
            const AABB &parent,
            std::size_t index,
            Orientation orientation)
        {
            const std::size_t remapped = remapIndex(index, orientation);
            const auto center = parent.center();

            AABB child;

            for (std::size_t axis = 0; axis < 3; ++axis)
            {
                const bool upper = (remapped >> axis) & 1;

                child.minimum()[axis] = upper ? center[axis] : parent.minimum()[axis];
                child.maximum()[axis] = upper ? parent.maximum()[axis] : center[axis];
            }

            return child;
        }

        template <typename NodeT, typename Fn>
        void forEachChild(NodeT &node, const AABB &bounds, Fn &&fn) const
        {
            for (std::size_t i = 0; i < 8; ++i)
            {
                if (node.children[i])
                {
                    fn(*node.children[i], childBounds(bounds, i, m_orientation));
                }
            }
        }

        template <typename Classifier>
        void buildRecursive(
            Node &node,
            const AABB &bounds,
            std::size_t depth,
            Classifier &classifier) const
        {
            node.state = classifier(bounds, depth);

            if (!node.isBranch())
            {
                return;
            }

            node.makeBranch();

            forEachChild(node, bounds, [&](Node &child, const AABB &childBox)
                         { buildRecursive(child, childBox, depth + 1, classifier); });
        }

        template <typename Visitor>
        void readRecursive(
            const Node &node,
            const AABB &bounds,
            std::size_t depth,
            Visitor &visitor) const
        {
            visitor(bounds, depth, node);

            if (!node.isBranch())
            {
                return;
            }

            forEachChild(node, bounds, [&](const Node &child, const AABB &childBox)
                         { readRecursive(child, childBox, depth + 1, visitor); });
        }

        float volumeRecursive(const Node &node, const AABB &bounds) const
        {
            if (node.isEmpty())
            {
                return 0.0f;
            }

            if (node.isFilled())
            {
                return bounds.volume();
            }

            float total = 0.0f;

            forEachChild(node, bounds, [&](const Node &child, const AABB &childBox)
                         { total += volumeRecursive(child, childBox); });

            return total;
        }

        void buildMesh(Mesh3f &mesh, const Node &node, const AABB &bounds) const
        {
            if (node.isEmpty())
            {
                return;
            }

            if (node.isFilled())
            {
                const auto size = bounds.maximum() - bounds.minimum();

                Mesh3f cube = Block(size[0], size[1], size[2]).toMesh();
                cube.translate(bounds.center().to_vector());

                appendMesh(mesh, cube);
                return;
            }

            forEachChild(node, bounds, [&](const Node &child, const AABB &childBox)
                         { buildMesh(mesh, child, childBox); });
        }

        static void appendMesh(Mesh3f &dst, const Mesh3f &src)
        {
            const auto offset = dst.vertexCount();

            for (const auto &vertex : src.vertices())
            {
                (void)dst.addVertex(vertex);
            }

            for (const auto &edge : src.edges())
            {
                dst.addEdge(edge.v1 + offset, edge.v2 + offset);
            }

            for (const auto &face : src.faces())
            {
                dst.addFace(
                    face.indices[0] + offset,
                    face.indices[1] + offset,
                    face.indices[2] + offset);
            }
        }

        template <typename Op>
        Octree &apply(const Octree &other, Op op)
        {
            assert(m_orientation == other.m_orientation);

            Node result;
            combine(result, m_root, other.m_root, op);
            m_root = std::move(result);

            return *this;
        }

        static NodeState leaf(bool filled)
        {
            return filled ? NodeState::Filled : NodeState::Empty;
        }

        static const Node &childOrLeaf(const Node &node, std::size_t index)
        {
            if (!node.isBranch())
            {
                return node;
            }

            static const Node empty;

            return node.children[index] ? *node.children[index] : empty;
        }

        template <typename Op>
        static void combine(Node &out, const Node &a, const Node &b, Op op)
        {
            if (!a.isBranch() && !b.isBranch())
            {
                out.state = leaf(op(a.isFilled(), b.isFilled()));
                return;
            }

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
                combine(*out.children[i], childOrLeaf(a, i), childOrLeaf(b, i), op);
            }

            collapse(out);
        }

        static void collapse(Node &node)
        {
            const NodeState first = node.children[0]->state;

            if (first == NodeState::Branch)
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
    };

} // namespace geometry