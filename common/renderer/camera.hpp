#pragma once

#include "vector.hpp"
#include "rotator3.hpp"

class Camera
{
public:

    Camera();

    void setPerspective(
        float fov,
        float aspect,
        float nearPlane,
        float farPlane
    );

    void resize(
        int width,
        int height
    );

    void orbit(
        float deltaYaw,
        float deltaPitch
    );

    void pan(
        float dx,
        float dy
    );

    void zoom(
        float amount
    );

    [[nodiscard]]
    const geometry::Vec3f& position() const
    {
        return m_position;
    }

    [[nodiscard]]
    const geometry::Vec3f& target() const
    {
        return m_target;
    }

    [[nodiscard]]
    const geometry::Vec3f& up() const
    {
        return m_up;
    }

    [[nodiscard]]
    const geometry::Rot3f& rotation() const
    {
        return m_rotation;
    }

    [[nodiscard]]
    float fov() const
    {
        return m_fov;
    }

    [[nodiscard]]
    float aspect() const
    {
        return m_aspect;
    }

    [[nodiscard]]
    float nearPlane() const
    {
        return m_nearPlane;
    }

    [[nodiscard]]
    float farPlane() const
    {
        return m_farPlane;
    }

    [[nodiscard]]
    float distance() const
    {
        return m_distance;
    }

    [[nodiscard]]
    bool viewDirty() const
    {
        return m_viewDirty;
    }

    [[nodiscard]]
    bool projectionDirty() const
    {
        return m_projectionDirty;
    }

    void clearViewDirty()
    {
        m_viewDirty = false;
    }

    void clearProjectionDirty()
    {
        m_projectionDirty = false;
    }

    void setViewDirty(
        bool value = true
    )
    {
        m_viewDirty = value;
    }

    void setProjectionDirty(
        bool value = true
    )
    {
        m_projectionDirty = value;
    }

    void setDistance(
        float value
    );

    void setTarget(
        const geometry::Vec3f& target
    );

    void setRotation(
        const geometry::Rot3f& rotation
    );

private:

    void updatePosition();

private:

    geometry::Vec3f m_position;
    geometry::Vec3f m_target;
    geometry::Vec3f m_up;

    geometry::Rot3f m_rotation;

    float m_fov;
    float m_aspect;
    float m_nearPlane;
    float m_farPlane;

    float m_distance;

    bool m_viewDirty;
    bool m_projectionDirty;
};