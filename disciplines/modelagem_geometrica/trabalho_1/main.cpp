#include "renderer_glfw_opengl.hpp"

#include <array>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

using namespace geometry;

namespace {
    constexpr float kOrbitSensitivity = 0.25f;
    constexpr float kFovDegrees = 45.0f;
    constexpr float kNearPlane = 0.1f;
    constexpr float kFarPlane = 1000.0f;
}

class Trabalho01 : public RendererGlfwOpengl {

public:
    Trabalho01() = default;

protected:
    void onInit(
        int width,
        int height,
        const std::string&
    ) override
    {
        initImGui();

        ImGui::GetIO().ConfigFlags |=
            ImGuiConfigFlags_DockingEnable;

        camera().setPerspective(
            kFovDegrees,
            static_cast<float>(width) / height,
            kNearPlane,
            kFarPlane
        );
    }

    void onShutdown() override
    {
        shutdownImGui();
    }

    void onUpdate(float) override
    {
        const double dx = input().m_mouseX - m_lastMouseX;
        const double dy = input().m_mouseY - m_lastMouseY;

        m_lastMouseX = input().m_mouseX;
        m_lastMouseY = input().m_mouseY;

        if (input().rightMouse())
        {
            camera().orbit(
                static_cast<float>(dx) * kOrbitSensitivity,
                static_cast<float>(dy) * kOrbitSensitivity
            );
        }

        if (input().m_scrollOffset != 0.0)
        {
            camera().zoom(
                static_cast<float>(
                    input().m_scrollOffset
                )
            );
        }
    }

    void onRender(Drawer& drawer) override
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        renderUI();

        renderScene(drawer);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

private:

    // Estado do mouse do frame anterior. Antes eram `static` locais dentro
    // de onUpdate — movidos para membros para não depender de estado
    // escondido em função e para poderem ser reiniciados em onInit se
    // necessário no futuro (ex.: ao trocar de câmera).
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;

    void renderUI()
    {
        ImGui::DockSpaceOverViewport(
            0,
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode
        );

        drawToolsPanel();
        drawHierarchyPanel();
        drawPropertiesPanel();
    }

    // Antes eram 12 chamadas manuais a drawFace, uma por triângulo, com o
    // winding da face "Trás" escrito na ordem contrária às demais (sem
    // comentário explicando por quê). Substituído por uma tabela de faces
    // com convenção única (a,b,c)+(a,c,d), então toda face segue a mesma
    // regra e o outward winding fica implícito na ordem dos índices.
    void renderScene(Drawer& drawer)
    {
        static const std::array<Point3f, 8> kVertices{{
            {-1.0f, -1.0f, -1.0f}, // 0
            { 1.0f, -1.0f, -1.0f}, // 1
            { 1.0f,  1.0f, -1.0f}, // 2
            {-1.0f,  1.0f, -1.0f}, // 3
            {-1.0f, -1.0f,  1.0f}, // 4
            { 1.0f, -1.0f,  1.0f}, // 5
            { 1.0f,  1.0f,  1.0f}, // 6
            {-1.0f,  1.0f,  1.0f}, // 7
        }};

        struct Face {
            int a, b, c, d;
            Color color;
        };

        static const std::array<Face, 6> kFaces{{
            {4, 5, 6, 7, Color::Red()},    // Frente
            {0, 3, 2, 1, Color::Green()},  // Trás
            {0, 4, 7, 3, Color::Blue()},   // Esquerda
            {1, 2, 6, 5, Color::Cyan()},   // Direita
            {3, 7, 6, 2, Color::Yellow()}, // Topo
            {0, 1, 5, 4, Color::White()},  // Base
        }};

        for (const auto& face : kFaces)
        {
            drawer.drawFace(
                kVertices[face.a], kVertices[face.b], kVertices[face.c],
                face.color
            );
            drawer.drawFace(
                kVertices[face.a], kVertices[face.c], kVertices[face.d],
                face.color
            );
        }
    }

    void drawToolsPanel() {
        ImGui::Begin("Ferramentas");

        ImGui::Button("Vertice");
        ImGui::Button("Aresta");
        ImGui::Button("Face");

        ImGui::End();
    }

    void drawHierarchyPanel() {
        ImGui::Begin("Hierarquia");

        ImGui::Text("Objetos");

        ImGui::End();
    }

    void drawPropertiesPanel() {
        ImGui::Begin("Propriedades");

        ImGui::Text("Propriedades do objeto");

        ImGui::End();
    }

    void initImGui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window(), true);
        ImGui_ImplOpenGL3_Init("#version 330");
    }

    void shutdownImGui() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
};

int main() {
    Trabalho01 app;

    app.run(
        1280,
        720,
        "Modelador Geometrico"
    );
}