#pragma once

#include "algebric_vector.hpp"

#include <algorithm>
#include <array>
#include <compare>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace geometry
{
    struct Edge
    {
        std::size_t v1, v2;
        auto operator<=>(const Edge &) const = default;
    };

    struct Face
    {
        std::array<std::size_t, 3> indices;

        // Igualdade ignora a ordem dos índices.
        [[nodiscard]] bool operator==(const Face &other) const
        {
            auto a = indices, b = other.indices;
            std::ranges::sort(a);
            std::ranges::sort(b);
            return a == b;
        }
    };

    template <Scalar T, std::size_t N>
    class Mesh
    {
    public:
        using ValueType = T;
        using VectorType = AlgebricVector<T, N>;

        [[nodiscard]] const std::vector<VectorType> &vertices() const noexcept { return m_vertices; }
        [[nodiscard]] const std::vector<Edge> &edges() const noexcept { return m_edges; }
        [[nodiscard]] const std::vector<Face> &faces() const noexcept { return m_faces; }

        [[nodiscard]] std::vector<VectorType> &vertices() noexcept { return m_vertices; }
        [[nodiscard]] std::vector<Edge> &edges() noexcept { return m_edges; }
        [[nodiscard]] std::vector<Face> &faces() noexcept { return m_faces; }

        [[nodiscard]] std::size_t vertexCount() const noexcept { return m_vertices.size(); }
        [[nodiscard]] std::size_t edgeCount() const noexcept { return m_edges.size(); }
        [[nodiscard]] std::size_t faceCount() const noexcept { return m_faces.size(); }
        [[nodiscard]] bool empty() const noexcept { return m_vertices.empty(); }

        [[nodiscard]] std::size_t addVertex(const VectorType &point)
        {
            m_vertices.push_back(point);
            return m_vertices.size() - 1;
        }

        void addEdge(std::size_t v1, std::size_t v2)
        {
            const Edge e{v1, v2};
            if (!validEdge(e))
                throw std::invalid_argument("Mesh: invalid edge");
            m_edges.push_back(e);
        }

        void addFace(std::size_t i0, std::size_t i1, std::size_t i2)
        {
            const Face f{{i0, i1, i2}};
            if (!validFace(f))
                throw std::invalid_argument("Mesh: invalid face");
            m_faces.push_back(f);
        }

        // Substitui as arestas pelas arestas únicas das faces.
        void buildEdgesFromFaces()
        {
            m_edges.clear();

            for (const auto &[idx] : m_faces)
                for (std::size_t i = 0; i < 3; ++i)
                {
                    const auto a = idx[i], b = idx[(i + 1) % 3];
                    m_edges.push_back({std::min(a, b), std::max(a, b)});
                }

            std::ranges::sort(m_edges);
            m_edges.erase(std::ranges::unique(m_edges).begin(), m_edges.end());
        }

        void translate(const VectorType &offset)
        {
            for (auto &v : m_vertices)
                v += offset;
        }

        void clear()
        {
            m_vertices.clear();
            m_edges.clear();
            m_faces.clear();
        }

        // Útil após mutação externa via vertices()/edges()/faces().
        [[nodiscard]] bool valid() const noexcept
        {
            return std::ranges::all_of(m_edges, [this](const Edge &e) { return validEdge(e); }) &&
                   std::ranges::all_of(m_faces, [this](const Face &f) { return validFace(f); });
        }

        friend std::ostream &operator<<(std::ostream &os, const Mesh &m)
        {
            return os << "Mesh{ V: " << m.vertexCount()
                      << ", E: " << m.edgeCount()
                      << ", F: " << m.faceCount() << " }";
        }

    private:
        [[nodiscard]] bool validEdge(const Edge &e) const noexcept
        {
            return e.v1 < m_vertices.size() && e.v2 < m_vertices.size() && e.v1 != e.v2;
        }

        [[nodiscard]] bool validFace(const Face &f) const noexcept
        {
            const auto &[i] = f;
            return i[0] < m_vertices.size() && i[1] < m_vertices.size() && i[2] < m_vertices.size() &&
                   i[0] != i[1] && i[1] != i[2] && i[0] != i[2];
        }

        std::vector<VectorType> m_vertices;
        std::vector<Edge> m_edges;
        std::vector<Face> m_faces;
    };

    template <Scalar T> using Mesh2 = Mesh<T, 2>;
    template <Scalar T> using Mesh3 = Mesh<T, 3>;

    using Mesh2f = Mesh2<float>;
    using Mesh2d = Mesh2<double>;
    using Mesh3f = Mesh3<float>;
    using Mesh3d = Mesh3<double>;

} // namespace geometry