#pragma once

#include "vector.hpp"

#include <cmath>

namespace geometry {

template<Scalar T>
class Rotator3
{
public:

    using Vec3 = Vector<T, 3>;
    using Quat = Vector<T, 4>;

private:

    Quat m_rotation{
        T{0},
        T{0},
        T{0},
        T{1}
    };

public:

    constexpr Rotator3() = default;

    explicit Rotator3(
        const Quat& rotation
    )
        : m_rotation(
            rotation.normalized()
        )
    {
    }

    [[nodiscard]]
    const Quat& quaternion() const
    {
        return m_rotation;
    }

    [[nodiscard]]
    Quat quaternion()
    {
        return m_rotation;
    }

    void setQuaternion(
        const Quat& q
    )
    {
        m_rotation =
            q.normalized();
    }

    [[nodiscard]]
    static Quat fromAxisAngle(
        const Vec3& axis,
        T angle
    )
    {
        const T halfAngle =
            angle * T{0.5};

        const T s =
            std::sin(
                halfAngle
            );

        const auto n =
            axis.normalized();

        return {
            n[0] * s,
            n[1] * s,
            n[2] * s,
            std::cos(halfAngle)
        };
    }

    void rotateLocal(
    const Vec3& axis,
    T angle
    )
    {
        const auto delta =
            fromAxisAngle(
                axis,
                angle
            );

        m_rotation =
            m_rotation
                .hamilton(delta)
                .normalized();
    }

    void rotateWorld(
        const Vec3& axis,
        T angle
    )
    {
        const auto delta =
            fromAxisAngle(
                axis,
                angle
            );

        m_rotation =
            delta
                .hamilton(m_rotation)
                .normalized();
    }

    [[nodiscard]]
    Vec3 rotateVector(
        const Vec3& v
    ) const
    {
        return v.rotated(
            m_rotation
        );
    }

    [[nodiscard]]
    Vec3 forward() const
    {
        return rotateVector(
            {
                T{0},
                T{0},
                T{-1}
            }
        );
    }

    [[nodiscard]]
    Vec3 right() const
    {
        return rotateVector(
            {
                T{1},
                T{0},
                T{0}
            }
        );
    }

    [[nodiscard]]
    Vec3 up() const
    {
        return rotateVector(
            {
                T{0},
                T{1},
                T{0}
            }
        );
    }

    [[nodiscard]]
    T x() const
    {
        return m_rotation[0];
    }

    [[nodiscard]]
    T y() const
    {
        return m_rotation[1];
    }

    [[nodiscard]]
    T z() const
    {
        return m_rotation[2];
    }

    [[nodiscard]]
    T w() const
    {
        return m_rotation[3];
    }

    [[nodiscard]]
    Vec3 euler() const
    {
        const auto& q =
            m_rotation;

        const T x = q[0];
        const T y = q[1];
        const T z = q[2];
        const T w = q[3];

        const T sinPitch =
            T{2} *
            (w * x + y * z);

        const T cosPitch =
            T{1} -
            T{2} *
            (x * x + y * y);

        const T pitch =
            std::atan2(
                sinPitch,
                cosPitch
            );

        const T sinYaw =
            T{2} *
            (w * y - z * x);

        const T yaw =
            std::abs(
                sinYaw
            ) >= T{1}
            ?
            std::copysign(
                static_cast<T>(
                    M_PI / 2.0
                ),
                sinYaw
            )
            :
            std::asin(
                sinYaw
            );

        const T sinRoll =
            T{2} *
            (w * z + x * y);

        const T cosRoll =
            T{1} -
            T{2} *
            (y * y + z * z);

        const T roll =
            std::atan2(
                sinRoll,
                cosRoll
            );

        return {
            pitch,
            yaw,
            roll
        };
    }

    [[nodiscard]]
    static Rotator3 identity()
    {
        return {};
    }
};

using Rot3f = Rotator3<float>;
using Rot3d = Rotator3<double>;

}