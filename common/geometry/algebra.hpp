#pragma once

#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <vector>

namespace geometry
{
    /* ================= CONCEITOS ================= */

    template <typename T>
    concept Scalar =
        std::regular<T> &&
        requires(T a, T b) {
            { a + b } -> std::convertible_to<T>;
            { a - b } -> std::convertible_to<T>;
            { a * b } -> std::convertible_to<T>;
            { a / b } -> std::convertible_to<T>;
            { -a } -> std::convertible_to<T>;
            { a < b } -> std::convertible_to<bool>;
        };

    template <typename T>
    concept NormalizableScalar =
        Scalar<T> &&
        requires(T a) {
            { std::sqrt(a) } -> std::convertible_to<T>;
        };

    /* ================= STORAGE ================= */

    // N == 0 -> tamanho dinâmico
    template <typename T, std::size_t N>
    using LinearStorage = std::conditional_t<N == 0, std::vector<T>, std::array<T, N>>;

    /* ================= OPERAÇÕES ALGÉBRICAS ================= */

    namespace detail
    {
        template <Scalar T, std::size_t N, typename F>
        [[nodiscard]] LinearStorage<T, N> zipWith(const LinearStorage<T, N> &a,
                                                  const LinearStorage<T, N> &b, F f)
        {
            LinearStorage<T, N> out = a;
            for (std::size_t i = 0; i < out.size(); ++i)
                out[i] = f(a[i], b[i]);
            return out;
        }
    }

    template <Scalar T, std::size_t N>
    [[nodiscard]] LinearStorage<T, N> add(const LinearStorage<T, N> &a, const LinearStorage<T, N> &b)
    {
        return detail::zipWith<T, N>(a, b, [](const T &x, const T &y) { return x + y; });
    }

    template <Scalar T, std::size_t N>
    [[nodiscard]] LinearStorage<T, N> sub(const LinearStorage<T, N> &a, const LinearStorage<T, N> &b)
    {
        return detail::zipWith<T, N>(a, b, [](const T &x, const T &y) { return x - y; });
    }

    // Produto elemento a elemento
    template <Scalar T, std::size_t N>
    [[nodiscard]] LinearStorage<T, N> mul(const LinearStorage<T, N> &a, const LinearStorage<T, N> &b)
    {
        return detail::zipWith<T, N>(a, b, [](const T &x, const T &y) { return x * y; });
    }

    template <Scalar T, std::size_t N>
    [[nodiscard]] LinearStorage<T, N> scale(const LinearStorage<T, N> &v, T s)
    {
        LinearStorage<T, N> out = v;
        for (auto &x : out)
            x = x * s;
        return out;
    }

    template <Scalar T, std::size_t N>
    [[nodiscard]] T dot(const LinearStorage<T, N> &a, const LinearStorage<T, N> &b)
    {
        T res{};
        for (std::size_t i = 0; i < a.size(); ++i)
            res = res + a[i] * b[i];
        return res;
    }

} // namespace geometry