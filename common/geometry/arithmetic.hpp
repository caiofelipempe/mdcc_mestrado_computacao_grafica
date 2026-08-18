#pragma once
#pragma GCC optimize("Ofast,unroll-loops")
#pragma GCC target("avx,avx2,fma")

#include <array>
#include <vector>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <concepts>
#include <immintrin.h>
#include <memory>

namespace geometry {

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

template<typename T>
concept Addable =
requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
};

template<typename T>
concept Subtractable =
requires(T a, T b) {
    { a - b } -> std::convertible_to<T>;
};

template<typename T>
concept Multipliable =
requires(T a, T b) {
    { a * b } -> std::convertible_to<T>;
};

template<typename T>
concept Divisible =
requires(T a, T b) {
    { a / b } -> std::convertible_to<T>;
};

template<typename T>
concept Negatable =
requires(T a) {
    { -a } -> std::convertible_to<T>;
};

template<typename T>
concept Comparable =
requires(T a, T b) {
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
    { a <  b } -> std::convertible_to<bool>;
    { a <= b } -> std::convertible_to<bool>;
    { a >  b } -> std::convertible_to<bool>;
    { a >= b } -> std::convertible_to<bool>;
};

template<typename T>
concept DefaultConstructible =
requires {
    T{};
};

template<typename T>
concept Sqrtable =
requires(T a) {
    { std::sqrt(a) } -> std::convertible_to<T>;
};

template<typename T>
concept Scalar =
    Addable<T> &&
    Subtractable<T> &&
    Multipliable<T> &&
    Divisible<T> &&
    Negatable<T> &&
    Comparable<T> &&
    DefaultConstructible<T>;

template<typename T>
concept NormalizableScalar =
    Scalar<T> &&
    Sqrtable<T>;

/* ================= STORAGE ================= */

template <typename T, std::size_t N>
struct alignas(32) AlignedArray {

    std::array<T, N> _data;

    constexpr T* data() noexcept {
        return _data.data();
    }

    constexpr const T* data() const noexcept {
        return _data.data();
    }

    constexpr void fill(const T& value) noexcept {
        _data.fill(value);
    }

    constexpr std::size_t size() const noexcept {
        return N;
    }

    constexpr T& operator[](std::size_t i) noexcept {
        return _data[i];
    }

    constexpr const T& operator[](std::size_t i) const noexcept {
        return _data[i];
    }

    auto begin() noexcept { return _data.begin(); }
    auto end() noexcept { return _data.end(); }

    auto begin() const noexcept { return _data.begin(); }
    auto end() const noexcept { return _data.end(); }
};

/* ================= ALIGNED ALLOCATOR ================= */

template <typename T, std::size_t Align>
struct AlignedAllocator {

    using value_type = T;

    AlignedAllocator() = default;

    template<typename U>
    constexpr AlignedAllocator(
        const AlignedAllocator<U, Align>&
    ) noexcept {}

    [[nodiscard]]
    T* allocate(std::size_t n) {

        void* ptr = nullptr;

        if (posix_memalign(&ptr, Align, n * sizeof(T)) != 0)
            throw std::bad_alloc();

        return static_cast<T*>(ptr);
    }

    void deallocate(T* p, std::size_t) noexcept {
        free(p);
    }
};

template<typename T1, typename T2, std::size_t A>
constexpr bool operator==(
    const AlignedAllocator<T1, A>&,
    const AlignedAllocator<T2, A>&
) noexcept {
    return true;
}

/* ================= STORAGE ================= */

template <typename T, std::size_t N>
using LinearStorage =
    std::conditional_t<
        N == 0,
        std::vector<T, AlignedAllocator<T, 32>>,
        AlignedArray<T, N>
    >;

/* ================= UTILS ================= */

template <Scalar T, std::size_t N>
constexpr std::size_t getSize(
    const LinearStorage<T, N>& v
) noexcept {

    if constexpr (N == 0)
        return v.size();
    else
        return N;
}

