#pragma once

#include "algebra.hpp"

#include <algorithm>
#include <initializer_list>
#include <ostream>
#include <span>
#include <stdexcept>

namespace geometry
{
    /**
     * @brief Vetor matemático (N == 0 -> tamanho dinâmico).
     * Quatérnions seguem a convenção (x, y, z, w).
     */
    template <Scalar T, std::size_t N>
    class AlgebricVector
    {
    public:
        using ValueType = T;
        using Storage = LinearStorage<T, N>;

        Storage data;

        /* ================= CONSTRUTORES ================= */

        constexpr AlgebricVector() : data{} {}

        explicit AlgebricVector(std::size_t size)
            requires(N == 0)
            : data(size, T{}) {}

        // Tamanho checado só em runtime (limitação do initializer_list).
        AlgebricVector(std::initializer_list<T> init)
        {
            if constexpr (N == 0)
                data.assign(init.begin(), init.end());
            else
            {
                if (init.size() != N)
                    throw std::invalid_argument("Invalid dimension");
                std::copy(init.begin(), init.end(), data.begin());
            }
        }

        // Alternativa com tamanho checado em compile-time: Vec3f v(1.f, 2.f, 3.f)
        template <typename... Args>
            requires(N != 0 && sizeof...(Args) == N && (std::convertible_to<Args, T> && ...))
        constexpr AlgebricVector(Args &&...args) : data{static_cast<T>(args)...} {}

        explicit AlgebricVector(const Storage &storage) : data(storage) {}

        /* ================= ACESSO ================= */

        [[nodiscard]] constexpr std::size_t size() const noexcept { return data.size(); }

        constexpr T *data_ptr() noexcept { return data.data(); }
        constexpr const T *data_ptr() const noexcept { return data.data(); }

        constexpr T &operator[](std::size_t i) { return data[i]; }
        constexpr const T &operator[](std::size_t i) const { return data[i]; }

        constexpr std::span<T> span() noexcept { return {data.data(), size()}; }
        constexpr std::span<const T> span() const noexcept { return {data.data(), size()}; }

        auto begin() noexcept { return data.begin(); }
        auto end() noexcept { return data.end(); }
        auto begin() const noexcept { return data.begin(); }
        auto end() const noexcept { return data.end(); }

        /* ================= OPERADORES ================= */

        [[nodiscard]] AlgebricVector operator+(const AlgebricVector &r) const { return AlgebricVector(add<T, N>(data, r.data)); }
        [[nodiscard]] AlgebricVector operator-(const AlgebricVector &r) const { return AlgebricVector(sub<T, N>(data, r.data)); }
        [[nodiscard]] AlgebricVector operator*(const AlgebricVector &r) const { return AlgebricVector(mul<T, N>(data, r.data)); }
        [[nodiscard]] AlgebricVector operator*(T s) const { return AlgebricVector(scale<T, N>(data, s)); }

        [[nodiscard]] AlgebricVector operator/(T s) const
        {
            AlgebricVector out = *this;
            out /= s;
            return out;
        }

        AlgebricVector &operator+=(const AlgebricVector &r) { return *this = *this + r; }
        AlgebricVector &operator-=(const AlgebricVector &r) { return *this = *this - r; }

        AlgebricVector &operator*=(T s)
        {
            for (auto &x : data)
                x = x * s;
            return *this;
        }

        AlgebricVector &operator/=(T s)
        {
            for (auto &x : data)
                x = x / s;
            return *this;
        }

        [[nodiscard]] bool operator==(const AlgebricVector &) const = default;

        /* ================= ÁLGEBRA ================= */

        [[nodiscard]] T dot(const AlgebricVector &r) const { return geometry::dot<T, N>(data, r.data); }

        /* ================= GEOMETRIA ================= */

        [[nodiscard]] T sqrNorm() const { return dot(*this); }

        [[nodiscard]] T norm() const
            requires NormalizableScalar<T>
        {
            return std::sqrt(sqrNorm());
        }

