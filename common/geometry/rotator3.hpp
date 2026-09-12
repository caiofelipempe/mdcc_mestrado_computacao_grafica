#pragma once

#include "vector.hpp"

#include <cmath>
#include <numbers>
#include <concepts>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace geometry {

namespace detail {

// ---- Escala de ponto fixo para quaternions com T inteiro ----
//
// kUnit = maior 2^n tal que 4 * kUnit^2 <= numeric_limits<T>::max().
//
// Motivo do fator 4: hamilton()/dot() em arithmetic.hpp somam até 4
// produtos de pares de componentes por saída (ex.:
// out[0] = aw*bx + ax*bw + ay*bz - az*by). Cada produto na escala kUnit
// vale ~kUnit^2; no pior caso os 4 termos somam no mesmo sentido, então
// 4*kUnit^2 precisa caber em T para não estourar. Rotator3 evita fazer
// essa soma em aritmética T crua internamente (ver composeHamilton), mas
// o bound protege qualquer uso direto de Vector<T,4>::hamilton()/dot()
// fora do escopo desta classe, já que Quat é só um Vector<T,4> comum.
template <std::integral T>
constexpr T fixedPointScale() {
    const std::uint64_t maxT = static_cast<std::uint64_t>(std::numeric_limits<T>::max());

    std::uint64_t unit = 1;
    while (true) {
        const std::uint64_t candidate = unit * 2;
        if (4ull * candidate * candidate > maxT)
            break;
        unit = candidate;
    }
    return static_cast<T>(unit);
}

// "Valor unitário" de um componente: 1 em ponto flutuante, kUnit no
// ponto fixo inteiro.
template <Scalar T>
constexpr T unitValue() {
    if constexpr (std::integral<T>)
        return fixedPointScale<T>();
    else
        return T{1};
}

// 1 unidade inteira de ângulo == 2π / kUnit radianos — kUnit unidades
// equivalem a uma volta completa. Para T ponto flutuante o ângulo já é
// o próprio radiano (fator 1, identidade).
template <Scalar T>
constexpr double unitsPerRadian() {
    if constexpr (std::integral<T>)
        return static_cast<double>(fixedPointScale<T>()) / (2.0 * std::numbers::pi);
    else
        return 1.0;
}

template <typename T>
T roundToInt(double v) {
    return static_cast<T>(std::llround(v));
}

} // namespace detail

// Convenção do quaternion: (x, y, z, w) — parte vetorial nos índices
// 0..2, escalar no índice 3. Consistente com hamilton()/rotate()/
// conjugate() em arithmetic.hpp.
//
// T inteiro: representação em ponto fixo. Um componente "unitário" vale
// kUnit (não 1) — ver detail::fixedPointScale. Toda aritmética interna
// de composição/rotação é feita em double e só convertida de volta para
// T no final (ver composeHamilton/rotateVector/normalizeAny), então
// Rotator3 em si nunca soma produtos em T cru. kUnit continua definido
// pelo bound 4*kUnit^2<=maxT porque Quat é um Vector<T,4> comum — nada
// impede chamar .hamilton()/.dot() direto nele fora desta classe, e aí
// o bound é o que garante que essa soma não estoure.
//
// Vetores passados para rotateVector() (e os retornados por
// forward/right/up) são assumidos na MESMA escala kUnit; não serve para
// rotacionar posições de mundo com escala própria.
//
// euler() assume a sequência pitch(X) -> yaw(Y) -> roll(Z), com Y como
// eixo "up" (forward = -Z, right = +X, up = +Y).
template<NormalizableScalar T>
class Rotator3
{
    static_assert(std::floating_point<T> || std::signed_integral<T>,
        "Rotator3<T> requer T ponto flutuante ou inteiro com sinal");

public:
    using Vec3 = Vector<T, 3>;
    using Quat = Vector<T, 4>;

    // Magnitude de um componente "unitário" (1 para ponto flutuante,
    // escala de ponto fixo para T inteiro).
    static constexpr T kUnit = detail::unitValue<T>();

private:
    Quat m_rotation{ T{0}, T{0}, T{0}, kUnit };

