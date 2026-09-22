#pragma once

#include "algebra.hpp"

#include <span>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <ostream>

namespace geometry
{

    /**
     * @brief Vetor matemático baseado em algebra.hpp
     */
    template <Scalar T, std::size_t N>
    class AlgebricVector
    {
    public:
        using ValueType = T;
        using Storage = LinearStorage<T, N>;

        Storage data;

        /* ================= CONSTRUTORES ================= */

        constexpr AlgebricVector()
        {
            if constexpr (N != 0)
                data.fill(T{});
        }

        explicit AlgebricVector(std::size_t size)
            requires(N == 0)
            : data(size, T{})
        {
        }

        // Nota: o tamanho da initializer_list só é checado em runtime (throw),
        // mesmo N sendo conhecido em compile-time — limitação inerente de
        // std::initializer_list em C++. Ver construtor variádico abaixo para
        // uma alternativa checada em compile-time quando N != 0.
        AlgebricVector(std::initializer_list<T> init)
        {
            if constexpr (N == 0)
            {
                data.assign(init.begin(), init.end());
            }
            else
            {
                if (init.size() != N)
                    throw std::invalid_argument("Invalid dimension");
                std::copy(init.begin(), init.end(), data.begin());
            }
        }

        // Alternativa com checagem de tamanho em compile-time — mas só é
        // escolhida pelo compilador quando chamada com parênteses, ex.:
        // Vector<float,3> v(1.f, 2.f, 3.f). Com chaves, `{1.f,2.f,3.f}` o
        // overload resolution ainda prefere o initializer_list acima (regra
        // padrão de list-initialization), então isso não substitui o de cima,
        // só oferece uma via extra mais segura pra quem quiser.
        template <typename... Args>
            requires(N != 0 && sizeof...(Args) == N && (std::convertible_to<Args, T> && ...))
        constexpr AlgebricVector(Args &&...args)
        {
            T tmp[N] = {static_cast<T>(args)...};
            std::copy(std::begin(tmp), std::end(tmp), data.begin());
        }

        explicit AlgebricVector(const Storage &storage)
            : data(storage) {}

        /* ================= ACESSO ================= */

        [[nodiscard]] constexpr std::size_t size() const noexcept
        {
            return geometry::getSize<T, N>(data);
        }

        constexpr T *data_ptr() noexcept { return data.data(); }
        constexpr const T *data_ptr() const noexcept { return data.data(); }

        constexpr T &operator[](std::size_t i) { return data[i]; }
        constexpr const T &operator[](std::size_t i) const { return data[i]; }

        constexpr std::span<T> span() noexcept { return {data.data(), size()}; }
        constexpr std::span<const T> span() const noexcept { return {data.data(), size()}; }

        /* ================= OPERADORES ================= */

        [[nodiscard]] AlgebricVector operator+(const AlgebricVector &rhs) const
        {
            return AlgebricVector{geometry::operator+ <T, N>(data, rhs.data)};
        }

        [[nodiscard]] AlgebricVector operator-(const AlgebricVector &rhs) const
        {
            return AlgebricVector{geometry::operator- <T, N>(data, rhs.data)};
        }

        [[nodiscard]] AlgebricVector operator*(const AlgebricVector &rhs) const
        {
            return AlgebricVector{geometry::operator* <T, N>(data, rhs.data)};
        }

        [[nodiscard]] AlgebricVector operator*(T scalar) const
        {
            return AlgebricVector{geometry::mul<T, N>(data, scalar)};
        }

        [[nodiscard]] AlgebricVector operator/(T scalar) const
        {
            AlgebricVector result = *this;
            for (auto &x : result.data)
                x /= scalar;
            return result;
        }

        AlgebricVector &operator+=(const AlgebricVector &rhs)
        {
            geometry::operator+= <T, N>(data, rhs.data);
            return *this;
        }

        AlgebricVector &operator-=(const AlgebricVector &rhs)
        {
            geometry::operator-= <T, N>(data, rhs.data);
            return *this;
        }

        AlgebricVector &operator*=(T scalar)
        {
            for (auto &x : data)
                x *= scalar;
            return *this;
        }

        AlgebricVector &operator/=(T scalar)
        {
            for (auto &x : data)
                x /= scalar;
            return *this;
        }