template <Scalar T, std::size_t N>
constexpr auto makeSimilar(
    const LinearStorage<T, N>& v
) {

    if constexpr (N == 0)
        return std::vector<T, AlignedAllocator<T,32>>(v.size());
    else
        return AlignedArray<T, N>{};
}

/* ================= SIMD ================= */

namespace simd {

GEOM_FORCE_INLINE
float hsum(__m256 v) noexcept {

    __m128 lo = _mm256_castps256_ps128(v);
    __m128 hi = _mm256_extractf128_ps(v, 1);

    __m128 s = _mm_add_ps(lo, hi);

    s = _mm_add_ps(s, _mm_movehl_ps(s, s));
    s = _mm_add_ps(s, _mm_shuffle_ps(s, s, 0x55));

    return _mm_cvtss_f32(s);
}

GEOM_FORCE_INLINE
double hsum(__m256d v) noexcept {

    __m128d lo = _mm256_castpd256_pd128(v);
    __m128d hi = _mm256_extractf128_pd(v, 1);

    __m128d s = _mm_add_pd(lo, hi);

    return _mm_cvtsd_f64(
        _mm_add_sd(s, _mm_unpackhi_pd(s, s))
    );
}

struct Add {

    static __m256 v(__m256 a, __m256 b) noexcept {
        return _mm256_add_ps(a, b);
    }

    static __m256d v(__m256d a, __m256d b) noexcept {
        return _mm256_add_pd(a, b);
    }

    template<typename T>
    static constexpr T s(
        const T& a,
        const T& b
    ) noexcept {
        return a + b;
    }
};

struct Sub {

    static __m256 v(__m256 a, __m256 b) noexcept {
        return _mm256_sub_ps(a, b);
    }

    static __m256d v(__m256d a, __m256d b) noexcept {
        return _mm256_sub_pd(a, b);
    }

    template<typename T>
    static constexpr T s(
        const T& a,
        const T& b
    ) noexcept {
        return a - b;
    }
};

struct Mul {

    static __m256 v(__m256 a, __m256 b) noexcept {
        return _mm256_mul_ps(a, b);
    }

    static __m256d v(__m256d a, __m256d b) noexcept {
        return _mm256_mul_pd(a, b);
    }

    template<typename T>
    static constexpr T s(
        const T& a,
        const T& b
    ) noexcept {
        return a * b;
    }
};

template <typename Op, typename T>
GEOM_FORCE_INLINE
void apply_op(
    T* GEOM_RESTRICT out,
    const T* GEOM_RESTRICT a,
    const T* GEOM_RESTRICT b,
    std::size_t n
) noexcept {

    std::size_t i = 0;

    if constexpr (std::is_same_v<T, float>) {

        for (; i + 8 <= n; i += 8) {

            _mm256_store_ps(
                out + i,
                Op::v(
                    _mm256_load_ps(a + i),
                    _mm256_load_ps(b + i)
                )
            );
        }

    } else if constexpr (std::is_same_v<T, double>) {

        for (; i + 4 <= n; i += 4) {

            _mm256_store_pd(
                out + i,
                Op::v(
                    _mm256_load_pd(a + i),
                    _mm256_load_pd(b + i)
                )
            );
        }
    }

    for (; i < n; ++i)
        out[i] = Op::s(a[i], b[i]);
}

}

/* ================= DOT ================= */