    // Normaliza para módulo == kUnit. Ponto flutuante usa
    // Vector::normalized(); ponto fixo calcula em double internamente
    // pra não estourar T e re-escala pro módulo kUnit.
    template <std::size_t N>
    static Vector<T, N> normalizeAny(const Vector<T, N>& v) {
        if constexpr (std::integral<T>) {
            double sumSq = 0.0;
            for (std::size_t i = 0; i < N; ++i)
                sumSq += static_cast<double>(v[i]) * static_cast<double>(v[i]);

            if (sumSq <= 0.0)
                throw std::runtime_error("Zero length");

            const double invLen = static_cast<double>(kUnit) / std::sqrt(sumSq);

            Vector<T, N> out;
            for (std::size_t i = 0; i < N; ++i)
                out[i] = detail::roundToInt<T>(static_cast<double>(v[i]) * invLen);

            return out;
        } else {
            return v.normalized();
        }
    }

    // Produto de Hamilton entre dois quaternions de ponto fixo, feito
    // inteiramente em double: soma os 4 termos em ponto flutuante (sem
    // risco de overflow) e só converte de volta para T no final. Nunca
    // chama hamilton<T>() bruto quando T é inteiro — essa soma em T cru
    // é exatamente o que estouraria mesmo com o bound de kUnit, porque
    // o bound limita a soma, não o valor intermediário em si.
    static Quat composeHamilton(const Quat& a, const Quat& b) {
        if constexpr (std::integral<T>) {
            const double ax = static_cast<double>(a[0]) / kUnit;
            const double ay = static_cast<double>(a[1]) / kUnit;
            const double az = static_cast<double>(a[2]) / kUnit;
            const double aw = static_cast<double>(a[3]) / kUnit;

            const double bx = static_cast<double>(b[0]) / kUnit;
            const double by = static_cast<double>(b[1]) / kUnit;
            const double bz = static_cast<double>(b[2]) / kUnit;
            const double bw = static_cast<double>(b[3]) / kUnit;

            const double rx = aw*bx + ax*bw + ay*bz - az*by;
            const double ry = aw*by - ax*bz + ay*bw + az*bx;
            const double rz = aw*bz + ax*by - ay*bx + az*bw;
            const double rw = aw*bw - ax*bx - ay*by - az*bz;

            return {
                detail::roundToInt<T>(rx * kUnit),
                detail::roundToInt<T>(ry * kUnit),
                detail::roundToInt<T>(rz * kUnit),
                detail::roundToInt<T>(rw * kUnit)
            };
        } else {
            return a.hamilton(b);
        }
    }

public:

    constexpr Rotator3() = default;

    explicit Rotator3(const Quat& rotation)
        : m_rotation(normalizeAny(rotation)) {}

    [[nodiscard]] const Quat& quaternion() const { return m_rotation; }

    void setQuaternion(const Quat& q) {
        m_rotation = normalizeAny(q);
    }

    // T ponto flutuante: angle em radianos (comportamento original).
    // T inteiro: angle já em "unidades de rotação" (kUnit unidades =
    // uma volta completa). Use fromRadians()/toRadians() para converter.
    [[nodiscard]]
    static Quat fromAxisAngle(const Vec3& axis, T angle) {
        const double radians   = static_cast<double>(angle) / detail::unitsPerRadian<T>();
        const double halfAngle = radians * 0.5;
        const double s = std::sin(halfAngle);
        const double c = std::cos(halfAngle);

        const auto n = normalizeAny(axis);

        if constexpr (std::integral<T>) {
            return {
                detail::roundToInt<T>(static_cast<double>(n[0]) * s),
                detail::roundToInt<T>(static_cast<double>(n[1]) * s),
                detail::roundToInt<T>(static_cast<double>(n[2]) * s),
                detail::roundToInt<T>(c * static_cast<double>(kUnit))
            };
        } else {
            return {
                n[0] * static_cast<T>(s),
                n[1] * static_cast<T>(s),
                n[2] * static_cast<T>(s),
                static_cast<T>(c)
            };
        }
    }

    // Converte radianos para a unidade de ângulo de T (identidade para
    // ponto flutuante; aplica kUnit/(2π) para T inteiro).
    [[nodiscard]] static T fromRadians(double radians) {
        return detail::roundToInt<T>(radians * detail::unitsPerRadian<T>());
    }

    [[nodiscard]] static double toRadians(T angleUnits) {
        return static_cast<double>(angleUnits) / detail::unitsPerRadian<T>();
    }

    void rotateLocal(const Vec3& axis, T angle) {
        const auto delta = fromAxisAngle(axis, angle);
        m_rotation = normalizeAny(composeHamilton(m_rotation, delta));
    }

