#include "renderer_glfw_opengl.hpp"

#include <array>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "shape.hpp"
#include "octree.hpp"

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

    void drawScene(Drawer &drawer)
    {
        using namespace geometry;

        Octree tree(
            {-8.f, -8.f, -8.f},
            {8.f, 8.f, 8.f});

        const std::string data =
            "{{01100001}010{00101101}011{11001101}}";

        std::size_t cursor = 0;

        tree.build(
            [&data, &cursor](const AABB &bounds,
               std::size_t depth)
            {
                while (cursor < data.size())
                {
                    const char c =
                        data[cursor++];

                    switch (c)
                    {
                    case '0':
                        return OctreeState::Empty;

                    case '1':
                        return OctreeState::Filled;

                    case '{':
                        return OctreeState::Branch;

                    case '}':
                        continue;
                    }
                }

                return OctreeState::Empty;
            });

        drawer.drawMesh(
            tree.toMesh(),
            Color::Green());
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