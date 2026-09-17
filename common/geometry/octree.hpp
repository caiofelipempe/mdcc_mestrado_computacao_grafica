#pragma once

#include "mesh.hpp"
#include "shape.hpp"

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <stdexcept>
#include <algorithm>

namespace geometry
{
    enum class OctreeNodeType
    {
        Empty,
        Filled,
        Branch
    };

    struct OctreeNode
    {
        OctreeNodeType type =
            OctreeNodeType::Empty;

        std::array<
            std::unique_ptr<OctreeNode>,
            8> children;

        OctreeNode() = default;

        explicit OctreeNode(
            OctreeNodeType t)
            : type(t)
        {
        }

        [[nodiscard]]
        bool isEmpty() const
        {
            return type ==
                   OctreeNodeType::Empty;
        }

        [[nodiscard]]
        bool isFilled() const
        {
            return type ==
                   OctreeNodeType::Filled;
        }

        [[nodiscard]]
        bool isBranch() const
        {
            return type ==
                   OctreeNodeType::Branch;
        }

        void makeBranch()
        {
            if (isBranch())
            {
                return;
            }

            type =
                OctreeNodeType::Branch;

            for (auto& child : children)
            {
                child =
                    std::make_unique<
                        OctreeNode>(
                            OctreeNodeType::Empty);
            }
        }
    };

    class Octree
    {
    public:

        Octree(
            const Point3f& minimum,
            const Point3f& maximum)
            : m_minimum(minimum),
              m_maximum(maximum)
        {
        }

        Octree(
            const Point3f& minimum,
            const Point3f& maximum,
            std::string_view text)
            : m_minimum(minimum),
              m_maximum(maximum)
        {
            parse(text);
        }

        void parse(
            std::string_view text)
        {
            std::size_t cursor = 0;

            auto root =
                parseNode(
                    text,
                    cursor);

            if (cursor != text.size())
            {
                throw std::runtime_error(
                    "Unexpected characters");
            }

            m_root =
                std::move(*root);
        }

        [[nodiscard]]
        std::string serialize() const
        {
            std::string text;

            serializeNode(
                m_root,
                text);

            return text;
        }

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

        static std::unique_ptr<OctreeNode>
        parseNode(
            std::string_view text,
            std::size_t& cursor)
        {
            if (cursor >= text.size())
            {
                throw std::runtime_error(
                    "Unexpected end");
            }

            const char c =
                text[cursor++];

            if (c == '0')
            {
                return std::make_unique<
                    OctreeNode>(
                        OctreeNodeType::Empty);
            }

            if (c == '1')
            {
                return std::make_unique<
                    OctreeNode>(
                        OctreeNodeType::Filled);
            }

            if (c == '{')
            {
                auto node =
                    std::make_unique<
                        OctreeNode>();

                node->makeBranch();

                for (std::size_t i = 0; i < 8; ++i)
                {
                    node->children[i] =
                        parseNode(
                            text,
                            cursor);
                }

                if (cursor >= text.size())
                {
                    throw std::runtime_error(
                        "Missing }");
                }

                if (text[cursor] != '}')
                {
                    throw std::runtime_error(
                        "Missing }");
                }

                ++cursor;

                return node;
            }

            throw std::runtime_error(
                "Invalid character");
        }

        static void serializeNode(
            const OctreeNode& node,
            std::string& text)
        {
            if (node.isEmpty())
            {
                text.push_back('0');
                return;
            }

            if (node.isFilled())
            {
                text.push_back('1');
                return;
            }

            text.push_back('{');

            for (std::size_t i = 0; i < 8; ++i)
            {
                serializeNode(
                    *node.children[i],
                    text);
            }

            text.push_back('}');
        }

        static void splitBounds(
            const Point3f& min,
            const Point3f& max,
            std::size_t index,
            Point3f& childMin,
            Point3f& childMax)
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
            Mesh3f& dst,
            const Mesh3f& src)
        {
            const auto offset =
                dst.vertexCount();

            for (const auto& vertex :
                 src.getVertices())
            {
                (void)dst.addVertex(
                    vertex);
            }

            for (const auto& edge :
                 src.getEdges())
            {
                dst.addEdge(
                    edge.v1 + offset,
                    edge.v2 + offset);
            }

            for (const auto& face :
                 src.getFaces())
            {
                dst.addFace(
                    face.indices[0] + offset,
                    face.indices[1] + offset,
                    face.indices[2] + offset);
            }
        }

        static void buildMesh(
            Mesh3f& mesh,
            const OctreeNode& node,
            const Point3f& min,
            const Point3f& max)
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
                    *node.children[i],
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