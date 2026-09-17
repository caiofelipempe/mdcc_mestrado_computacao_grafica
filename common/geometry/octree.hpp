#pragma once

#include "mesh.hpp"
#include "shape.hpp"

#include <array>
#include <memory>
#include <vector>
#include <stdexcept>
#include <algorithm>

namespace geometry
{
    //==========================================================================
    // OctreeNodeType
    //==========================================================================

    enum class OctreeNodeType
    {
        Empty,
        Filled,
        Branch
    };

    //==========================================================================
    // OctreeNode
    //==========================================================================

    class OctreeNode
    {
    public:
        using Children =
            std::array<
                std::unique_ptr<OctreeNode>,
                8>;

        OctreeNode() = default;

        explicit OctreeNode(
            OctreeNodeType type)
            : m_type(type)
        {
        }

        [[nodiscard]]
        OctreeNodeType type() const
        {
            return m_type;
        }

        [[nodiscard]]
        bool isEmpty() const
        {
            return m_type ==
                   OctreeNodeType::Empty;
        }

        [[nodiscard]]
        bool isFilled() const
        {
            return m_type ==
                   OctreeNodeType::Filled;
        }

        [[nodiscard]]
        bool isBranch() const
        {
            return m_type ==
                   OctreeNodeType::Branch;
        }

        [[nodiscard]]
        bool isLeaf() const
        {
            return !isBranch();
        }

        void setEmpty()
        {
            m_type =
                OctreeNodeType::Empty;

            m_children = {};
        }

        void setFilled()
        {
            m_type =
                OctreeNodeType::Filled;

            m_children = {};
        }

        void subdivide()
        {
            if (isBranch())
            {
                return;
            }

            m_type =
                OctreeNodeType::Branch;

            for (auto &child : m_children)
            {
                child =
                    std::make_unique<
                        OctreeNode>(
                        OctreeNodeType::Empty);
            }
        }

        [[nodiscard]]
        OctreeNode *child(
            std::size_t index)
        {
            return m_children.at(index).get();
        }

        [[nodiscard]]
        const OctreeNode *child(
            std::size_t index) const
        {
            return m_children.at(index).get();
        }

        [[nodiscard]]
        bool canCollapse() const
        {
            if (!isBranch())
            {
                return false;
            }

            bool allEmpty = true;
            bool allFilled = true;

            for (const auto &child : m_children)
            {
                allEmpty &= child->isEmpty();
                allFilled &= child->isFilled();
            }

            return allEmpty || allFilled;
        }

        void collapse()
        {
            if (!isBranch())
            {
                return;
            }

            for (auto &child : m_children)
            {
                child->collapse();
            }

            if (!canCollapse())
            {
                return;
            }

            bool allEmpty = true;

            for (const auto &child : m_children)
            {
                allEmpty &= child->isEmpty();
            }

            if (allEmpty)
            {
                setEmpty();
            }
            else
            {
                setFilled();
            }
        }

    private:
        OctreeNodeType m_type =
            OctreeNodeType::Empty;

        Children m_children;
    };

    //==========================================================================
    // Octree
    //==========================================================================

    class Octree
    {
    public:
        using Path =
            std::vector<std::uint8_t>;

        void fill(
            std::initializer_list<std::uint8_t> path)
        {
            fill(
                Path(path));
        }

        void clear(
            std::initializer_list<std::uint8_t> path)
        {
            clear(
                Path(path));
        }

        void subdivide(
            std::initializer_list<std::uint8_t> path)
        {
            subdivide(
                Path(path));
        }

        OctreeNode *create(
            std::initializer_list<std::uint8_t> path)
        {
            return create(
                Path(path));
        }

        OctreeNode *node(
            std::initializer_list<std::uint8_t> path)
        {
            return node(
                Path(path));
        }

        const OctreeNode *node(
            std::initializer_list<std::uint8_t> path) const
        {
            return node(
                Path(path));
        }

        std::pair<Point3f, Point3f>
        bounds(
            std::initializer_list<std::uint8_t> path) const
        {
            return bounds(
                Path(path));
        }

        Octree(
            const Point3f &minimum,
            const Point3f &maximum)
            : m_minimum(minimum),
              m_maximum(maximum)
        {
        }

        [[nodiscard]]
        OctreeNode &root()
        {
            return m_root;
        }

        [[nodiscard]]
        const OctreeNode &root() const
        {
            return m_root;
        }

        //======================================================================
        // Navegação
        //======================================================================

        [[nodiscard]]
        OctreeNode *node(
            const Path &path)
        {
            OctreeNode *current =
                &m_root;

            for (const auto index : path)
            {
                if (!current->isBranch())
                {
                    return nullptr;
                }

                current =
                    current->child(index);
            }

            return current;
        }