template <Scalar T, std::size_t N>
GEOM_FORCE_INLINE
T dot(
    const LinearStorage<T, N>& lhs,
    const LinearStorage<T, N>& rhs
) noexcept {

    const std::size_t n =
        getSize<T, N>(lhs);

    const T* a = lhs.data();
    const T* b = rhs.data();

    std::size_t i = 0;

    T res{};

    if constexpr (std::is_same_v<T,float>) {

        __m256 acc0 = _mm256_setzero_ps();
        __m256 acc1 = _mm256_setzero_ps();

        for (; i + 16 <= n; i += 16) {

            acc0 = _mm256_fmadd_ps(
                _mm256_load_ps(a + i),
                _mm256_load_ps(b + i),
                acc0
            );

            acc1 = _mm256_fmadd_ps(
                _mm256_load_ps(a + i + 8),
                _mm256_load_ps(b + i + 8),
                acc1
            );
        }

        acc0 = _mm256_add_ps(acc0, acc1);

        for (; i + 8 <= n; i += 8) {

            acc0 = _mm256_fmadd_ps(
                _mm256_load_ps(a + i),
                _mm256_load_ps(b + i),
                acc0
            );
        }

        res = simd::hsum(acc0);

    } else if constexpr (std::is_same_v<T,double>) {

        __m256d acc = _mm256_setzero_pd();

        for (; i + 4 <= n; i += 4) {

            acc = _mm256_fmadd_pd(
                _mm256_load_pd(a + i),
                _mm256_load_pd(b + i),
                acc
            );
        }

        res = simd::hsum(acc);
    }

    for (; i < n; ++i)
        res += a[i] * b[i];

    return res;
}

/* ================= CROSS ================= */

template <Scalar T, std::size_t N>
requires (N == 3)
GEOM_FORCE_INLINE
auto cross(
    const LinearStorage<T,3>& a,
    const LinearStorage<T,3>& b
) noexcept {

    AlignedArray<T,3> out;

    out[0] = a[1]*b[2] - a[2]*b[1];
    out[1] = a[2]*b[0] - a[0]*b[2];
    out[2] = a[0]*b[1] - a[1]*b[0];

    return out;
}

/* ================= LENGTH ================= */

template <Scalar T, std::size_t N>
GEOM_FORCE_INLINE
T sqrLength(
    const LinearStorage<T, N>& v
) noexcept {

    return dot<T,N>(v, v);
}

template <NormalizableScalar T, std::size_t N>
GEOM_FORCE_INLINE
T length(
    const LinearStorage<T, N>& v
) {

    return std::sqrt(
        sqrLength<T,N>(v)
    );
}

/* ================= MUL ================= */

template <Scalar T, std::size_t N>
GEOM_FORCE_INLINE
auto mul(
    const LinearStorage<T,N>& v,
    T scalar
) {

    auto out = makeSimilar<T,N>(v);

    const std::size_t n =
        getSize<T,N>(v);

    std::size_t i = 0;

    if constexpr (std::is_same_v<T,float>) {

        const __m256 s =
            _mm256_set1_ps(scalar);

        for (; i + 8 <= n; i += 8) {

            _mm256_store_ps(
                out.data() + i,
                _mm256_mul_ps(
                    _mm256_load_ps(v.data() + i),
                    s
                )
            );
        }

    } else if constexpr (std::is_same_v<T,double>) {

        const __m256d s =
            _mm256_set1_pd(scalar);

        for (; i + 4 <= n; i += 4) {

            _mm256_store_pd(
                out.data() + i,
                _mm256_mul_pd(
                    _mm256_load_pd(v.data() + i),
                    s
                )
            );
        }
    }

    for (; i < n; ++i)
        out[i] = v[i] * scalar;

    return out;
}

/* ================= NORMALIZE ================= */

template <NormalizableScalar T, std::size_t N>
GEOM_FORCE_INLINE
auto normalize(
    const LinearStorage<T,N>& v
) {

    const T sql =
        sqrLength<T,N>(v);

    if (sql <= T{})
        throw std::runtime_error("Zero length");

    if constexpr (std::is_same_v<T,float>) {

        float r;

        const __m128 s_sql =
            _mm_set_ss(sql);

        const __m128 s_rsqrt =
            _mm_rsqrt_ss(s_sql);

        const __m128 h =
            _mm_set_ss(0.5f);

        const __m128 th =
            _mm_set_ss(1.5f);

        const __m128 res =
            _mm_mul_ss(
                s_rsqrt,
                _mm_sub_ss(
                    th,
                    _mm_mul_ss(
                        h,
                        _mm_mul_ss(
                            s_sql,
                            _mm_mul_ss(
                                s_rsqrt,
                                s_rsqrt
                            )
                        )
                    )
                )
            );

        _mm_store_ss(&r, res);

        return mul<T,N>(v, r);

    } else {

        return mul<T,N>(
            v,
            T{1} / std::sqrt(sql)
        );
    }
}

