#include "renderer.hpp"

#include <imgui.h>

class Trabalho01 : public Renderer {
public:
    Trabalho01() = default;

protected:
    void onInit(
    int width,
    int height,
    const std::string&
    ) override
    {
        ImGui::GetIO().ConfigFlags |=
            ImGuiConfigFlags_DockingEnable;

        camera().setPerspective(
            45.0f,
            static_cast<float>(width) / height,
            0.1f,
            1000.0f
        );
    }

    void onUpdate(float) override
    {
        static double lastX = input().mouseX;
        static double lastY = input().mouseY;

        const double dx =
            input().mouseX - lastX;

        const double dy =
            input().mouseY - lastY;

        lastX = input().mouseX;
        lastY = input().mouseY;

        if (input().rightMouse())
        {
            camera().orbit(
                static_cast<float>(dx) * 0.25f,
                static_cast<float>(-dy) * 0.25f
            );
        }

        if (input().scrollOffset != 0.0)
        {
            camera().zoom(
                static_cast<float>(
                    input().scrollOffset
                )
            );
        }
    }

    void onUI() override
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
    
    void onRender() override
    {
        renderScene();
    }

private:

    void renderScene()
    {
        const geometry::Point3f p0{-1.0f, -1.0f, -1.0f};
        const geometry::Point3f p1{ 1.0f, -1.0f, -1.0f};
        const geometry::Point3f p2{ 1.0f,  1.0f, -1.0f};
        const geometry::Point3f p3{-1.0f,  1.0f, -1.0f};

        const geometry::Point3f p4{-1.0f, -1.0f,  1.0f};
        const geometry::Point3f p5{ 1.0f, -1.0f,  1.0f};
        const geometry::Point3f p6{ 1.0f,  1.0f,  1.0f};
        const geometry::Point3f p7{-1.0f,  1.0f,  1.0f};

        const geometry::Color red   {1,0,0,1};
        const geometry::Color green {0,1,0,1};
        const geometry::Color blue  {0,0,1,1};
        const geometry::Color cyan  {0,1,1,1};
        const geometry::Color yellow{1,1,0,1};
        const geometry::Color white {1,1,1,1};

        // Frente
        drawFace(p4, p5, p6, red);
        drawFace(p4, p6, p7, red);

        // Trás
        drawFace(p0, p2, p1, green);
        drawFace(p0, p3, p2, green);

        // Esquerda
        drawFace(p0, p4, p7, blue);
        drawFace(p0, p7, p3, blue);

        // Direita
        drawFace(p1, p2, p6, cyan);
        drawFace(p1, p6, p5, cyan);

        // Topo
        drawFace(p3, p7, p6, yellow);
        drawFace(p3, p6, p2, yellow);

        // Base
        drawFace(p0, p1, p5, white);
        drawFace(p0, p5, p4, white);
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
};

int main() {
    Trabalho01 app;

    app.run(
        1280,
        720,
        "Modelador Geometrico"
    );
}