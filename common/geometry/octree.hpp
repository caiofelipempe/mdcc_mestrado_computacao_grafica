#pragma once

#include "mesh.hpp"
#include "bounding_volume.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace geometry
{
    class Octree
    {
    public:
        enum class Coverage
        {
            Empty,
            Partial,
            Filled
        };

        enum class Orientation
        {
            CounterClockwiseBottomToTop,
            CounterClockwiseTopToBottom,
            ClockwiseBottomToTop,
            ClockwiseTopToBottom
        };

        [[nodiscard]] static AABB unitSpace()
        {
            return AABB{Point3f{-1.0f, -1.0f, -1.0f}, Point3f{1.0f, 1.0f, 1.0f}};
        }

        template <typename Classifier>
        [[nodiscard]] static Octree fromShape(
            std::size_t maxDepth,
            Classifier &&classifier,
            Orientation orientation = Orientation::CounterClockwiseBottomToTop)
        {
            Octree tree(orientation);
            tree.grow(tree.m_root, unitSpace(), 0, maxDepth, classifier);
            return tree;
        }

        [[nodiscard]] static Octree fromString(
            const std::string &text,
            std::size_t maxDepth,
            Orientation orientation = Orientation::CounterClockwiseBottomToTop)
        {
            std::size_t pos = 0;

            return fromShape(
                maxDepth,
                [&](const AABB &)
                { return parseToken(text, pos); },
                orientation);
        }

        template <typename Visitor>
        void forEachLeaf(Visitor &&visitor, const AABB &space = unitSpace()) const
        {
            visitLeaves(m_root, space, 0, visitor);
        }

        [[nodiscard]] float volume(const AABB &space = unitSpace()) const
        {
            float total = 0.0f;

            forEachLeaf(
                [&](const AABB &box, bool filled, std::size_t)
                {
                    if (filled)
                        total += box.volume();
                },
                space);

            return total;
        }

        [[nodiscard]] Mesh3f toMesh(const AABB &space = unitSpace()) const
        {
            Mesh3f mesh;

            forEachLeaf(
                [&](const AABB &box, bool filled, std::size_t)
                {
                    if (!filled)
                        return;

                    const auto size = box.maximum() - box.minimum();

                    Mesh3f cube = Block(size[0], size[1], size[2]).toMesh();
                    cube.translate(box.center().to_vector());

                    appendMesh(mesh, cube);
                },
                space);

            return mesh;
        }

        [[nodiscard]] std::string toString() const
        {
            std::string text;
            writeString(m_root, text);
            return text;
        }

        friend Octree operator|(const Octree &a, const Octree &b)
        {
            return combined(a, b, [](bool x, bool y)
                            { return x || y; });
        }

        friend Octree operator&(const Octree &a, const Octree &b)
        {
            return combined(a, b, [](bool x, bool y)
                            { return x && y; });
        }

        friend Octree operator-(const Octree &a, const Octree &b)
        {
            return combined(a, b, [](bool x, bool y)
                            { return x && !y; });
        }

        Octree &operator|=(const Octree &other) { return *this = *this | other; }
        Octree &operator&=(const Octree &other) { return *this = *this & other; }
        Octree &operator-=(const Octree &other) { return *this = *this - other; }

    private:
        struct Node
        {
            bool filled = false;
            std::unique_ptr<std::array<Node, 8>> children;

            [[nodiscard]] bool isBranch() const { return children != nullptr; }

            void split()
            {
                filled = false;

                if (!children)
                {
                    children = std::make_unique<std::array<Node, 8>>();
                }
            }

            void setLeaf(bool value)
            {
                filled = value;
                children.reset();
            }
        };

        Orientation m_orientation;
        Node m_root;

        explicit Octree(Orientation orientation)
            : m_orientation(orientation)
        {
        }

        static Coverage parseToken(const std::string &text, std::size_t &pos)
        {
            if (pos >= text.size())
            {
                return Coverage::Empty;
            }

            switch (text[pos++])
            {
            case '1':
            case 'W':
            case 'w':
                return Coverage::Filled;
            case '(':
                return Coverage::Partial;
            default:
                return Coverage::Empty;
            }
        }

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
                fn((*node.children)[i], childBounds(bounds, i, m_orientation));
            }
        }

        template <typename Classifier>
        void grow(
            Node &node,
            const AABB &bounds,
            std::size_t depth,
            std::size_t maxDepth,
            Classifier &classifier) const
        {
            Coverage coverage = classifier(bounds);

            if (coverage == Coverage::Partial && depth >= maxDepth)
            {
                coverage = Coverage::Filled;
            }

            if (coverage != Coverage::Partial)
            {
                node.setLeaf(coverage == Coverage::Filled);
                return;
            }

            node.split();

            forEachChild(node, bounds, [&](Node &child, const AABB &childBox)
                         { grow(child, childBox, depth + 1, maxDepth, classifier); });
        }

        template <typename Visitor>
        void visitLeaves(
            const Node &node,
            const AABB &bounds,
            std::size_t depth,
            Visitor &visitor) const
        {
            if (!node.isBranch())
            {
                visitor(bounds, node.filled, depth);
                return;
            }

            forEachChild(node, bounds, [&](const Node &child, const AABB &childBox)
                         { visitLeaves(child, childBox, depth + 1, visitor); });
        }

        static void writeString(const Node &node, std::string &out)
        {
            if (!node.isBranch())
            {
                out += node.filled ? 'W' : 'B';
                return;
            }

            out += '(';

            for (const auto &child : *node.children)
            {
                writeString(child, out);
            }
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
        static Octree combined(const Octree &a, const Octree &b, Op op)
        {
            assert(a.m_orientation == b.m_orientation);

            Octree result(a.m_orientation);
            combine(result.m_root, a.m_root, b.m_root, op);
            return result;
        }

        static const Node &childOrLeaf(const Node &node, std::size_t index)
        {
            return node.isBranch() ? (*node.children)[index] : node;
        }

        template <typename Op>
        static void combine(Node &out, const Node &a, const Node &b, Op op)
        {
            if (!a.isBranch() && !b.isBranch())
            {
                out.setLeaf(op(a.filled, b.filled));
                return;
            }

            if (!a.isBranch() && op(a.filled, false) == op(a.filled, true))
            {
                out.setLeaf(op(a.filled, false));
                return;
            }

            if (!b.isBranch() && op(false, b.filled) == op(true, b.filled))
            {
                out.setLeaf(op(false, b.filled));
                return;
            }

            out.split();

            for (std::size_t i = 0; i < 8; ++i)
            {
                combine((*out.children)[i], childOrLeaf(a, i), childOrLeaf(b, i), op);
            }

            collapse(out);
        }

        static void collapse(Node &node)
        {
            const bool filled = (*node.children)[0].filled;

            for (const auto &child : *node.children)
            {
                if (child.isBranch() || child.filled != filled)
                {
                    return;
                }
            }

            node.setLeaf(filled);
        }
    };

} // namespace geometry