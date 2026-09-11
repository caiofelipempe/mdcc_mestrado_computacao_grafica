#pragma once

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
};