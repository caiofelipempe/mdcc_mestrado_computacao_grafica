#include "renderer.hpp"

#include <imgui.h>

class Trabalho01 : public Renderer {
public:
    Trabalho01() = default;

protected:
    void onInit(int, int, const std::string&) override {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    void onUpdate(float) override {
        renderScene();
    }

    void onUI() override {
        createDockSpace();

        drawToolsPanel();
        drawHierarchyPanel();
        drawPropertiesPanel();
    }

private:
    void createDockSpace() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoBackground;

        ImGui::Begin(
            "MainDockSpace",
            nullptr,
            flags
        );

        ImGui::DockSpace(
            ImGui::GetID("DockSpace")
        );

        ImGui::End();
    }

    void renderScene() {
        /*
         * O OpenGL desenha diretamente
         * no fundo da janela inteira.
         *
         * Exemplo:
         *
         * glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
         * glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
         *
         * Desenhe sua geometria aqui.
         */
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