        /* ================= ÁLGEBRA ================= */

        [[nodiscard]] T dot(const AlgebricVector &rhs) const
        {
            return geometry::dot<T, N>(data, rhs.data);
        }

        template <std::size_t M = N>
            requires(M == 2)
        [[nodiscard]] T cross(const AlgebricVector &rhs) const
        {
            return data[0] * rhs.data[1] - data[1] * rhs.data[0];
        }

        template <std::size_t M = N>
            requires(M == 3)
        [[nodiscard]] AlgebricVector cross(const AlgebricVector &rhs) const
        {
            return AlgebricVector{geometry::cross<T, 3>(data, rhs.data)};
        }

        [[nodiscard]] T sqrNorm() const
        {
            return geometry::sqrLength<T, N>(data);
        }

        [[nodiscard]] T norm() const
            requires NormalizableScalar<T>
        {
            return geometry::length<T, N>(data);
        }

        [[nodiscard]] AlgebricVector normalized() const
            requires NormalizableScalar<T>
        {
            return AlgebricVector{geometry::normalize<T, N>(data)};
        }

        [[nodiscard]] AlgebricVector projectOnto(const AlgebricVector &normal) const
        {
            return AlgebricVector{geometry::project<T, N>(data, normal.data)};
        }

        [[nodiscard]] AlgebricVector reflect(const AlgebricVector &normal) const
        {
            return AlgebricVector{geometry::reflect<T, N>(data, normal.data)};
        }

        /* ================= ROTAÇÃO / QUATÉRNIONS ================= */

        // Unificado M==3 (rotaciona um vetor 3D por um quatérnio) e M==4
        // (mesma chamada, aplicada a um Vector<T,4>) — corpo era idêntico nas
        // duas versões antes. TODO confirmar com quem escreveu algebra.hpp:
        // o caso M==4 é redundante com hamilton()?
        template <std::size_t M = N>
            requires(M == 3 || M == 4)
        [[nodiscard]] AlgebricVector rotated(const AlgebricVector<T, 4> &q) const
        {
            return AlgebricVector{geometry::rotate<T>(data, q.data)};
        }

        template <std::size_t M = N>
            requires(M == 4)
        [[nodiscard]] AlgebricVector hamilton(const AlgebricVector<T, 4> &q) const
        {
            return AlgebricVector{geometry::hamilton<T>(data, q.data)};
        }

        // TODO: confirmar semântica — conjugado costuma ser unário (nega a
        // parte imaginária de *this). Esse `q` extra é copy-paste de
        // hamilton/rotated, ou é de fato o "sandwich product" q * this * q⁻¹
        // (que também é chamado de conjugação no contexto de rotação)? Se for
        // isso, provavelmente devia existir também para M==3, não só M==4.
        template <std::size_t M = N>
            requires(M == 4)
        [[nodiscard]] AlgebricVector conjugated(const AlgebricVector<T, 4> &q) const
        {
            return AlgebricVector{geometry::conjugate<T>(data, q.data)};
        }

        /* ================= ITERADORES ================= */

        auto begin() noexcept { return data.begin(); }
        auto end() noexcept { return data.end(); }
        auto begin() const noexcept { return data.begin(); }
        auto end() const noexcept { return data.end(); }

        /* ================= COMPARAÇÃO ================= */

        [[nodiscard]] bool operator==(const AlgebricVector &) const = default;

        /* ================= I/O ================= */

        friend std::ostream &operator<<(std::ostream &os, const AlgebricVector &v)
        {
            os << "[";
            for (std::size_t i = 0; i < v.size(); ++i)
            {
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
    [[nodiscard]] AlgebricVector<T, N> operator*(T scalar, const AlgebricVector<T, N> &v)
    {
        return v * scalar;
    }

    /* ================= ALIASES ================= */

    template <Scalar T>
    using Vec2 = AlgebricVector<T, 2>;
    template <Scalar T>
    using Vec3 = AlgebricVector<T, 3>;
    template <Scalar T>
    using Quat = AlgebricVector<T, 4>;

    using Vec2f = Vec2<float>;
    using Vec2d = Vec2<double>;
    using Vec3f = Vec3<float>;
    using Vec3d = Vec3<double>;
    using Quatf = Quat<float>;
    using Quatd = Quat<double>;

} // namespace geometry