/* ================= PROJECT ================= */

template <Scalar T, std::size_t N>
GEOM_FORCE_INLINE
auto project(
    const LinearStorage<T,N>& v,
    const LinearStorage<T,N>& nnit
) {

    return mul<T,N>(
        nnit,
        dot<T,N>(v, nnit)
    );
}

/* ================= REFLECT ================= */

template <Scalar T, std::size_t N>
GEOM_FORCE_INLINE
auto reflect(
    const LinearStorage<T,N>& v,
    const LinearStorage<T,N>& nnit
) {

    const T d =
        dot<T,N>(v, nnit);

    return v -
        mul<T,N>(nnit, T{2} * d);
}

/* ================= QUATERNION ================= */

template <Scalar T>
GEOM_FORCE_INLINE
constexpr auto conjugate(
    const LinearStorage<T,4>& q
) noexcept {

    AlignedArray<T,4> out;

    out[0] = -q[0];
    out[1] = -q[1];
    out[2] = -q[2];
    out[3] =  q[3];

    return out;
}

template <Scalar T>
GEOM_FORCE_INLINE
constexpr auto hamilton(
    const LinearStorage<T,4>& a,
    const LinearStorage<T,4>& b
) noexcept {

    AlignedArray<T,4> out;

    const T ax = a[0];
    const T ay = a[1];
    const T az = a[2];
    const T aw = a[3];

    const T bx = b[0];
    const T by = b[1];
    const T bz = b[2];
    const T bw = b[3];

    out[0] =
        aw*bx + ax*bw + ay*bz - az*by;

    out[1] =
        aw*by - ax*bz + ay*bw + az*bx;

    out[2] =
        aw*bz + ax*by - ay*bx + az*bw;

    out[3] =
        aw*bw - ax*bx - ay*by - az*bz;

    return out;
}

/* ================= ROTATE 3 ================= */

template <Scalar T>
GEOM_FORCE_INLINE
auto rotate(
    const LinearStorage<T,3>& v,
    const LinearStorage<T,4>& q
) {

    if constexpr (std::is_same_v<T,float>) {

        const __m128 vv =
            _mm_set_ps(0.f, v[2], v[1], v[0]);

        const __m128 qq =
            _mm_load_ps(q.data());

        const __m128 qw =
            _mm_shuffle_ps(
                qq,
                qq,
                _MM_SHUFFLE(3,3,3,3)
            );

        auto cross = [](
            __m128 a,
            __m128 b
        ) {

            return _mm_sub_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(
                        a,a,
                        _MM_SHUFFLE(3,0,2,1)
                    ),
                    _mm_shuffle_ps(
                        b,b,
                        _MM_SHUFFLE(3,1,0,2)
                    )
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(
                        a,a,
                        _MM_SHUFFLE(3,1,0,2)
                    ),
                    _mm_shuffle_ps(
                        b,b,
                        _MM_SHUFFLE(3,0,2,1)
                    )
                )
            );
        };

        const __m128 t =
            _mm_add_ps(
                cross(qq, vv),
                cross(qq, vv)
            );

        const __m128 r =
            _mm_add_ps(
                vv,
                _mm_add_ps(
                    _mm_mul_ps(qw, t),
                    cross(qq, t)
                )
            );

        alignas(16) float tmp[4];

        _mm_store_ps(tmp, r);

        AlignedArray<T,3> out;

        out[0] = tmp[0];
        out[1] = tmp[1];
        out[2] = tmp[2];

        return out;
    }

    const T tx =
        T{2} * (q[1]*v[2] - q[2]*v[1]);

    const T ty =
        T{2} * (q[2]*v[0] - q[0]*v[2]);

    const T tz =
        T{2} * (q[0]*v[1] - q[1]*v[0]);

    AlignedArray<T,3> out;

    out[0] =
        v[0] +
        q[3]*tx +
        (q[1]*tz - q[2]*ty);

    out[1] =
        v[1] +
        q[3]*ty +
        (q[2]*tx - q[0]*tz);

    out[2] =
        v[2] +
        q[3]*tz +
        (q[0]*ty - q[1]*tx);

    return out;
}

