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

/* ================= TRIANGLE ================= */

/* ================= MESH ================= */

template <Scalar T, std::size_t N>
class Mesh {
public:

    using ValueType  = T;
    using PointType  = Point<T, N>;
    using VectorType = Vector<T, N>;

private:

    std::vector<PointType> vertices;
    std::vector<std::array<std::size_t, 3>> faces;
    std::vector<std::array<std::size_t, 4>> tetrahedrons;

public:

    /* ================= CONSTRUTORES ================= */

    Mesh() = default;

    Mesh(std::vector<PointType> verts)
        : vertices(std::move(verts)) {}

    Mesh(std::vector<PointType> verts, std::vector<std::array<std::size_t, 3>> faces)
        : vertices(std::move(verts)),
          faces(std::move(faces)) {}

    Mesh(std::vector<PointType> verts, std::vector<std::array<std::size_t, 4>> tetrahedrons)
        : vertices(std::move(verts)),
          tetrahedrons(std::move(tetrahedrons)) {}

    Mesh(std::vector<PointType> verts, std::vector<std::array<std::size_t, 3>> faces, std::vector<std::array<std::size_t, 4>> tetrahedrons)
        : vertices(std::move(verts)),
          faces(std::move(faces)),
          tetrahedrons(std::move(tetrahedrons)) {}

    /* ================= DADOS ================= */

    [[nodiscard]]
    std::size_t vertexCount() const noexcept {
        return vertices.size();
    }

    [[nodiscard]]
    std::size_t faceCount() const noexcept {
        return faces.size();
    }

    [[nodiscard]]
    const std::vector<PointType>&
    getVertices() const noexcept {
        return vertices;
    }

    [[nodiscard]]
    const std::vector<std::array<std::size_t, 3>>&
    getFaces() const noexcept {
        return faces;
    }

    [[nodiscard]]
    const std::vector<std::array<std::size_t, 4>>&
    getTetrahedrons() const noexcept {
        return tetrahedrons;
    }

    [[nodiscard]]
    std::vector<PointType>&
    getVertices() noexcept {
        return vertices;
    }

    [[nodiscard]]
    std::vector<std::array<std::size_t, 3>>&
    getFaces() noexcept {
        return faces;
    }

    [[nodiscard]]
    std::vector<std::array<std::size_t, 4>>&
    getTetrahedrons() noexcept {
        return tetrahedrons;
    }

    void clear() {

        vertices.clear();
        faces.clear();
    }

    /* ================= ADIÇÃO ================= */

    [[nodiscard]]
    std::size_t addVertex(const PointType& p) {

        vertices.push_back(p);

        return vertices.size() - 1;
    }

    void addFace(std::size_t i0, std::size_t i1, std::size_t i2) {

#ifndef NDEBUG

        const auto sz = vertices.size();

        if (i0 >= sz || i1 >= sz || i2 >= sz)
            throw std::out_of_range(
                "Mesh face index out of bounds"
            );

#endif

        faces.push_back({
            i0,
            i1,
            i2
        });
    }

    void addTetrahedron(std::size_t i0, std::size_t i1, std::size_t i2, std::size_t i3) {

#ifndef NDEBUG

        const auto sz = vertices.size();

        if (i0 >= sz || i1 >= sz || i2 >= sz || i3 >= sz)
            throw std::out_of_range(
                "Mesh tetrahedron index out of bounds"
            );

#endif

        tetrahedrons.push_back({
            i0,
            i1,
            i2,
            i3
        });
    }

    /* ================= ÁREA ================= */

    [[nodiscard]]
    T faceArea(const std::vector<std::array<std::size_t, 3>>& f) const
    requires NormalizableScalar<T>
    {
        const VectorType ab =
            vertices[f[1]] - vertices[f[0]];

        const VectorType ac =
            vertices[f[2]] - vertices[f[0]];

        if constexpr (N == 2) {

            return
                std::abs(ab.cross(ac))
                *
                T{0.5};

        } else if constexpr (N == 3) {

            return
                ab.cross(ac).norm()
                *
                T{0.5};

        } else {

            const T dot_ab =
                ab.dot(ab);

            const T dot_ac =
                ac.dot(ac);

            const T dot_ab_ac =
                ab.dot(ac);

            const T det =
                dot_ab * dot_ac
                -
                dot_ab_ac * dot_ab_ac;

            using std::sqrt;

            return
                sqrt(std::max(T{}, det))
                *
                T{0.5};
        }
    }

    [[nodiscard]]
    T totalArea() const
    requires NormalizableScalar<T>
    {
        T area{};

        for (const auto& face : faces) {

            area =
                area
                +
                faceArea(face);
        }

        return area;
    }

    /* ================= CENTROIDE ================= */

    [[nodiscard]]
    PointType centroid() const {

        if (vertices.empty()) {

            if constexpr (N == 0)
                return PointType{0};

            return PointType{};
        }

        VectorType accum;

        if constexpr (N == 0) {
            accum = VectorType(vertices[0].size());
        }

        for (const auto& v : vertices) {

            accum += v.to_vector();
        }

        accum /= T(vertices.size());

        return PointType{accum};
    }

    /* ================= TRANSFORMAÇÕES ================= */

    void translate(const VectorType& offset) {

        for (auto& v : vertices) {
            v += offset;
        }
    }

    void scale(const T& factor) {

        const PointType c =
            centroid();

        for (auto& v : vertices) {

            const VectorType dir =
                v - c;

            v =
                c
                +
                (dir * factor);
        }
    }

    template<std::size_t M = N>
    requires (M == 3)
    void rotate(const Vector<T, 4>& q) {

        const PointType c =
            centroid();

        for (auto& v : vertices) {

            VectorType dir =
                v - c;

            dir =
                dir.rotated(q);

            v =
                c + dir;
        }
    }

    /* ================= BOUNDING BOX ================= */

    [[nodiscard]]
    std::pair<PointType, PointType>
    boundingBox() const {

        if (vertices.empty()) {
            return {};
        }

        PointType min_p =
            vertices.front();

        PointType max_p =
            vertices.front();

        for (const auto& v : vertices) {

            for (std::size_t i = 0;
                 i < v.size();
                 ++i) {

                min_p[i] =
                    std::min(
                        min_p[i],
                        v[i]
                    );

                max_p[i] =
                    std::max(
                        max_p[i],
                        v[i]
                    );
            }
        }

        return {
            min_p,
            max_p
        };
    }

    /* ================= RESERVA ================= */

    void reserveVertices(std::size_t n) {
        vertices.reserve(n);
    }

    void reserveFaces(std::size_t n) {
        faces.reserve(n);
    }

    /* ================= VALIDAÇÃO ================= */

    [[nodiscard]]
    bool empty() const noexcept {

        return vertices.empty();
    }

    [[nodiscard]]
    bool valid() const noexcept {

        const auto sz =
            vertices.size();

        for (const auto& f : faces) {

            if (f[0] >= sz ||
                f[1] >= sz ||
                f[2] >= sz) {

                return false;
            }
        }

        return true;
    }

    /* ================= I/O ================= */

    friend std::ostream& operator<<(
        std::ostream& os,
        const Mesh& mesh
    ) {

        os
            << "Mesh{ V: "
            << mesh.vertices.size()
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