        [[nodiscard]] AlgebricVector normalized() const
            requires NormalizableScalar<T>
        {
            const T sql = sqrNorm();
            if (sql <= T{})
                throw std::runtime_error("Zero length vector cannot be normalized");
            return *this * (T{1} / std::sqrt(sql));
        }

        template <std::size_t M = N>
            requires(M == 2)
        [[nodiscard]] T cross(const AlgebricVector &r) const
        {
            return data[0] * r[1] - data[1] * r[0];
        }

        template <std::size_t M = N>
            requires(M == 3)
        [[nodiscard]] AlgebricVector cross(const AlgebricVector &r) const
        {
            return AlgebricVector(data[1] * r[2] - data[2] * r[1],
                                  data[2] * r[0] - data[0] * r[2],
                                  data[0] * r[1] - data[1] * r[0]);
        }

        // `unit` deve estar normalizado
        [[nodiscard]] AlgebricVector projectOnto(const AlgebricVector &unit) const
        {
            return unit * dot(unit);
        }

        [[nodiscard]] AlgebricVector reflect(const AlgebricVector &unit) const
        {
            return *this - unit * (T{2} * dot(unit));
        }

        /* ================= QUATÉRNIONS (x, y, z, w) ================= */

        // Rotaciona (x,y,z) por um quatérnio unitário; em M == 4 mantém o w.
        template <std::size_t M = N>
            requires(M == 3 || M == 4)
        [[nodiscard]] AlgebricVector rotated(const AlgebricVector<T, 4> &q) const
        {
            const T tx = T{2} * (q[1] * data[2] - q[2] * data[1]);
            const T ty = T{2} * (q[2] * data[0] - q[0] * data[2]);
            const T tz = T{2} * (q[0] * data[1] - q[1] * data[0]);

            AlgebricVector out = *this;
            out[0] = data[0] + q[3] * tx + (q[1] * tz - q[2] * ty);
            out[1] = data[1] + q[3] * ty + (q[2] * tx - q[0] * tz);
            out[2] = data[2] + q[3] * tz + (q[0] * ty - q[1] * tx);
            return out;
        }

        // Produto de Hamilton: *this * q
        template <std::size_t M = N>
            requires(M == 4)
        [[nodiscard]] AlgebricVector hamilton(const AlgebricVector &q) const
        {
            const auto &a = data;
            return AlgebricVector(a[3] * q[0] + a[0] * q[3] + a[1] * q[2] - a[2] * q[1],
                                  a[3] * q[1] - a[0] * q[2] + a[1] * q[3] + a[2] * q[0],
                                  a[3] * q[2] + a[0] * q[1] - a[1] * q[0] + a[2] * q[3],
                                  a[3] * q[3] - a[0] * q[0] - a[1] * q[1] - a[2] * q[2]);
        }

        // Conjugado unário: nega a parte imaginária.
        template <std::size_t M = N>
            requires(M == 4)
        [[nodiscard]] AlgebricVector conjugated() const
        {
            return AlgebricVector(-data[0], -data[1], -data[2], data[3]);
        }

        /* ================= I/O ================= */

        friend std::ostream &operator<<(std::ostream &os, const AlgebricVector &v)
        {
            os << "[";
            for (std::size_t i = 0; i < v.size(); ++i)
                os << (i ? ", " : "") << v[i];
            return os << "]";
        }
    };

    template <Scalar T, std::size_t N>
    [[nodiscard]] AlgebricVector<T, N> operator*(T scalar, const AlgebricVector<T, N> &v)
    {
        return v * scalar;
    }

    /* ================= ALIASES ================= */

    template <Scalar T> using Vec2 = AlgebricVector<T, 2>;
    template <Scalar T> using Vec3 = AlgebricVector<T, 3>;
    template <Scalar T> using Quat = AlgebricVector<T, 4>;

    using Vec2f = Vec2<float>;
    using Vec2d = Vec2<double>;
    using Vec3f = Vec3<float>;
    using Vec3d = Vec3<double>;
    using Quatf = Quat<float>;
    using Quatd = Quat<double>;

} // namespace geometry