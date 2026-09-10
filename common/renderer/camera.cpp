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
    , m_yaw(0.0f)
    , m_pitch(25.0f)
    , m_viewDirty(true)
    , m_projectionDirty(true)
{
    updatePosition();
}

const geometry::Vec3f&
Camera::position() const
{
    return m_position;
}

const geometry::Vec3f&
Camera::target() const
{
    return m_target;
}

const geometry::Vec3f&
Camera::up() const
{
    return m_up;
}

void Camera::setPerspective(
    float fov,
    float aspect,
    float nearPlane,
    float farPlane)
{
    m_fov = fov;
    m_aspect = aspect;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;

    m_projectionDirty = true;
}

void Camera::resize(
    int width,
    int height)
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
    float deltaPitch)
{
    m_yaw += deltaYaw;
    m_pitch += deltaPitch;

    if (m_pitch > 89.0f)
        m_pitch = 89.0f;

    if (m_pitch < -89.0f)
        m_pitch = -89.0f;

    updatePosition();

    m_viewDirty = true;
}

void Camera::zoom(float amount)
{
    m_distance -= amount;

    if (m_distance < 0.1f)
        m_distance = 0.1f;

    updatePosition();

    m_viewDirty = true;
}

void Camera::pan(
    float dx,
    float dy)
{
    m_target[0] += dx;
    m_target[1] += dy;

    updatePosition();

    m_viewDirty = true;
}

void Camera::updatePosition()
{
    constexpr float DEG2RAD =
        3.14159265358979323846f /
        180.0f;

    const float yawRad =
        m_yaw * DEG2RAD;

    const float pitchRad =
        m_pitch * DEG2RAD;

    m_position[0] =
        m_target[0] +
        m_distance *
        std::cos(pitchRad) *
        std::sin(yawRad);

    m_position[1] =
        m_target[1] +
        m_distance *
        std::sin(pitchRad);

    m_position[2] =
        m_target[2] +
        m_distance *
        std::cos(pitchRad) *
        std::cos(yawRad);
}