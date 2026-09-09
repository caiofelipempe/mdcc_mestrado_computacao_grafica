#include "renderer.hpp"

#include <imgui.h>
#include <imgui_internal.h>

class Trabalho01 : public Renderer {
public:
    Trabalho01() = default;

protected:
    void onUI() override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);

        ImGui::Begin(
            "Main",
            nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse
        );

        float fullHeight = ImGui::GetContentRegionAvail().y;

        drawPanel(fullHeight);

        drawVerticalSplitter(
            leftPanelWidth,
            150.0f,
            200.0f
        );

        ImGui::SameLine();

        drawCanvas(fullHeight);

        ImGui::End();
    }

private:
    float leftPanelWidth = 300.0f;

private:
    void drawPanel(float height) {
        ImGui::BeginChild(
            "Panel",
            ImVec2(leftPanelWidth, height),
            true
        );

        ImGui::Text("Panel");

        ImGui::EndChild();
    }

    void drawCanvas(float height) {
        ImGui::BeginChild(
            "Canvas",
            ImVec2(0, height),
            true
        );

        ImGui::Text("Canvas");

        ImGui::EndChild();
    }

    void drawVerticalSplitter(
        float& leftWidth,
        float minLeft,
        float minRight)
    {
        ImGui::SameLine();

        ImGui::InvisibleButton(
            "##splitter",
            ImVec2(6.0f, -1.0f)
        );

        if (ImGui::IsItemHovered() ||
            ImGui::IsItemActive())
        {
            ImGui::SetMouseCursor(
                ImGuiMouseCursor_ResizeEW
            );
        }

        if (ImGui::IsItemActive()) {
            leftWidth += ImGui::GetIO().MouseDelta.x;
        }

        float total =
            ImGui::GetContentRegionAvail().x +
            leftWidth;

        leftWidth = ImClamp(
            leftWidth,
            minLeft,
            total - minRight
        );
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
