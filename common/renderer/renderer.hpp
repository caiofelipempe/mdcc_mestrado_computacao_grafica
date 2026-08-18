#pragma once

#include <string>
#include "input.h"

struct GLFWwindow;

class Renderer {
public:
    Renderer();
    virtual ~Renderer();

    void run(int w, int h, const std::string& t);

protected:
    virtual void onInit(int initialWidth, int initialHeight, const std::string& initialTitle);
    virtual void onUpdate(float dt);
    virtual void onUI();
    virtual void onShutdown();
    virtual void onWindowResize(int width, int height);

    const InputState& input() const;

private:
    void initGLFW(int w, int h, const std::string& t);
    void setCallbacks();
    void initImGui();
    void shutdownImGui();

    // Callbacks GLFW
    static void keyCallback        (GLFWwindow*, int, int, int, int);
    static void mouseButtonCallback(GLFWwindow*, int, int, int);
    static void cursorPosCallback  (GLFWwindow*, double, double);
    static void scrollCallback     (GLFWwindow*, double, double);
    static void windowSizeCallback (GLFWwindow*, int, int);

private:
    GLFWwindow* m_window{};
    InputState  m_input{};
};