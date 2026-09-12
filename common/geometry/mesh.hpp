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

    [[nodiscard]] bool operator==(const Edge&) const = default;
};

struct Face {
    std::array<std::size_t, 3> indices;

    // Igualdade independente de ordem/winding: a mesma face triangular
    // pode aparecer com vértices em ordens diferentes vindo de
    // tetraedros/triângulos vizinhos (comum ao comparar faces de
    // fronteira durante retriangulação tipo Bowyer-Watson). Comparação
    // ingênua campo-a-campo (indices == indices) não pegaria isso.
    [[nodiscard]] bool operator==(const Face& other) const {
        auto a = indices;
        auto b = other.indices;
        std::ranges::sort(a);
        std::ranges::sort(b);
        return a == b;
    }
};

template <Scalar T, std::size_t N>
class Mesh {
public:
    using ValueType  = T;
    using PointType  = Point<T, N>;
    using VectorType = Vector<T, N>;

private:
    std::vector<PointType> m_vertices;
    std::vector<Edge>      m_edges;
    std::vector<Face>      m_faces;

    void checkVertexIndex(std::size_t i) const {
        if (i >= m_vertices.size())
            throw std::out_of_range("Mesh: vertex index out of bounds");
    }

public:
    Mesh() = default;

    [[nodiscard]] const std::vector<PointType>& getVertices() const noexcept { return m_vertices; }
    [[nodiscard]] const std::vector<Edge>&      getEdges()    const noexcept { return m_edges; }
    [[nodiscard]] const std::vector<Face>&      getFaces()    const noexcept { return m_faces; }

    [[nodiscard]] std::vector<PointType>& getVertices() noexcept { return m_vertices; }
    [[nodiscard]] std::vector<Edge>&      getEdges()    noexcept { return m_edges; }
    [[nodiscard]] std::vector<Face>&      getFaces()    noexcept { return m_faces; }

    [[nodiscard]] std::size_t vertexCount() const noexcept { return m_vertices.size(); }
    [[nodiscard]] std::size_t edgeCount()   const noexcept { return m_edges.size(); }
    [[nodiscard]] std::size_t faceCount()   const noexcept { return m_faces.size(); }

    [[nodiscard]] std::size_t addVertex(const PointType& point) {
        m_vertices.push_back(point);
        return m_vertices.size() - 1;
    }

    // Validação sempre ativa (não depende de NDEBUG): o custo é uma
    // comparação de inteiros, irrelevante frente ao push_back, e pegar
    // um índice inválido aqui — no ponto de inserção — é muito mais
    // barato de depurar do que deixar a malha corromper silenciosamente
    // e só descobrir o problema no consumidor (ex. drawMesh indexando
    // fora dos limites) ou numa contagem de elementos que só "parece"
    // errada no final.
    void addEdge(std::size_t v1, std::size_t v2) {
        checkVertexIndex(v1);
        checkVertexIndex(v2);

        if (v1 == v2)
            throw std::invalid_argument("Mesh: degenerate edge (v1 == v2)");

        m_edges.push_back(Edge{v1, v2});
    }

    void addFace(std::size_t i0, std::size_t i1, std::size_t i2) {
        checkVertexIndex(i0);
        checkVertexIndex(i1);
        checkVertexIndex(i2);

        if (i0 == i1 || i1 == i2 || i0 == i2)
            throw std::invalid_argument("Mesh: degenerate face (repeated index)");

        m_faces.push_back(Face{{i0, i1, i2}});
    }

    void clear() {
        m_vertices.clear();
        m_edges.clear();
        m_faces.clear();
    }

    [[nodiscard]] bool empty() const noexcept { return m_vertices.empty(); }

    // Mantido mesmo com addEdge/addFace agora validando na inserção:
    // getVertices()/getEdges()/getFaces() não-const devolvem referências
    // mutáveis, então nada impede alguém de, por exemplo, encolher
    // vertices depois de faces já apontarem para índices altos — valid()
    // continua sendo a forma de checar o invariante após mutação externa.
    [[nodiscard]] bool valid() const noexcept {
        const auto sz = m_vertices.size();

        for (const auto& edge : m_edges) {
            if (edge.v1 >= sz || edge.v2 >= sz || edge.v1 == edge.v2)
                return false;
        }

        for (const auto& face : m_faces) {
            const auto& idx = face.indices;
            if (idx[0] >= sz || idx[1] >= sz || idx[2] >= sz)
                return false;
            if (idx[0] == idx[1] || idx[1] == idx[2] || idx[0] == idx[2])
                return false;
        }

        return true;
    }

    friend std::ostream& operator<<(std::ostream& os, const Mesh& mesh) {
        os << "Mesh{ V: " << mesh.m_vertices.size()
           << ", E: "     << mesh.m_edges.size()
           << ", F: "     << mesh.m_faces.size()
           << " }";
        return os;
    }
};

/* ================= ALIASES ================= */

template <Scalar T> using Mesh2 = Mesh<T, 2>;
template <Scalar T> using Mesh3 = Mesh<T, 3>;

using Mesh2f = Mesh2<float>;
using Mesh2d = Mesh2<double>;
using Mesh3f = Mesh3<float>;
using Mesh3d = Mesh3<double>;

} // namespace geometry