#pragma once

// ATENÇÃO: As diretivas #pragma GCC target foram removidas daqui
// e devem ser configuradas via CMake (ex: -mavx2 -mfma).

#include <array>
#include <vector>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <concepts>
#include <immintrin.h>
#include <memory>
#include <cstdlib>
#include <new>

namespace geometry
{

    /* ================= FORCE INLINE ================= */

#if defined(__GNUC__) || defined(__clang__)
#define GEOM_FORCE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define GEOM_FORCE_INLINE __forceinline
#else
#define GEOM_FORCE_INLINE inline
#endif

#if defined(__GNUC__) || defined(__clang__)
#define GEOM_RESTRICT __restrict__
#elif defined(_MSC_VER)
#define GEOM_RESTRICT __restrict
#else
#define GEOM_RESTRICT
#endif

    /* ================= CONCEITOS ================= */

    template <typename T>
    concept Addable = requires(T a, T b) {
        { a + b } -> std::convertible_to<T>;
    };

    template <typename T>
    concept Subtractable = requires(T a, T b) {
        { a - b } -> std::convertible_to<T>;
    };

    template <typename T>
    concept Multipliable = requires(T a, T b) {
        { a * b } -> std::convertible_to<T>;
    };

    template <typename T>
    concept Divisible = requires(T a, T b) {
        { a / b } -> std::convertible_to<T>;
    };

    template <typename T>
    concept Negatable = requires(T a) {
        { -a } -> std::convertible_to<T>;
    };

    template <typename T>
    concept Comparable = requires(T a, T b) {
        { a == b } -> std::convertible_to<bool>;
        { a != b } -> std::convertible_to<bool>;
        { a < b } -> std::convertible_to<bool>;
        { a <= b } -> std::convertible_to<bool>;
        { a > b } -> std::convertible_to<bool>;
        { a >= b } -> std::convertible_to<bool>;
    };

    template <typename T>
    concept DefaultConstructible = requires {
        T{};
    };

    template <typename T>
    concept Sqrtable = requires(T a) {
        { std::sqrt(a) } -> std::convertible_to<T>;
    };

    template <typename T>
    concept Scalar =
        Addable<T> &&
        Subtractable<T> &&
        Multipliable<T> &&
        Divisible<T> &&
        Negatable<T> &&
        Comparable<T> &&
        DefaultConstructible<T>;

    template <typename T>
    concept NormalizableScalar =
        Scalar<T> &&
        Sqrtable<T>;

    /* ================= STORAGE ================= */

    template <typename T, std::size_t N>
    struct alignas(32) AlignedArray
    {
        std::array<T, N> _data;

        constexpr T *data() noexcept { return _data.data(); }
        constexpr const T *data() const noexcept { return _data.data(); }

        constexpr void fill(const T &value) noexcept { _data.fill(value); }
        constexpr std::size_t size() const noexcept { return N; }

        constexpr T &operator[](std::size_t i) noexcept { return _data[i]; }
        constexpr const T &operator[](std::size_t i) const noexcept { return _data[i]; }

        auto begin() noexcept { return _data.begin(); }
        auto end() noexcept { return _data.end(); }
        auto begin() const noexcept { return _data.begin(); }
        auto end() const noexcept { return _data.end(); }
    };

    /* ================= ALIGNED ALLOCATOR ================= */

    template <typename T, std::size_t Align = 32>
    struct AlignedAllocator
    {
        using value_type = T;

        AlignedAllocator() = default;

        template <typename U>
        constexpr AlignedAllocator(const AlignedAllocator<U, Align> &) noexcept {}

        [[nodiscard]] T *allocate(std::size_t n)
        {
            void *ptr = nullptr;
            std::size_t bytes = n * sizeof(T);
#if defined(_MSC_VER)
            ptr = _aligned_malloc(bytes, Align);
            if (!ptr)
                throw std::bad_alloc();
#else
            if (posix_memalign(&ptr, Align < sizeof(void *) ? sizeof(void *) : Align, bytes) != 0)
                throw std::bad_alloc();
#endif
            return static_cast<T *>(ptr);
        }

