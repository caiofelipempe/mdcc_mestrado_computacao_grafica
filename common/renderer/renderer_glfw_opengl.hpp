#pragma once

#include <string>

#include "renderer.hpp"

#include "drawer_opengl.hpp"

struct GLFWwindow;

class RendererGlfwOpengl : public Renderer<DrawerOpengl>
{
public:
    RendererGlfwOpengl();

    ~RendererGlfwOpengl() override;

    void run(
        int width,
        int height,
        const std::string &title) override;

protected:
    virtual void onInit(
        int initialWidth,
        int initialHeight,
        const std::string &initialTitle) override {};

    virtual void onUpdate(
        float dt) override {};

    virtual void onRender(DrawerOpengl &drawer) override {};

    virtual void onUI() override {};

    virtual void onShutdown() override {};

    virtual void onWindowResize(int width, int height);

private:
    void initGLFW(
        int width,
        int height,
        const std::string &title);

    void updateCamera();

    void updateGamepad();

    static void keyCallback(
        GLFWwindow *window,
        int key,
        int scancode,
        int action,
        int mods);

    static void mouseButtonCallback(
        GLFWwindow *window,
        int button,
        int action,
        int mods);

    static void cursorPosCallback(
        GLFWwindow *window,
        double x,
        double y);

    static void scrollCallback(
        GLFWwindow *window,
        double dx,
        double dy);

    static void windowSizeCallback(
        GLFWwindow *window,
        int width,
        int height);

    static void framebufferSizeCallback(GLFWwindow *, int, int);

    void onFramebufferResize(int width, int height);

private:
    GLFWwindow *m_window{};
    int m_lastFbW = 0;
    int m_lastFbH = 0;

protected:
    GLFWwindow *const window()
    {
        return m_window;
    }
};