    void rotateWorld(const Vec3& axis, T angle) {
        const auto delta = fromAxisAngle(axis, angle);
        m_rotation = normalizeAny(composeHamilton(delta, m_rotation));
    }

    // Assume v na mesma escala de ponto fixo do quaternion (kUnit =
    // "1.0") — ver nota na documentação da classe.
    [[nodiscard]] Vec3 rotateVector(const Vec3& v) const {
        if constexpr (std::integral<T>) {
            const double qx = static_cast<double>(m_rotation[0]) / kUnit;
            const double qy = static_cast<double>(m_rotation[1]) / kUnit;
            const double qz = static_cast<double>(m_rotation[2]) / kUnit;
            const double qw = static_cast<double>(m_rotation[3]) / kUnit;

            const double vx = static_cast<double>(v[0]) / kUnit;
            const double vy = static_cast<double>(v[1]) / kUnit;
            const double vz = static_cast<double>(v[2]) / kUnit;

            const double tx = 2.0 * (qy * vz - qz * vy);
            const double ty = 2.0 * (qz * vx - qx * vz);
            const double tz = 2.0 * (qx * vy - qy * vx);

            const double rx = vx + qw * tx + (qy * tz - qz * ty);
            const double ry = vy + qw * ty + (qz * tx - qx * tz);
            const double rz = vz + qw * tz + (qx * ty - qy * tx);

            return {
                detail::roundToInt<T>(rx * kUnit),
                detail::roundToInt<T>(ry * kUnit),
                detail::roundToInt<T>(rz * kUnit)
            };
        } else {
            return v.rotated(m_rotation);
        }
    }

    [[nodiscard]] Vec3 forward() const { return rotateVector({ T{0}, T{0}, T(-kUnit) }); }
    [[nodiscard]] Vec3 right()   const { return rotateVector({ kUnit, T{0}, T{0} }); }
    [[nodiscard]] Vec3 up()      const { return rotateVector({ T{0}, kUnit, T{0} }); }

    [[nodiscard]] T x() const { return m_rotation[0]; }
    [[nodiscard]] T y() const { return m_rotation[1]; }
    [[nodiscard]] T z() const { return m_rotation[2]; }
    [[nodiscard]] T w() const { return m_rotation[3]; }

    // Retorno na mesma unidade de ângulo usada por rotateLocal/
    // rotateWorld/fromAxisAngle (radianos para T flutuante, unidades de
    // ponto fixo para T inteiro) — simétrico com a entrada dessas
    // funções.
    [[nodiscard]] Vec3 euler() const {
        const double x = static_cast<double>(m_rotation[0]) / static_cast<double>(kUnit);
        const double y = static_cast<double>(m_rotation[1]) / static_cast<double>(kUnit);
        const double z = static_cast<double>(m_rotation[2]) / static_cast<double>(kUnit);
        const double w = static_cast<double>(m_rotation[3]) / static_cast<double>(kUnit);

        const double sinPitch = 2.0 * (w * x + y * z);
        const double cosPitch = 1.0 - 2.0 * (x * x + y * y);
        const double pitchRad = std::atan2(sinPitch, cosPitch);

        const double sinYaw = 2.0 * (w * y - z * x);
        const double yawRad = std::abs(sinYaw) >= 1.0
            ? std::copysign(std::numbers::pi / 2.0, sinYaw)
            : std::asin(sinYaw);

        const double sinRoll = 2.0 * (w * z + x * y);
        const double cosRoll = 1.0 - 2.0 * (y * y + z * z);
        const double rollRad = std::atan2(sinRoll, cosRoll);

        if constexpr (std::integral<T>) {
            const double upr = detail::unitsPerRadian<T>();
            return {
                detail::roundToInt<T>(pitchRad * upr),
                detail::roundToInt<T>(yawRad   * upr),
                detail::roundToInt<T>(rollRad  * upr)
            };
        } else {
            return { static_cast<T>(pitchRad), static_cast<T>(yawRad), static_cast<T>(rollRad) };
        }
    }

    [[nodiscard]] static Rotator3 identity() { return {}; }
};

using Rot3f   = Rotator3<float>;
using Rot3d   = Rotator3<double>;
using Rot3i32 = Rotator3<std::int32_t>;
using Rot3i64 = Rotator3<std::int64_t>;

} // namespace geometry