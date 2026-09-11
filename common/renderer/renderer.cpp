#include <GL/glew.h>

#include "renderer.hpp"
#include "input.h"

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
Renderer::Renderer()  = default;
Renderer::~Renderer() = default;

// ─────────────────────────────────────────────────────────────────────────────
//  GLFW
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::initGLFW(const int w, const int h, const std::string& t) {
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
void Renderer::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void Renderer::shutdownImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Renderer::updateCamera() {
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
    // (removida a linha redundante que zerava m_viewDirty de novo fora do if)
}

void Renderer::updateGamepad() {
    m_input.m_gamepadConnected = glfwJoystickPresent(GLFW_JOYSTICK_1);

    if (!m_input.m_gamepadConnected) {
        m_input.m_gamepadButtons.fill(false);
        m_input.m_gamepadAxes.fill(0.0f);
        return;
    }

    GLFWgamepadstate state;
    if (!glfwGetGamepadState(GLFW_JOYSTICK_1, &state))
        return;

    for (int i = 0; i < 15; ++i)
        m_input.m_gamepadButtons[i] = state.buttons[i] == GLFW_PRESS;

    for (int i = 0; i < 6; ++i)
        m_input.m_gamepadAxes[i] = state.axes[i];
}

// ─────────────────────────────────────────────────────────────────────────────
//  Loop principal
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::run(const int w, const int h, const std::string& t) {
    initGLFW(w, h, t);
    initImGui();

    // Garante shutdown correto mesmo se onInit/onUpdate/onRender lançarem.
    struct ShutdownGuard {
        Renderer* self;
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

    while (!glfwWindowShouldClose(m_window)) {
        auto currentTime = clock::now();
        const float dt = std::chrono::duration<float>(currentTime - lastTime).count();
        if (m_targetFPS > 0)
        {
            const auto targetFrameTime =
                std::chrono::duration<float>(
                    1.0f / static_cast<float>(
                        m_targetFPS
                    )
                );

            const auto frameDuration =
                clock::now() - currentTime;

            if (frameDuration < targetFrameTime)
            {
                std::this_thread::sleep_for(
                    targetFrameTime - frameDuration
                );
            }
        }

        lastTime = currentTime;

        glfwPollEvents();
        updateGamepad();
        onUpdate(dt);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        onUI();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        updateCamera();
        onRender();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window);
        m_input.resetFrameData();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Accessors
// ─────────────────────────────────────────────────────────────────────────────
const InputState& Renderer::input() const { return m_input; }

// ─────────────────────────────────────────────────────────────────────────────
//  Virtuais com implementação vazia (base)
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::onInit        (int, int, const std::string&) {}
void Renderer::onUpdate      (float) {}
void Renderer::onRender      () {}
void Renderer::onUI          () {}
void Renderer::onShutdown    () {}

void Renderer::onWindowResize(int width, int height) {
    glViewport(0, 0, width, height);
    m_camera.resize(width, height);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Callbacks GLFW (estáticos)
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::keyCallback(GLFWwindow* window, int key, int, int action, int) {
    auto* self = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (!self || key < 0 || key >= 512) return;
    self->m_input.m_keys[key] = (action != GLFW_RELEASE);
}

void Renderer::mouseButtonCallback(GLFWwindow* window, int button, int action, int) {
    auto* self = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (!self || button < 0 || button >= 8) return;
    self->m_input.m_mouseButtons[button] = (action == GLFW_PRESS);
}

void Renderer::cursorPosCallback(GLFWwindow* window, double x, double y) {
    auto* self = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (!self) return;
    self->m_input.mouseX = x;
    self->m_input.mouseY = y;
}

void Renderer::scrollCallback(GLFWwindow* window, double /*dx*/, double dy) {
    auto* self = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (!self) return;
    self->m_input.scrollOffset = dy;
}

void Renderer::windowSizeCallback(GLFWwindow* window, int width, int height) {
    auto* self = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (!self) return;
    self->onWindowResize(width, height);
}

/* ================= DRAW FUNCTIONS ================= */

namespace {
    inline void emitVertex(const geometry::Point3f& p) {
        glVertex3f(p[0], p[1], p[2]);
    }
    inline void setColor(const geometry::Color& c) {
        glColor4f(c.r, c.g, c.b, c.a);
    }
}

void Renderer::drawVertex(const geometry::Point3f& point, const geometry::Color& color, float size) {
    glPointSize(size);
    setColor(color);

    glBegin(GL_POINTS);
    emitVertex(point);
    glEnd();
}

void Renderer::drawLine(const geometry::Point3f& a, const geometry::Point3f& b,
                         const geometry::Color& color, float width) {
    glLineWidth(width);
    setColor(color);

    glBegin(GL_LINES);
    emitVertex(a);
    emitVertex(b);
    glEnd();
}

void Renderer::drawFace(const geometry::Point3f& a, const geometry::Point3f& b,
                         const geometry::Point3f& c, const geometry::Color& color) {
    setColor(color);

    glBegin(GL_TRIANGLES);
    emitVertex(a);
    emitVertex(b);
    emitVertex(c);
    glEnd();
}

void Renderer::drawMesh(const geometry::Mesh3f& mesh, const geometry::Color& color) {
    const auto& vertices = mesh.getVertices();
    const auto& edges    = mesh.getEdges();
    const auto& faces    = mesh.getFaces();

    if (!faces.empty()) {
        setColor(color);
        glBegin(GL_TRIANGLES);
        for (const auto& face : faces) {
            emitVertex(vertices[face.indices[0]]);
            emitVertex(vertices[face.indices[1]]);
            emitVertex(vertices[face.indices[2]]);
        }
        glEnd();
    }

    if (!edges.empty()) {
        setColor({0.0f, 0.0f, 0.0f, 1.0f});
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        for (const auto& edge : edges) {
            emitVertex(vertices[edge.v1]);
            emitVertex(vertices[edge.v2]);
        }
        glEnd();
    }

    if (!vertices.empty()) {
        setColor({1.0f, 0.0f, 0.0f, 1.0f});
        glPointSize(5.0f);
        glBegin(GL_POINTS);
        for (const auto& vertex : vertices)
            emitVertex(vertex);
        glEnd();
    }
}