/* ================= ROTATE 4 ================= */

template <Scalar T>
GEOM_FORCE_INLINE
auto rotate(
    const LinearStorage<T,4>& v,
    const LinearStorage<T,4>& q
) {

    AlignedArray<T,4> out;

    const T vx = v[0];
    const T vy = v[1];
    const T vz = v[2];

    const T qx = q[0];
    const T qy = q[1];
    const T qz = q[2];
    const T qw = q[3];

    const T tx =
        T{2} * (qy*vz - qz*vy);

    const T ty =
        T{2} * (qz*vx - qx*vz);

    const T tz =
        T{2} * (qx*vy - qy*vx);

    out[0] =
        vx +
        qw*tx +
        (qy*tz - qz*ty);

    out[1] =
        vy +
        qw*ty +
        (qz*tx - qx*tz);

    out[2] =
        vz +
        qw*tz +
        (qx*ty - qy*tx);

    out[3] = v[3];

    return out;
}

/* ================= OPERATORS ================= */

template <Scalar T, std::size_t N>
GEOM_FORCE_INLINE
void operator+=(
    LinearStorage<T,N>& a,
    const LinearStorage<T,N>& b
) {

    simd::apply_op<simd::Add>(
        a.data(),
        a.data(),
        b.data(),
        getSize<T,N>(a)
    );
}

template <Scalar T, std::size_t N>
GEOM_FORCE_INLINE
void operator-=(
    LinearStorage<T,N>& a,
    const LinearStorage<T,N>& b
) {

    simd::apply_op<simd::Sub>(
        a.data(),
        a.data(),
        b.data(),
        getSize<T,N>(a)
    );
}

template <Scalar T, std::size_t N>
GEOM_FORCE_INLINE
auto operator+(
    const LinearStorage<T,N>& a,
    const LinearStorage<T,N>& b
) {

    auto out = makeSimilar<T,N>(a);

    simd::apply_op<simd::Add>(
        out.data(),
        a.data(),
        b.data(),
        getSize<T,N>(a)
    );

    return out;
}

template <Scalar T, std::size_t N>
GEOM_FORCE_INLINE
auto operator-(
    const LinearStorage<T,N>& a,
    const LinearStorage<T,N>& b
) {

    auto out = makeSimilar<T,N>(a);

    simd::apply_op<simd::Sub>(
        out.data(),
        a.data(),
        b.data(),
        getSize<T,N>(a)
    );

    return out;
}

template <Scalar T, std::size_t N>
GEOM_FORCE_INLINE
auto operator*(
    const LinearStorage<T,N>& a,
    const LinearStorage<T,N>& b
) {

    auto out = makeSimilar<T,N>(a);

    simd::apply_op<simd::Mul>(
        out.data(),
        a.data(),
        b.data(),
        getSize<T,N>(a)
    );

    return out;
}

template <Scalar T, std::size_t N>
GEOM_FORCE_INLINE
auto operator*(
    const LinearStorage<T,N>& v,
    T s
) {

    return mul<T,N>(v, s);
}

template <Scalar T, std::size_t N>
GEOM_FORCE_INLINE
auto operator*(
    T s,
    const LinearStorage<T,N>& v
) {

    return mul<T,N>(v, s);
}

} // namespace geometry