        void deallocate(T *p, std::size_t) noexcept
        {
#if defined(_MSC_VER)
            _aligned_free(p);
#else
            free(p);
#endif
        }
    };

    template <typename T1, typename T2, std::size_t A>
    constexpr bool operator==(const AlignedAllocator<T1, A> &, const AlignedAllocator<T2, A> &) noexcept
    {
        return true;
    }

    template <typename T, std::size_t N>
    using LinearStorage = std::conditional_t<
        N == 0,
        std::vector<T, AlignedAllocator<T, 32>>,
        AlignedArray<T, N>>;

    /* ================= UTILS ================= */

    template <Scalar T, std::size_t N>
    constexpr std::size_t getSize(const LinearStorage<T, N> &v) noexcept
    {
        if constexpr (N == 0)
            return v.size();
        else
            return N;
    }

    template <Scalar T, std::size_t N>
    constexpr auto makeSimilar(const LinearStorage<T, N> &v)
    {
        if constexpr (N == 0)
            return std::vector<T, AlignedAllocator<T, 32>>(v.size());
        else
            return AlignedArray<T, N>{};
    }

    /* ================= SIMD PRIMITIVES ================= */

    namespace simd
    {
        GEOM_FORCE_INLINE float hsum(__m256 v) noexcept
        {
            __m128 lo = _mm256_castps256_ps128(v);
            __m128 hi = _mm256_extractf128_ps(v, 1);
            __m128 s = _mm_add_ps(lo, hi);
            s = _mm_add_ps(s, _mm_movehl_ps(s, s));
            s = _mm_add_ps(s, _mm_shuffle_ps(s, s, 0x55));
            return _mm_cvtss_f32(s);
        }

        GEOM_FORCE_INLINE double hsum(__m256d v) noexcept
        {
            __m128d lo = _mm256_castpd256_pd128(v);
            __m128d hi = _mm256_extractf128_pd(v, 1);
            __m128d s = _mm_add_pd(lo, hi);
            return _mm_cvtsd_f64(_mm_add_sd(s, _mm_unpackhi_pd(s, s)));
        }

        struct Add
        {
            static __m256 v(__m256 a, __m256 b) noexcept { return _mm256_add_ps(a, b); }
            static __m256d v(__m256d a, __m256d b) noexcept { return _mm256_add_pd(a, b); }
            template <typename T>
            static constexpr T s(const T &a, const T &b) noexcept { return a + b; }
        };

        struct Sub
        {
            static __m256 v(__m256 a, __m256 b) noexcept { return _mm256_sub_ps(a, b); }
            static __m256d v(__m256d a, __m256d b) noexcept { return _mm256_sub_pd(a, b); }
            template <typename T>
            static constexpr T s(const T &a, const T &b) noexcept { return a - b; }
        };

        struct Mul
        {
            static __m256 v(__m256 a, __m256 b) noexcept { return _mm256_mul_ps(a, b); }
            static __m256d v(__m256d a, __m256d b) noexcept { return _mm256_mul_pd(a, b); }
            template <typename T>
            static constexpr T s(const T &a, const T &b) noexcept { return a * b; }
        };

        template <typename Op, typename T>
        GEOM_FORCE_INLINE void apply_op(
            T *GEOM_RESTRICT out,
            const T *GEOM_RESTRICT a,
            const T *GEOM_RESTRICT b,
            std::size_t n) noexcept
        {
            std::size_t i = 0;
            if constexpr (std::is_same_v<T, float>)
            {
                for (; i + 8 <= n; i += 8)
                {
                    _mm256_store_ps(out + i, Op::v(_mm256_load_ps(a + i), _mm256_load_ps(b + i)));
                }
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                for (; i + 4 <= n; i += 4)
                {
                    _mm256_store_pd(out + i, Op::v(_mm256_load_pd(a + i), _mm256_load_pd(b + i)));
                }
            }

            for (; i < n; ++i)
                out[i] = Op::s(a[i], b[i]);
        }
    }

