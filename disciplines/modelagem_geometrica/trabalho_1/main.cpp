#include "renderer_glfw_opengl.hpp"

#include <array>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "shape.hpp"

using namespace geometry;

namespace
{
    constexpr float kOrbitSensitivity = 0.25f;
    constexpr float kFovDegrees = 45.0f;
    constexpr float kNearPlane = 0.1f;
    constexpr float kFarPlane = 1000.0f;
}

class Trabalho01 : public RendererGlfwOpengl
{

public:
    Trabalho01() = default;

protected:
    void onInit(
        int width,
        int height,
        const std::string &) override
    {
        initImGui();

        ImGui::GetIO().ConfigFlags |=
            ImGuiConfigFlags_DockingEnable;

        camera().setPerspective(
            kFovDegrees,
            static_cast<float>(width) / height,
            kNearPlane,
            kFarPlane);
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
                static_cast<float>(dy) * kOrbitSensitivity);
        }

        if (input().m_scrollOffset != 0.0)
        {
            camera().zoom(
                static_cast<float>(
                    input().m_scrollOffset));
        }
    }

    void onRender(Drawer &drawer) override
    {
        imguiStartRender();
        drawScene(drawer);
        imguiEndRender();
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
            ImGuiDockNodeFlags_PassthruCentralNode);

        drawToolsPanel();
        drawHierarchyPanel();
        drawPropertiesPanel();
    }

    // Antes eram 12 chamadas manuais a drawFace, uma por triângulo, com o
    // winding da face "Trás" escrito na ordem contrária às demais (sem
    // comentário explicando por quê). Substituído por uma tabela de faces
    // com convenção única (a,b,c)+(a,c,d), então toda face segue a mesma
    // regra e o outward winding fica implícito na ordem dos índices.
    void drawScene(Drawer &drawer)
    {
        using namespace geometry;

        drawer.drawMesh(
            Block(2.0f, 2.0f, 2.0f).toMesh({-10.0f, 0.0f, 0.0f}),
            Color::Red());

        drawer.drawMesh(
            HalfBlock(2.0f, 2.0f, 2.0f).toMesh({-6.0f, 0.0f, 0.0f}),
            Color::Green());

        drawer.drawMesh(
            Sphere(1.5f).toMesh(
                32,
                {3.0f, 0.0f, 0.0f}),
            Color::Yellow());

        drawer.drawMesh(
            Cylinder(1.0f, 3.0f).toMesh(32, {8.0f, 0.0f, 0.0f}),
            Color::Cyan());

        drawer.drawMesh(
            Cone(1.0f, 3.0f).toMesh(32, {13.0f, 0.0f, 0.0f}),
            Color::White());
    }

    void drawToolsPanel()
    {
        ImGui::Begin("Ferramentas");

        ImGui::Button("Vertice");
        ImGui::Button("Aresta");
        ImGui::Button("Face");

        ImGui::End();
    }

    void drawHierarchyPanel()
    {
        ImGui::Begin("Hierarquia");

        ImGui::Text("Objetos");

        ImGui::End();
    }

    void drawPropertiesPanel()
    {
        ImGui::Begin("Propriedades");

        ImGui::Text("Propriedades do objeto");

        ImGui::End();
    }

    void initImGui()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window(), true);
        ImGui_ImplOpenGL3_Init("#version 330");
    }

    void imguiStartRender()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        renderUI();
    }

    void imguiEndRender()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void shutdownImGui()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
};

int main()
{
    Trabalho01 app;

    app.run(
        1280,
        720,
        "Modelador Geometrico");
}