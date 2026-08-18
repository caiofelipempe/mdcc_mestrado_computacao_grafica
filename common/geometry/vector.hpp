#pragma once

#include "arithmetic.hpp"

#include <span>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <ostream>

namespace geometry {

/**
 * @brief Vetor matemático baseado em arithmetic.hpp
 */
template <Scalar T, std::size_t N>
class Vector {
public:

    using ValueType = T;
    using Storage   = LinearStorage<T, N>;

    Storage data;

    /* ================= CONSTRUTORES ================= */

    constexpr Vector() {

        if constexpr (N != 0) {
            data.fill(T{});
        }
    }

    explicit Vector(std::size_t size)
    requires (N == 0)
        : data(size, T{}) {}

    Vector(std::initializer_list<T> init) {

        if constexpr (N == 0) {

            data.assign(init.begin(), init.end());

        } else {

            if (init.size() != N)
                throw std::invalid_argument("Invalid dimension");

            std::copy(init.begin(), init.end(), data.begin());
        }
    }

    explicit Vector(const Storage& storage)
        : data(storage) {}

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

    constexpr std::span<T> span() noexcept {
        return { data.data(), size() };
    }

    constexpr std::span<const T> span() const noexcept {
        return { data.data(), size() };
    }

    /* ================= OPERADORES ================= */

    [[nodiscard]]
    Vector operator+(const Vector& rhs) const {

        return Vector{
            geometry::operator+<T, N>(data, rhs.data)
        };
    }

    [[nodiscard]]
    Vector operator-(const Vector& rhs) const {

        return Vector{
            geometry::operator-<T, N>(data, rhs.data)
        };
    }

    [[nodiscard]]
    Vector operator*(const Vector& rhs) const {

        return Vector{
            geometry::operator*<T, N>(data, rhs.data)
        };
    }

    [[nodiscard]]
    Vector operator*(const T& scalar) const {

        return Vector{
            geometry::mul<T, N>(data, scalar)
        };
    }

    [[nodiscard]]
    Vector operator/(const T& scalar) const {

        return (*this) * (T{1} / scalar);
    }

    Vector& operator+=(const Vector& rhs) {

        geometry::operator+=<T, N>(data, rhs.data);

        return *this;
    }

    Vector& operator-=(const Vector& rhs) {

        geometry::operator-=<T, N>(data, rhs.data);

        return *this;
    }

    Vector& operator*=(const T& scalar) {

        *this = (*this) * scalar;

        return *this;
    }

    Vector& operator/=(const T& scalar) {

        *this = (*this) / scalar;

        return *this;
    }

    /* ================= ÁLGEBRA ================= */

    [[nodiscard]]
    T dot(const Vector& rhs) const {

        return geometry::dot<T, N>(
            data,
            rhs.data
        );
    }

    template<std::size_t M = N>
    requires (M == 2)
    [[nodiscard]]
    T cross(const Vector& rhs) const {

        return
            data[0] * rhs.data[1]
            -
            data[1] * rhs.data[0];
    }

    template<std::size_t M = N>
    requires (M == 3)
    [[nodiscard]]
    Vector cross(const Vector& rhs) const {

        return Vector{
            geometry::cross<T, 3>(
                data,
                rhs.data
            )
        };
    }

    [[nodiscard]]
    T sqrNorm() const {

        return geometry::sqrLength<T, N>(data);
    }

    [[nodiscard]]
    T norm() const
    requires NormalizableScalar<T>
    {
        return geometry::length<T, N>(data);
    }

    [[nodiscard]]
    Vector normalized() const
    requires NormalizableScalar<T>
    {
        return Vector{
            geometry::normalize<T, N>(data)
        };
    }

    [[nodiscard]]
    Vector projectOnto(const Vector& normal) const {

        return Vector{
            geometry::project<T, N>(
                data,
                normal.data
            )
        };
    }

    [[nodiscard]]
    Vector reflect(const Vector& normal) const {

        return Vector{
            geometry::reflect<T, N>(
                data,
                normal.data
            )
        };
    }

    /* ================= ROTAÇÃO ================= */

    template<std::size_t M = N>
    requires (M == 3)
    [[nodiscard]]
    Vector rotated(const Vector<T, 4>& q) const {

        return Vector{
            geometry::rotate<T>(
                data,
                q.data
            )
        };
    }

    template<std::size_t M = N>
    requires (M == 4)
    [[nodiscard]]
    Vector rotated(const Vector<T,4>& q) const {

        return Vector{
            geometry::rotate<T>(
                data,
                q.data
            )
        };
    }

    template<std::size_t M = N>
    requires (M == 4)
    [[nodiscard]]
    Vector hamilton(const Vector<T,4>& q) const {

        return Vector{
            geometry::hamilton<T>(
                data,
                q.data
            )
        };
    }

    template<std::size_t M = N>
    requires (M == 4)
    [[nodiscard]]
    Vector conjugated(const Vector<T,4>& q) const {

        return Vector{
            geometry::conjugate<T>(
                data,
                q.data
            )
        };
    }

    /* ================= ITERADORES ================= */

    auto begin() noexcept { return data.begin(); }
    auto end() noexcept { return data.end(); }

    auto begin() const noexcept { return data.begin(); }
    auto end() const noexcept { return data.end(); }

    /* ================= COMPARAÇÃO ================= */

    [[nodiscard]]
    bool operator==(const Vector&) const = default;

    /* ================= I/O ================= */

    friend std::ostream& operator<<(
        std::ostream& os,
        const Vector& v
    ) {

        os << "[";

        for (std::size_t i = 0; i < v.size(); ++i) {

            os << v.data[i];

            if (i + 1 < v.size())
                os << ", ";
        }

        os << "]";

        return os;
    }
};

/* ================= OPERADORES GLOBAIS ================= */

template <Scalar T, std::size_t N>
[[nodiscard]]
Vector<T, N> operator*(
    const T& scalar,
    const Vector<T, N>& v
) {
    return v * scalar;
}

/* ================= ALIASES ================= */

template <Scalar T>
using Vec2 = Vector<T, 2>;

template <Scalar T>
using Vec3 = Vector<T, 3>;

template <Scalar T>
using Quat = Vector<T, 4>;

using Vec2f = Vec2<float>;
using Vec2d = Vec2<double>;

using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;

using Quatf = Quat<float>;
using Quatd = Quat<double>;

} // namespace geometry