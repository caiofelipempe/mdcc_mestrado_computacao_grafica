#pragma once

#include <vector>
#include <array>
#include <utility>
#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <cmath>

#include "point.hpp"
#include "vector.hpp"

namespace geometry {

struct Edge {
    std::size_t v1;
    std::size_t v2;
};

struct Face {
    std::array<std::size_t, 3> indices;
};

template <Scalar T, std::size_t N>
class Mesh {
public:
    using ValueType  = T;
    using PointType  = Point<T, N>;
    using VectorType = Vector<T, N>;

private:
    std::vector<PointType> vertices;
    std::vector<Edge> edges;
    std::vector<Face> faces;

public:
    Mesh() = default;

    [[nodiscard]]
    const std::vector<PointType>&
    getVertices() const noexcept {
        return vertices;
    }

    [[nodiscard]]
    const std::vector<Edge>&
    getEdges() const noexcept {
        return edges;
    }

    [[nodiscard]]
    const std::vector<Face>&
    getFaces() const noexcept {
        return faces;
    }

    [[nodiscard]]
    std::vector<PointType>&
    getVertices() noexcept {
        return vertices;
    }

    [[nodiscard]]
    std::vector<Edge>&
    getEdges() noexcept {
        return edges;
    }

    [[nodiscard]]
    std::vector<Face>&
    getFaces() noexcept {
        return faces;
    }

    [[nodiscard]]
    std::size_t vertexCount() const noexcept {
        return vertices.size();
    }

    [[nodiscard]]
    std::size_t edgeCount() const noexcept {
        return edges.size();
    }

    [[nodiscard]]
    std::size_t faceCount() const noexcept {
        return faces.size();
    }

    [[nodiscard]]
    std::size_t addVertex(
        const PointType& point)
    {
        vertices.push_back(point);

        return vertices.size() - 1;
    }

    void addEdge(
        std::size_t v1,
        std::size_t v2)
    {
#ifndef NDEBUG
        const auto sz = vertices.size();

        if (v1 >= sz || v2 >= sz) {
            throw std::out_of_range(
                "Mesh edge index out of bounds"
            );
        }
#endif

        edges.push_back(
            Edge{
                v1,
                v2
            }
        );
    }

    void addFace(
        std::size_t i0,
        std::size_t i1,
        std::size_t i2)
    {
#ifndef NDEBUG
        const auto sz = vertices.size();

        if (i0 >= sz ||
            i1 >= sz ||
            i2 >= sz)
        {
            throw std::out_of_range(
                "Mesh face index out of bounds"
            );
        }
#endif

        faces.push_back(
            Face{
                {i0, i1, i2}
            }
        );
    }

    void clear() {
        vertices.clear();
        edges.clear();
        faces.clear();
    }

    [[nodiscard]]
    bool empty() const noexcept {
        return vertices.empty();
    }

    [[nodiscard]]
    bool valid() const noexcept {
        const auto sz =
            vertices.size();

        for (const auto& edge : edges) {

            if (edge.v1 >= sz ||
                edge.v2 >= sz)
            {
                return false;
            }
        }

        for (const auto& face : faces) {

            if (face.indices[0] >= sz ||
                face.indices[1] >= sz ||
                face.indices[2] >= sz)
            {
                return false;
            }
        }

        return true;
    }

    friend std::ostream& operator<<(
        std::ostream& os,
        const Mesh& mesh)
    {
        os
            << "Mesh{ V: "
            << mesh.vertices.size()
            << ", E: "
            << mesh.edges.size()
            << ", F: "
            << mesh.faces.size()
            << " }";

        return os;
    }
};

/* ================= ALIASES ================= */

template <Scalar T>
using Mesh2 = Mesh<T, 2>;

template <Scalar T>
using Mesh3 = Mesh<T, 3>;

using Mesh2f = Mesh2<float>;
using Mesh2d = Mesh2<double>;

using Mesh3f = Mesh3<float>;
using Mesh3d = Mesh3<double>;

} // namespace geometry