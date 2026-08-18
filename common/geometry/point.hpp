#pragma once

#include "arithmetic.hpp"
#include "vector.hpp"

#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <ostream>
#include <cmath>

namespace geometry {

/**
 * @brief Ponto geométrico.
 */
template <Scalar T, std::size_t N>
class Point {
public:

    using ValueType  = T;
    using VectorType = Vector<T, N>;
    using Storage    = LinearStorage<T, N>;

    Storage data;

    /* ================= CONSTRUTORES ================= */

    constexpr Point() {

        if constexpr (N != 0) {
            data.fill(T{});
        }
    }

    explicit Point(std::size_t size)
    requires (N == 0)
        : data(size, T{}) {}

    Point(std::initializer_list<T> init) {

        if constexpr (N == 0) {

            data.assign(init.begin(), init.end());

        } else {

            if (init.size() != N)
                throw std::invalid_argument("Invalid dimension");

            std::copy(init.begin(), init.end(), data.begin());
        }
    }

    explicit Point(const Storage& storage)
        : data(storage) {}

    explicit Point(const VectorType& vec)
        : data(vec.data) {}

    /* ================= ACESSO ================= */

    [[nodiscard]]
    constexpr std::size_t size() const noexcept {

        return geometry::getSize<T, N>(data);
    }

    constexpr T* data_ptr() noexcept {
        return data.data();
    }

    constexpr const T* data_ptr() const noexcept {
        return data.data();
    }

    constexpr T& operator[](std::size_t i) {
        return data[i];
    }

    constexpr const T& operator[](std::size_t i) const {
        return data[i];
    }

    /* ================= ÁLGEBRA ================= */

    [[nodiscard]]
    Point operator+(const VectorType& vec) const {

        return Point{
            geometry::operator+<T, N>(
                data,
                vec.data
            )
        };
    }

    [[nodiscard]]
    Point operator-(const VectorType& vec) const {

        return Point{
            geometry::operator-<T, N>(
                data,
                vec.data
            )
        };
    }

    [[nodiscard]]
    VectorType operator-(const Point& other) const {

        return VectorType{
            geometry::operator-<T, N>(
                data,
                other.data
            )
        };
    }

    Point& operator+=(const VectorType& vec) {

        geometry::operator+=<T, N>(
            data,
            vec.data
        );

        return *this;
    }

    Point& operator-=(const VectorType& vec) {

        geometry::operator-=<T, N>(
            data,
            vec.data
        );

        return *this;
    }

    /* ================= DISTÂNCIA ================= */

    [[nodiscard]]
    T squared_distance_to(const Point& other) const {

        return geometry::sqrLength<T, N>(
            geometry::operator-<T, N>(
                data,
                other.data
            )
        );
    }

    [[nodiscard]]
    T distance_to(const Point& other) const
    requires NormalizableScalar<T>
    {
        return std::sqrt(
            squared_distance_to(other)
        );
    }

    [[nodiscard]]
    T manhattan_distance(const Point& other) const {

        T sum{};

        for (std::size_t i = 0; i < size(); ++i) {

            using std::abs;

            sum = sum + abs(
                data[i] - other.data[i]
            );
        }

        return sum;
    }

    /* ================= INTERPOLAÇÃO ================= */

    [[nodiscard]]
    Point lerp(const Point& other,
               const T& t) const {

        return (*this)
            +
            ((other - (*this)) * t);
    }

    [[nodiscard]]
    Point midpoint(const Point& other) const {

        return lerp(other, T{0.5});
    }

    /* ================= CONVERSÃO ================= */

    [[nodiscard]]
    VectorType to_vector() const {

        return VectorType{ data };
    }

    /* ================= COMPARAÇÃO ================= */

    [[nodiscard]]
    bool operator==(const Point&) const = default;

    /* ================= I/O ================= */

    friend std::ostream& operator<<(
        std::ostream& os,
        const Point& p
    ) {

        os << "P[";

        for (std::size_t i = 0; i < p.size(); ++i) {

            os << p.data[i];

            if (i + 1 < p.size())
                os << ", ";
        }

        os << "]";

        return os;
    }
};

/* ================= ALIASES ================= */

template <Scalar T>
using Point2 = Point<T, 2>;

template <Scalar T>
using Point3 = Point<T, 3>;

using Point2f = Point2<float>;
using Point2d = Point2<double>;

using Point3f = Point3<float>;
using Point3d = Point3<double>;

} // namespace geometry