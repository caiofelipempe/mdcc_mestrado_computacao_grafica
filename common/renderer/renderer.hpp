#pragma once

#include <string>

#include "input.h"
#include "camera.hpp"

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

    virtual void onUpdate(float dt);

    virtual void onRender();

    virtual void onUI();

    virtual void onShutdown();

    virtual void onWindowResize(
        int width,
        int height
    );

    const InputState& input() const;

    Camera& camera() { return m_camera; }
    const Camera& camera() const{ return m_camera; }
    
    void setTargetFPS(int fps) { m_targetFPS = fps; }
    int targetFPS() const { return m_targetFPS; }

    void drawVertex(
        const geometry::Point3f& point,
        const geometry::Color& color =
            geometry::Color::White(),
        float size = 5.0f
    );

    void drawLine(
        const geometry::Point3f& a,
        const geometry::Point3f& b,
        const geometry::Color& color =
            geometry::Color::White(),
        float width = 1.0f
    );

    void drawFace(
        const geometry::Point3f& a,
        const geometry::Point3f& b,
        const geometry::Point3f& c,
        const geometry::Color& color =
            geometry::Color::White()
    );

    void drawMesh(
        const geometry::Mesh3f& mesh,
        const geometry::Color& color
    );

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

    int m_targetFPS = 60;
};