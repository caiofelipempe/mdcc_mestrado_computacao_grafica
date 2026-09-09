#include "renderer.hpp"

#include <imgui.h>

class Trabalho01 : public Renderer {
public:
    Trabalho01() = default;

protected:
    void onInit(int, int, const std::string&) override {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    void onUI() override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::Begin(
            "DockSpace",
            nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus
        );

        ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");

        ImGui::DockSpace(
            dockspaceId,
            ImVec2(0.0f, 0.0f)
        );

        ImGui::End();

        drawCanvas();
        drawPanel();
    }

private:
    void drawCanvas() {
        ImGui::Begin("Canvas");

        ImGui::Text("Canvas");

        ImGui::End();
    }

    void drawPanel() {
        ImGui::Begin("Panel");

        ImGui::Text("Panel");

        ImGui::End();
    }
};

int main() {
    Trabalho01 app;

    app.run(
        800,
        600,
        "Modelador Geometrico"
    );
}