        [[nodiscard]]
        const OctreeNode *node(
            const Path &path) const
        {
            const OctreeNode *current =
                &m_root;

            for (const auto index : path)
            {
                if (!current->isBranch())
                {
                    return nullptr;
                }

                current =
                    current->child(index);
            }

            return current;
        }

        //======================================================================
        // Criação automática
        //======================================================================

        [[nodiscard]]
        OctreeNode *create(
            const Path &path)
        {
            OctreeNode *current =
                &m_root;

            for (const auto index : path)
            {
                if (!current->isBranch())
                {
                    current->subdivide();
                }

                current =
                    current->child(index);
            }

            return current;
        }

        //======================================================================
        // Operações
        //======================================================================

        void fill(
            const Path &path)
        {
            create(path)->setFilled();
        }

        void clear(
            const Path &path)
        {
            create(path)->setEmpty();
        }

        void subdivide(
            const Path &path)
        {
            create(path)->subdivide();
        }

        void collapse()
        {
            m_root.collapse();
        }

        //======================================================================
        // Estatísticas
        //======================================================================

        [[nodiscard]]
        std::size_t nodeCount() const
        {
            return countNodes(
                m_root);
        }

        [[nodiscard]]
        std::size_t leafCount() const
        {
            return countLeaves(
                m_root);
        }

        [[nodiscard]]
        std::size_t depth() const
        {
            return depthRecursive(
                m_root);
        }

        //======================================================================
        // Bounds
        //======================================================================

        [[nodiscard]]
        std::pair<Point3f, Point3f>
        bounds(
            const Path &path) const
        {
            Point3f min =
                m_minimum;

            Point3f max =
                m_maximum;

            for (const auto index : path)
            {
                splitBounds(
                    min,
                    max,
                    index,
                    min,
                    max);
            }

            return {
                min,
                max};
        }

        //======================================================================
        // Mesh
        //======================================================================

        [[nodiscard]]
        Mesh3f toMesh() const
        {
            Mesh3f mesh;

            buildMesh(
                mesh,
                m_root,
                m_minimum,
                m_maximum);

            return mesh;
        }

    private:
        static std::size_t countNodes(
            const OctreeNode &node)
        {
            std::size_t total = 1;

            if (!node.isBranch())
            {
                return total;
            }

            for (std::size_t i = 0; i < 8; ++i)
            {
                total +=
                    countNodes(
                        *node.child(i));
            }

            return total;
        }

        static std::size_t countLeaves(
            const OctreeNode &node)
        {
            if (!node.isBranch())
            {
                return 1;
            }

            std::size_t total = 0;

            for (std::size_t i = 0; i < 8; ++i)
            {
                total +=
                    countLeaves(
                        *node.child(i));
            }

            return total;
        }

        static std::size_t depthRecursive(
            const OctreeNode &node)
        {
            if (!node.isBranch())
            {
                return 0;
            }

            std::size_t maxDepth = 0;

            for (std::size_t i = 0; i < 8; ++i)
            {
                maxDepth =
                    std::max(
                        maxDepth,
                        depthRecursive(
                            *node.child(i)));
            }

            return maxDepth + 1;
        }

        static void splitBounds(
            const Point3f &min,
            const Point3f &max,
            std::size_t index,
            Point3f &childMin,
            Point3f &childMax)
        {
            const auto center =
                min.midpoint(max);

            childMin[0] =
                (index & 1)
                    ? center[0]
                    : min[0];

            childMax[0] =
                (index & 1)
                    ? max[0]
                    : center[0];

            childMin[1] =
                (index & 2)
                    ? center[1]
                    : min[1];

            childMax[1] =
                (index & 2)
                    ? max[1]
                    : center[1];

            childMin[2] =
                (index & 4)
                    ? center[2]
                    : min[2];

            childMax[2] =
                (index & 4)
                    ? max[2]
                    : center[2];
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
                (void)dst.addVertex(vertex);
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
            const Point3f &min,
            const Point3f &max)
        {
            if (node.isEmpty())
            {
                return;
            }

            if (node.isFilled())
            {
                const auto center =
                    min.midpoint(max);

                const auto size =
                    max - min;

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

            for (std::size_t i = 0; i < 8; ++i)
            {
                Point3f childMin;
                Point3f childMax;

                splitBounds(
                    min,
                    max,
                    i,
                    childMin,
                    childMax);

                buildMesh(
                    mesh,
                    *node.child(i),
                    childMin,
                    childMax);
            }
        }

    private:
        Point3f m_minimum;
        Point3f m_maximum;

        OctreeNode m_root;
    };

} // namespace geometry