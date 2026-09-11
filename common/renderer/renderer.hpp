#pragma once

#include "input.h"
#include "camera.hpp"

#include <string>

class Drawer;

class Renderer
{
public:
    Renderer() = default;

    virtual ~Renderer() = default;

    virtual void run(
        int width,
        int height,
        const std::string& title
    ) = 0;

    const InputState& input() const { return m_input; }

    Camera& camera()
    {
        return m_camera;
    }

    const Camera& camera() const
    {
        return m_camera;
    }

protected:
    virtual void onInit(
        int initialWidth,
        int initialHeight,
        const std::string& initialTitle
    ) = 0;

    virtual void onUpdate(
        float dt
    ) = 0;

    virtual void onRender(
        Drawer& drawer
    ) = 0;

    virtual void onUI() = 0;

    virtual void onShutdown() = 0;

    virtual bool shouldRender()
    {
        return true;
    }

    virtual int targetFPS()
    {
        return 0;
    }

    InputState& inputMutable() { return m_input; }

    InputState m_input;

    Camera m_camera{};
};