    /* ================= OPERAÇÕES ELEMENTARES ESCALARES (2D, 3D, 4D) ================= */

    template <typename T, std::size_t N>
    struct AddImpl
    {
        static GEOM_FORCE_INLINE auto apply(const LinearStorage<T, N> &a, const LinearStorage<T, N> &b)
        {
            auto out = makeSimilar<T, N>(a);
            simd::apply_op<simd::Add>(out.data(), a.data(), b.data(), getSize<T, N>(a));
            return out;
        }
    };

    template <typename T>
    struct AddImpl<T, 2>
    {
        static GEOM_FORCE_INLINE AlignedArray<T, 2> apply(const AlignedArray<T, 2> &a, const AlignedArray<T, 2> &b) noexcept
        {
            return {a[0] + b[0], a[1] + b[1]};
        }
    };

    template <typename T>
    struct AddImpl<T, 3>
    {
        static GEOM_FORCE_INLINE AlignedArray<T, 3> apply(const AlignedArray<T, 3> &a, const AlignedArray<T, 3> &b) noexcept
        {
            return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
        }
    };

    template <typename T>
    struct AddImpl<T, 4>
    {
        static GEOM_FORCE_INLINE AlignedArray<T, 4> apply(const AlignedArray<T, 4> &a, const AlignedArray<T, 4> &b) noexcept
        {
            return {a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3]};
        }
    };

    template <typename T, std::size_t N>
    struct SubImpl
    {
        static GEOM_FORCE_INLINE auto apply(const LinearStorage<T, N> &a, const LinearStorage<T, N> &b)
        {
            auto out = makeSimilar<T, N>(a);
            simd::apply_op<simd::Sub>(out.data(), a.data(), b.data(), getSize<T, N>(a));
            return out;
        }
    };

    template <typename T>
    struct SubImpl<T, 2>
    {
        static GEOM_FORCE_INLINE AlignedArray<T, 2> apply(const AlignedArray<T, 2> &a, const AlignedArray<T, 2> &b) noexcept
        {
            return {a[0] - b[0], a[1] - b[1]};
        }
    };

    template <typename T>
    struct SubImpl<T, 3>
    {
        static GEOM_FORCE_INLINE AlignedArray<T, 3> apply(const AlignedArray<T, 3> &a, const AlignedArray<T, 3> &b) noexcept
        {
            return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
        }
    };

    template <typename T>
    struct SubImpl<T, 4>
    {
        static GEOM_FORCE_INLINE AlignedArray<T, 4> apply(const AlignedArray<T, 4> &a, const AlignedArray<T, 4> &b) noexcept
        {
            return {a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3]};
        }
    };

    template <typename T, std::size_t N>
    struct MulImpl
    {
        static GEOM_FORCE_INLINE auto apply(const LinearStorage<T, N> &a, const LinearStorage<T, N> &b)
        {
            auto out = makeSimilar<T, N>(a);
            simd::apply_op<simd::Mul>(out.data(), a.data(), b.data(), getSize<T, N>(a));
            return out;
        }
    };

    template <typename T>
    struct MulImpl<T, 2>
    {
        static GEOM_FORCE_INLINE AlignedArray<T, 2> apply(const AlignedArray<T, 2> &a, const AlignedArray<T, 2> &b) noexcept
        {
            return {a[0] * b[0], a[1] * b[1]};
        }
    };

    template <typename T>
    struct MulImpl<T, 3>
    {
        static GEOM_FORCE_INLINE AlignedArray<T, 3> apply(const AlignedArray<T, 3> &a, const AlignedArray<T, 3> &b) noexcept
        {
            return {a[0] * b[0], a[1] * b[1], a[2] * b[2]};
        }
    };

    template <typename T>
    struct MulImpl<T, 4>
    {
        static GEOM_FORCE_INLINE AlignedArray<T, 4> apply(const AlignedArray<T, 4> &a, const AlignedArray<T, 4> &b) noexcept
        {
            return {a[0] * b[0], a[1] * b[1], a[2] * b[2], a[3] * b[3]};
        }
    };

    /* ================= DOT PRODUCT ================= */

    template <Scalar T, std::size_t N>
    GEOM_FORCE_INLINE T dot(const LinearStorage<T, N> &lhs, const LinearStorage<T, N> &rhs) noexcept
    {
        if constexpr (N == 2)
        {
            return lhs[0] * rhs[0] + lhs[1] * rhs[1];
        }
        else if constexpr (N == 3)
        {
            return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
        }
        else if constexpr (N == 4)
        {
            return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2] + lhs[3] * rhs[3];
        }
        else
        {
            const std::size_t n = getSize<T, N>(lhs);
            const T *a = lhs.data();
            const T *b = rhs.data();
            std::size_t i = 0;
            T res{};

            if constexpr (std::is_same_v<T, float>)
            {
                __m256 acc0 = _mm256_setzero_ps();
                __m256 acc1 = _mm256_setzero_ps();

                for (; i + 16 <= n; i += 16)
                {
                    acc0 = _mm256_fmadd_ps(_mm256_load_ps(a + i), _mm256_load_ps(b + i), acc0);
                    acc1 = _mm256_fmadd_ps(_mm256_load_ps(a + i + 8), _mm256_load_ps(b + i + 8), acc1);
                }
                acc0 = _mm256_add_ps(acc0, acc1);

                for (; i + 8 <= n; i += 8)
                {
                    acc0 = _mm256_fmadd_ps(_mm256_load_ps(a + i), _mm256_load_ps(b + i), acc0);
                }
                res = simd::hsum(acc0);
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                __m256d acc = _mm256_setzero_pd();
                for (; i + 4 <= n; i += 4)
                {
                    acc = _mm256_fmadd_pd(_mm256_load_pd(a + i), _mm256_load_pd(b + i), acc);
                }
                res = simd::hsum(acc);
            }

            for (; i < n; ++i)
                res += a[i] * b[i];

            return res;
        }
    }

    /* ================= CROSS ================= */

    template <Scalar T, std::size_t N>
        requires(N == 3)
    GEOM_FORCE_INLINE auto cross(const LinearStorage<T, 3> &a, const LinearStorage<T, 3> &b) noexcept
    {
        AlignedArray<T, 3> out;
        out[0] = a[1] * b[2] - a[2] * b[1];
        out[1] = a[2] * b[0] - a[0] * b[2];
        out[2] = a[0] * b[1] - a[1] * b[0];
        return out;
    }

    /* ================= LENGTH ================= */

    template <Scalar T, std::size_t N>
    GEOM_FORCE_INLINE T sqrLength(const LinearStorage<T, N> &v) noexcept
    {
        return dot<T, N>(v, v);
    }

    template <NormalizableScalar T, std::size_t N>
    GEOM_FORCE_INLINE T length(const LinearStorage<T, N> &v)
    {
        return std::sqrt(sqrLength<T, N>(v));
    }

    /* ================= MUL (SCALAR) ================= */

    template <Scalar T, std::size_t N>
    GEOM_FORCE_INLINE auto mul(const LinearStorage<T, N> &v, T scalar)
    {
        if constexpr (N == 2)
        {
            return AlignedArray<T, 2>{v[0] * scalar, v[1] * scalar};
        }
        else if constexpr (N == 3)
        {
            return AlignedArray<T, 3>{v[0] * scalar, v[1] * scalar, v[2] * scalar};
        }
        else if constexpr (N == 4)
        {
            return AlignedArray<T, 4>{v[0] * scalar, v[1] * scalar, v[2] * scalar, v[3] * scalar};
        }
        else
        {
            auto out = makeSimilar<T, N>(v);
            const std::size_t n = getSize<T, N>(v);
            std::size_t i = 0;

            if constexpr (std::is_same_v<T, float>)
            {
                const __m256 s = _mm256_set1_ps(scalar);
                for (; i + 8 <= n; i += 8)
                {
                    _mm256_store_ps(out.data() + i, _mm256_mul_ps(_mm256_load_ps(v.data() + i), s));
                }
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                const __m256d s = _mm256_set1_pd(scalar);
                for (; i + 4 <= n; i += 4)
                {
                    _mm256_store_pd(out.data() + i, _mm256_mul_pd(_mm256_load_pd(v.data() + i), s));
                }
            }

            for (; i < n; ++i)
                out[i] = v[i] * scalar;

            return out;
        }
    }

    /* ================= NORMALIZE ================= */

    template <NormalizableScalar T, std::size_t N>
    GEOM_FORCE_INLINE auto normalize(const LinearStorage<T, N> &v)
    {
        const T sql = sqrLength<T, N>(v);
        if (sql <= T{})
            throw std::runtime_error("Zero length vector cannot be normalized");

        return mul<T, N>(v, T{1} / std::sqrt(sql));
    }

    /* ================= PROJECT & REFLECT ================= */

    template <Scalar T, std::size_t N>
    GEOM_FORCE_INLINE auto project(const LinearStorage<T, N> &v, const LinearStorage<T, N> &unit)
    {
        return mul<T, N>(unit, dot<T, N>(v, unit));
    }

    template <Scalar T, std::size_t N>
    GEOM_FORCE_INLINE auto reflect(const LinearStorage<T, N> &v, const LinearStorage<T, N> &unit)
    {
        const T d = dot<T, N>(v, unit);
        return operator- <T, N>(v, mul<T, N>(unit, T{2} * d));
    }

    /* ================= QUATERNION ================= */

    template <Scalar T>
    GEOM_FORCE_INLINE constexpr auto conjugate(const LinearStorage<T, 4> &q) noexcept
    {
        return AlignedArray<T, 4>{-q[0], -q[1], -q[2], q[3]};
    }

    template <Scalar T>
    GEOM_FORCE_INLINE constexpr auto hamilton(const LinearStorage<T, 4> &a, const LinearStorage<T, 4> &b) noexcept
    {
        AlignedArray<T, 4> out;
        const T ax = a[0], ay = a[1], az = a[2], aw = a[3];
        const T bx = b[0], by = b[1], bz = b[2], bw = b[3];

        out[0] = aw * bx + ax * bw + ay * bz - az * by;
        out[1] = aw * by - ax * bz + ay * bw + az * bx;
        out[2] = aw * bz + ax * by - ay * bx + az * bw;
        out[3] = aw * bw - ax * bx - ay * by - az * bz;

        return out;
    }

    /* ================= ROTATE ================= */

    template <Scalar T>
    GEOM_FORCE_INLINE auto rotate(const LinearStorage<T, 3> &v, const LinearStorage<T, 4> &q)
    {
        const T tx = T{2} * (q[1] * v[2] - q[2] * v[1]);
        const T ty = T{2} * (q[2] * v[0] - q[0] * v[2]);
        const T tz = T{2} * (q[0] * v[1] - q[1] * v[0]);

        AlignedArray<T, 3> out;
        out[0] = v[0] + q[3] * tx + (q[1] * tz - q[2] * ty);
        out[1] = v[1] + q[3] * ty + (q[2] * tx - q[0] * tz);
        out[2] = v[2] + q[3] * tz + (q[0] * ty - q[1] * tx);

        return out;
    }

    template <Scalar T>
    GEOM_FORCE_INLINE auto rotate(const LinearStorage<T, 4> &v, const LinearStorage<T, 4> &q)
    {
        AlignedArray<T, 4> out;
        const T vx = v[0], vy = v[1], vz = v[2];
        const T qx = q[0], qy = q[1], qz = q[2], qw = q[3];

        const T tx = T{2} * (qy * vz - qz * vy);
        const T ty = T{2} * (qz * vx - qx * vz);
        const T tz = T{2} * (qx * vy - qy * vx);

        out[0] = vx + qw * tx + (qy * tz - qz * ty);
        out[1] = vy + qw * ty + (qz * tx - qx * tz);
        out[2] = vz + qw * tz + (qx * ty - qy * tx);
        out[3] = v[3];

        return out;
    }

    /* ================= OPERATORS ================= */

    // Overloads para AlignedArray (N > 0)
    template <Scalar T, std::size_t N>
    GEOM_FORCE_INLINE auto operator+(const AlignedArray<T, N> &a, const AlignedArray<T, N> &b)
    {
        return AddImpl<T, N>::apply(a, b);
    }

    template <Scalar T, std::size_t N>
    GEOM_FORCE_INLINE auto operator-(const AlignedArray<T, N> &a, const AlignedArray<T, N> &b)
    {
        return SubImpl<T, N>::apply(a, b);
    }

    template <Scalar T, std::size_t N>
    GEOM_FORCE_INLINE auto operator*(const AlignedArray<T, N> &a, const AlignedArray<T, N> &b)
    {
        return MulImpl<T, N>::apply(a, b);
    }

    template <Scalar T, std::size_t N>
    GEOM_FORCE_INLINE void operator+=(AlignedArray<T, N> &a, const AlignedArray<T, N> &b)
    {
        a = a + b;
    }

    template <Scalar T, std::size_t N>
    GEOM_FORCE_INLINE void operator-=(AlignedArray<T, N> &a, const AlignedArray<T, N> &b)
    {
        a = a - b;
    }

    template <Scalar T, std::size_t N>
    GEOM_FORCE_INLINE auto operator*(const AlignedArray<T, N> &v, T s)
    {
        return mul<T, N>(v, s);
    }

    template <Scalar T, std::size_t N>
    GEOM_FORCE_INLINE auto operator*(T s, const AlignedArray<T, N> &v)
    {
        return mul<T, N>(v, s);
    }

    // Overloads para std::vector com AlignedAllocator (N == 0 / dinâmico)
    template <Scalar T>
    GEOM_FORCE_INLINE auto operator+(const std::vector<T, AlignedAllocator<T, 32>> &a,
                                     const std::vector<T, AlignedAllocator<T, 32>> &b)
    {
        return AddImpl<T, 0>::apply(a, b);
    }

    template <Scalar T>
    GEOM_FORCE_INLINE auto operator-(const std::vector<T, AlignedAllocator<T, 32>> &a,
                                     const std::vector<T, AlignedAllocator<T, 32>> &b)
    {
        return SubImpl<T, 0>::apply(a, b);
    }

    template <Scalar T>
    GEOM_FORCE_INLINE auto operator*(const std::vector<T, AlignedAllocator<T, 32>> &a,
                                     const std::vector<T, AlignedAllocator<T, 32>> &b)
    {
        return MulImpl<T, 0>::apply(a, b);
    }

    template <Scalar T>
    GEOM_FORCE_INLINE void operator+=(std::vector<T, AlignedAllocator<T, 32>> &a,
                                      const std::vector<T, AlignedAllocator<T, 32>> &b)
    {
        a = a + b;
    }

    template <Scalar T>
    GEOM_FORCE_INLINE void operator-=(std::vector<T, AlignedAllocator<T, 32>> &a,
                                      const std::vector<T, AlignedAllocator<T, 32>> &b)
    {
        a = a - b;
    }

    template <Scalar T>
    GEOM_FORCE_INLINE auto operator*(const std::vector<T, AlignedAllocator<T, 32>> &v, T s)
    {
        return mul<T, 0>(v, s);
    }

    template <Scalar T>
    GEOM_FORCE_INLINE auto operator*(T s, const std::vector<T, AlignedAllocator<T, 32>> &v)
    {
        return mul<T, 0>(v, s);
    }
} // namespace geometry