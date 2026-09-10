#pragma once

#include "vector.hpp"

class Renderer;

class Camera
{
    friend class Renderer;

public:

    Camera();

    void setPerspective(
        float fov,
        float aspect,
        float nearPlane,
        float farPlane);

    void resize(
        int width,
        int height);

    void orbit(
        float deltaYaw,
        float deltaPitch);

    void pan(
        float dx,
        float dy);

    void zoom(
        float amount);

    [[nodiscard]]
    const geometry::Vec3f& position() const;

    [[nodiscard]]
    const geometry::Vec3f& target() const;

    [[nodiscard]]
    const geometry::Vec3f& up() const;

private:

    void updatePosition();

private:

    geometry::Vec3f m_position;
    geometry::Vec3f m_target;
    geometry::Vec3f m_up;

    float m_fov;
    float m_aspect;
    float m_nearPlane;
    float m_farPlane;

    float m_distance;
    float m_yaw;
    float m_pitch;

    bool m_viewDirty;
    bool m_projectionDirty;
};