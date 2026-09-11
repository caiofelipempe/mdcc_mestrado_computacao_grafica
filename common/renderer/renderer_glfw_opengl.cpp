#include <GL/glew.h>

#include "renderer_glfw_opengl.hpp"
#include "drawer_opengl.hpp"

#include <GLFW/glfw3.h>
#include <GL/glu.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

// ─────────────────────────────────────────────────────────────────────────────
//  Ctor / Dtor
// ─────────────────────────────────────────────────────────────────────────────
RendererGlfwOpengl::RendererGlfwOpengl()  = default;
RendererGlfwOpengl::~RendererGlfwOpengl() = default;

// ─────────────────────────────────────────────────────────────────────────────
//  GLFW
// ─────────────────────────────────────────────────────────────────────────────
void RendererGlfwOpengl::initGLFW(const int w, const int h, const std::string& t) {
    if (!glfwInit())
        throw std::runtime_error("Erro ao iniciar GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    m_window = glfwCreateWindow(w, h, t.c_str(), nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Erro ao criar janela");
    }

    glfwMakeContextCurrent(m_window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        glfwTerminate();
        throw std::runtime_error("Erro ao iniciar GLEW");
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearDepth(1.0);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glfwSwapInterval(1);
    glViewport(0, 0, w, h);

    glfwSetWindowUserPointer   (m_window, this);
    glfwSetKeyCallback         (m_window, keyCallback);
    glfwSetMouseButtonCallback (m_window, mouseButtonCallback);
    glfwSetCursorPosCallback   (m_window, cursorPosCallback);
    glfwSetScrollCallback      (m_window, scrollCallback);
    glfwSetWindowSizeCallback  (m_window, windowSizeCallback);
}

// ─────────────────────────────────────────────────────────────────────────────
//  ImGui
// ─────────────────────────────────────────────────────────────────────────────
void RendererGlfwOpengl::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void RendererGlfwOpengl::shutdownImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void RendererGlfwOpengl::updateCamera() {
    if (m_camera.m_projectionDirty) {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        gluPerspective(
            m_camera.m_fov,
            m_camera.m_aspect,
            m_camera.m_nearPlane,
            m_camera.m_farPlane
        );

        m_camera.m_projectionDirty = false;
    }

    if (m_camera.m_viewDirty) {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        gluLookAt(
            m_camera.m_position[0], m_camera.m_position[1], m_camera.m_position[2],
            m_camera.m_target[0],   m_camera.m_target[1],   m_camera.m_target[2],
            m_camera.m_up[0],       m_camera.m_up[1],       m_camera.m_up[2]
        );

        m_camera.m_viewDirty = false;
    }
}

void RendererGlfwOpengl::updateGamepad() {
    auto& in = inputMutable();

    in.m_gamepadConnected = glfwJoystickPresent(GLFW_JOYSTICK_1);

    if (!in.m_gamepadConnected) {
        in.m_gamepadButtons.fill(false);
        in.m_gamepadAxes.fill(0.0f);
        return;
    }

    GLFWgamepadstate state;
    if (!glfwGetGamepadState(GLFW_JOYSTICK_1, &state))
        return;

    for (int i = 0; i < 15; ++i)
        in.m_gamepadButtons[i] = state.buttons[i] == GLFW_PRESS;

    for (int i = 0; i < 6; ++i)
        in.m_gamepadAxes[i] = state.axes[i];
}

// ─────────────────────────────────────────────────────────────────────────────
//  Loop principal
// ─────────────────────────────────────────────────────────────────────────────
void RendererGlfwOpengl::run(const int w, const int h, const std::string& t) {
    initGLFW(w, h, t);
    initImGui();

    // Garante shutdown correto mesmo se onInit/onUpdate/onRender lançarem.
    struct ShutdownGuard {
        RendererGlfwOpengl* self;
        ~ShutdownGuard() {
            self->onShutdown();
            self->shutdownImGui();
            if (self->m_window) {
                glfwDestroyWindow(self->m_window);
                self->m_window = nullptr;
            }
            glfwTerminate();
        }
    } guard{this};

    m_camera.resize(w, h);
    onInit(w, h, t);

    using clock = std::chrono::steady_clock;
    auto lastTime = clock::now();
    DrawerOpengl drawer;

    while (!glfwWindowShouldClose(m_window)) {
        const auto frameBegin = clock::now();
        const float dt = std::chrono::duration<float>(frameBegin - lastTime).count();
        lastTime = frameBegin;

        glfwPollEvents();
        updateGamepad();
        onUpdate(dt);

        if (shouldRender()) {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            onUI();

            drawer.frameBegin();
            updateCamera();
            onRender(drawer);
            drawer.frameEnd();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(m_window);
        }

        // Nota: com glfwSwapInterval(1) ativo, o swap já trava na taxa do
        // monitor. Esse sleep só tem efeito quando targetFPS() < taxa do
        // monitor; acima disso quem manda é o vsync, não o cap manual.
        const int fpsTarg = targetFPS();
        if (fpsTarg > 0) {
            const auto targetFrameTime = std::chrono::duration<float>(1.0f / static_cast<float>(fpsTarg));
            const auto frameDuration = clock::now() - frameBegin;

            if (frameDuration < targetFrameTime)
                std::this_thread::sleep_for(targetFrameTime - frameDuration);
        }

        inputMutable().resetFrameData();
    }
}

void RendererGlfwOpengl::onWindowResize(int width, int height) {
    glViewport(0, 0, width, height);
    m_camera.resize(width, height);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Callbacks GLFW (estáticos)
// ─────────────────────────────────────────────────────────────────────────────
void RendererGlfwOpengl::keyCallback(GLFWwindow* window, int key, int, int action, int) {
    auto* self = static_cast<RendererGlfwOpengl*>(glfwGetWindowUserPointer(window));
    if (!self || key < 0 || key >= 512) return;
    self->inputMutable().m_keys[key] = (action != GLFW_RELEASE);
}

void RendererGlfwOpengl::mouseButtonCallback(GLFWwindow* window, int button, int action, int) {
    auto* self = static_cast<RendererGlfwOpengl*>(glfwGetWindowUserPointer(window));
    if (!self || button < 0 || button >= 8) return;
    self->inputMutable().m_mouseButtons[button] = (action == GLFW_PRESS);
}

void RendererGlfwOpengl::cursorPosCallback(GLFWwindow* window, double x, double y) {
    auto* self = static_cast<RendererGlfwOpengl*>(glfwGetWindowUserPointer(window));
    if (!self) return;
    self->inputMutable().m_mouseX = x;
    self->inputMutable().m_mouseY = y;
}

void RendererGlfwOpengl::scrollCallback(GLFWwindow* window, double /*dx*/, double dy) {
    auto* self = static_cast<RendererGlfwOpengl*>(glfwGetWindowUserPointer(window));
    if (!self) return;
    self->inputMutable().m_scrollOffset = dy;
}

void RendererGlfwOpengl::windowSizeCallback(GLFWwindow* window, int width, int height) {
    auto* self = static_cast<RendererGlfwOpengl*>(glfwGetWindowUserPointer(window));
    if (!self) return;
    self->onWindowResize(width, height);
}