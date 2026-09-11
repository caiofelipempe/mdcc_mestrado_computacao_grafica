#pragma once

#include <string>

#include "input.h"
#include "camera.hpp"
#include "drawer.hpp"

#include "color.hpp"
#include "mesh.hpp"

struct GLFWwindow;

class Renderer
{
public:

    Renderer();
    virtual ~Renderer();

    void run(
        int w,
        int h,
        const std::string& t
    );

protected:

    virtual void onInit(
        int initialWidth,
        int initialHeight,
        const std::string& initialTitle
    );

    virtual bool shouldRender() { return true; }

    virtual int targetFPS() { return 0; }
    
    virtual void onUpdate(float dt);

    virtual void onRender(Drawer& drawer);

    virtual void onUI();

    virtual void onShutdown();

    virtual void onWindowResize(
        int width,
        int height
    );

    const InputState& input() const;

    Camera& camera() { return m_camera; }
    const Camera& camera() const{ return m_camera; }

private:

    void initGLFW(
        int w,
        int h,
        const std::string& t
    );

    void initImGui();

    void shutdownImGui();

    void updateCamera();

    void updateGamepad();

    static void keyCallback(
        GLFWwindow*,
        int,
        int,
        int,
        int
    );

    static void mouseButtonCallback(
        GLFWwindow*,
        int,
        int,
        int
    );

    static void cursorPosCallback(
        GLFWwindow*,
        double,
        double
    );

    static void scrollCallback(
        GLFWwindow*,
        double,
        double
    );

    static void windowSizeCallback(
        GLFWwindow*,
        int,
        int
    );

private:

    GLFWwindow* m_window{};

    InputState m_input{};

    Camera m_camera;
};