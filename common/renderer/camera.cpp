#include "camera.hpp"

#include <cmath>

Camera::Camera()
    : m_position({0.0f, 0.0f, 10.0f})
    , m_target({0.0f, 0.0f, 0.0f})
    , m_up({0.0f, 1.0f, 0.0f})
    , m_fov(45.0f)
    , m_aspect(1.0f)
    , m_nearPlane(0.1f)
    , m_farPlane(1000.0f)
    , m_distance(10.0f)
    , m_viewDirty(true)
    , m_projectionDirty(true)
{
    constexpr float DEG2RAD =
        3.14159265358979323846f /
        180.0f;

    m_rotation.rotateX(
        -25.0f * DEG2RAD
    );

    updatePosition();
}

void Camera::setPerspective(
    float fov,
    float aspect,
    float nearPlane,
    float farPlane
)
{
    m_fov        = fov;
    m_aspect     = aspect;
    m_nearPlane  = nearPlane;
    m_farPlane   = farPlane;

    m_projectionDirty = true;
}

void Camera::resize(
    int width,
    int height
)
{
    if (height <= 0)
        height = 1;

    m_aspect =
        static_cast<float>(width) /
        static_cast<float>(height);

    m_projectionDirty = true;
}

void Camera::orbit(
    float deltaYaw,
    float deltaPitch
)
{
    constexpr float DEG2RAD =
        3.14159265358979323846f /
        180.0f;

    m_rotation.rotateY(
        deltaYaw * DEG2RAD
    );

    m_rotation.rotateX(
        deltaPitch * DEG2RAD
    );

    updatePosition();

    m_viewDirty = true;
}

void Camera::zoom(
    float amount
)
{
    m_distance -= amount;

    if (m_distance < 0.1f)
        m_distance = 0.1f;

    updatePosition();

    m_viewDirty = true;
}

void Camera::pan(
    float dx,
    float dy
)
{
    const auto right =
        m_rotation.right();

    const auto up =
        m_rotation.up();

    m_target +=
        right * dx +
        up    * dy;

    updatePosition();

    m_viewDirty = true;
}

void Camera::setDistance(
    float value
)
{
    m_distance =
        std::max(
            value,
            0.1f
        );

    updatePosition();

    m_viewDirty = true;
}

void Camera::setTarget(
    const geometry::Vec3f& target
)
{
    m_target = target;

    updatePosition();

    m_viewDirty = true;
}

void Camera::setRotation(
    const geometry::Rot3f& rotation
)
{
    m_rotation = rotation;

    updatePosition();

    m_viewDirty = true;
}

void Camera::updatePosition()
{
    const auto forward =
        m_rotation.forward();

    m_position =
        m_target -
        forward * m_distance;

    m_up =
        m_rotation.up();
}