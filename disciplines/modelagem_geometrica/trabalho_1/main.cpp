#include "renderer_glfw_opengl.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <string>
#include <type_traits>
#include <variant>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_stdlib.h>

#include "shape.hpp"
#include "octree.hpp"

using namespace geometry;

namespace
{
    constexpr float kOrbitSensitivity = 0.25f;
    constexpr float kFovDegrees = 45.0f;
    constexpr float kNearPlane = 0.1f;
    constexpr float kFarPlane = 1000.0f;

    constexpr float kDragSpeed = 0.1f;
    constexpr float kDragMin = 0.1f;
    constexpr float kDragMax = 100.0f;

    // Compartilhada por qualquer forma que se testa contra a AABB de um nó
    // (esfera, bloco, cilindro): monta a octree a partir do boundSize() da
    // forma e de uma função de teste, promovendo Branch -> Filled quando a
    // profundidade máxima é atingida.
    template <typename TestFn>
    Octree buildOctreeFromTest(Vec3f boundSize, int maxDepth, TestFn &&test)
    {
        const auto half = boundSize * 0.5f;

        Octree tree(
            {-half[0], -half[1], -half[2]},
            {half[0], half[1], half[2]});

        tree.build(
            [&test, maxDepth](const AABB &bounds, std::size_t depth)
            {
                const auto result = test(bounds);

                if (depth >= static_cast<std::size_t>(maxDepth) &&
                    result == OctreeState::Branch)
                {
                    return OctreeState::Filled;
                }

                return result;
            });

        return tree;
    }

    // -------------------------------------------------------------------
    // Um editor por seleção do combo. Cada um sabe: (1) desenhar seus
    // próprios controles ImGui, (2) construir sua Octree. Nenhuma forma
    // conhece as outras; adicionar uma nova é só adicionar uma struct e
    // uma linha no std::variant abaixo.
    // -------------------------------------------------------------------

    struct SphereEditor
    {
        static constexpr const char *kLabel = "Esfera";

        Sphere shape{10.0f};

        bool drawUI()
        {
            return ImGui::DragFloat(
                "Raio", &shape.radius(), kDragSpeed, kDragMin, kDragMax);
        }

        Octree build(int maxDepth) const
        {
            return buildOctreeFromTest(
                shape.boundSize(), maxDepth,
                [this](const AABB &bounds) { return test(bounds); });
        }

    private:
        OctreeState test(const AABB &aabb) const
        {
            const auto min = aabb.minimum();
            const auto max = aabb.maximum();

            const Vec3 furthest{
                (std::abs(min[0]) > std::abs(max[0])) ? min[0] : max[0],
                (std::abs(min[1]) > std::abs(max[1])) ? min[1] : max[1],
                (std::abs(min[2]) > std::abs(max[2])) ? min[2] : max[2]};

            const auto sqrRadius = shape.radius() * shape.radius();

            if (furthest.dot(furthest) <= sqrRadius)
            {
                return OctreeState::Filled;
            }

            const Vec3 closest{
                std::clamp(0.0f, min[0], max[0]),
                std::clamp(0.0f, min[1], max[1]),
                std::clamp(0.0f, min[2], max[2])};

            if (closest.dot(closest) <= sqrRadius)
            {
                return OctreeState::Branch;
            }

            return OctreeState::Empty;
        }
    };

    struct BlockEditor
    {
        static constexpr const char *kLabel = "Bloco";

        Block shape{4.0f, 5.0f, 3.0f};

        bool drawUI()
        {
            float width = shape.boundSize()[0];
            float height = shape.boundSize()[1];
            float depth = shape.boundSize()[2];

            bool changed = false;
            changed |= ImGui::DragFloat("Largura", &width, kDragSpeed, kDragMin, kDragMax);
            changed |= ImGui::DragFloat("Altura", &height, kDragSpeed, kDragMin, kDragMax);
            changed |= ImGui::DragFloat("Profundidade", &depth, kDragSpeed, kDragMin, kDragMax);

            if (changed)
            {
                shape = Block(width, height, depth);
            }

            return changed;
        }

        Octree build(int maxDepth) const
        {
            return buildOctreeFromTest(
                shape.boundSize(), maxDepth,
                [this](const AABB &bounds) { return test(bounds); });
        }

    private:
        OctreeState test(const AABB &aabb) const
        {
            const auto half = shape.boundSize() * 0.5f;
            const auto min = aabb.minimum().to_vector();
            const auto max = aabb.maximum().to_vector();

            const Vec3 furthest{
                (std::abs(min[0]) > std::abs(max[0])) ? min[0] : max[0],
                (std::abs(min[1]) > std::abs(max[1])) ? min[1] : max[1],
                (std::abs(min[2]) > std::abs(max[2])) ? min[2] : max[2]};

            const Vec3 u{1.0f, 0.0f, 0.0f};
            const Vec3 v{0.0f, 1.0f, 0.0f};
            const Vec3 w{0.0f, 0.0f, 1.0f};

            if (std::abs(furthest.dot(u)) <= half[0] &&
                std::abs(furthest.dot(v)) <= half[1] &&
                std::abs(furthest.dot(w)) <= half[2])
            {
                return OctreeState::Filled;
            }

            if (max.dot(u) < -half[0] || min.dot(u) > half[0] ||
                max.dot(v) < -half[1] || min.dot(v) > half[1] ||
                max.dot(w) < -half[2] || min.dot(w) > half[2])
            {
                return OctreeState::Empty;
            }

            return OctreeState::Branch;
        }
    };

    struct CylinderEditor
    {
        static constexpr const char *kLabel = "Cilindro";

        Cylinder shape{5.0f, 5.0f};

        bool drawUI()
        {
            bool changed = false;
            changed |= ImGui::DragFloat("Raio", &shape.radius(), kDragSpeed, kDragMin, kDragMax);
            changed |= ImGui::DragFloat("Altura", &shape.height(), kDragSpeed, kDragMin, kDragMax);
            return changed;
        }

        Octree build(int maxDepth) const
        {
            return buildOctreeFromTest(
                shape.boundSize(), maxDepth,
                [this](const AABB &bounds) { return test(bounds); });
        }

    private:
        OctreeState test(const AABB &aabb) const
        {
            const Vec3 axisY{0.0f, 1.0f, 0.0f};

            const auto sqrRadius = shape.radius() * shape.radius();
            const auto halfHeight = shape.height() * 0.5f;

            const auto min = aabb.minimum();
            const auto max = aabb.maximum();

            const Vec3 furthest{
                (std::abs(min[0]) > std::abs(max[0])) ? min[0] : max[0],
                (std::abs(min[1]) > std::abs(max[1])) ? min[1] : max[1],
                (std::abs(min[2]) > std::abs(max[2])) ? min[2] : max[2]};

            const float furthestHeight = std::abs(furthest.dot(axisY));
            const Vec3 furthestRadial = furthest - (furthest.dot(axisY) * axisY);

            if (furthestRadial.dot(furthestRadial) <= sqrRadius &&
                furthestHeight <= halfHeight)
            {
                return OctreeState::Filled;
            }

            const Vec3 closest{
                std::clamp(0.0f, min[0], max[0]),
                std::clamp(0.0f, min[1], max[1]),
                std::clamp(0.0f, min[2], max[2])};

            const float closestHeight = closest.dot(axisY);
            const Vec3 closestRadial = closest - (closest.dot(axisY) * axisY);

            if (closestRadial.dot(closestRadial) > sqrRadius ||
                closestHeight < -halfHeight ||
                closestHeight > halfHeight)
            {
                return OctreeState::Empty;
            }

            return OctreeState::Branch;
        }
    };

    // A octree é descrita diretamente por uma gramática textual
    // ('0' vazio, '1' preenchido, '{...}' subdividido), em vez de testada
    // contra uma forma geométrica — por isso não compartilha
    // buildOctreeFromTest e define seus próprios limites (AABB).
    struct StringEditor
    {
        static constexpr const char *kLabel = "String";

        std::string text = "{11111111}";
        std::array<float, 3> minBound{-10.0f, -10.0f, -10.0f};
        std::array<float, 3> maxBound{10.0f, 10.0f, 10.0f};

        bool drawUI()
        {
            bool changed = false;

            changed |= ImGui::InputTextMultiline(
                "Octree", &text, ImVec2(0.0f, 150.0f));

            ImGui::Separator();
            ImGui::Text("Gramatica:");
            ImGui::BulletText("0 = No vazio");
            ImGui::BulletText("1 = No preenchido");
            ImGui::BulletText("{...} = No subdividido");

            ImGui::Separator();
            ImGui::Text("Exemplos:");

            changed |= drawExampleButton("Voxel", "1");
            ImGui::SameLine();
            changed |= drawExampleButton("8 Voxels", "{11111111}");
            changed |= drawExampleButton("Nivel 2", "{{11111111}0000000}");
            changed |= drawExampleButton(
                "Exemplo", "{1{01100010}0{0{00010110}011101}0110}");

            ImGui::Separator();
            ImGui::Text("Tamanho do AABB");
            changed |= ImGui::DragFloat3("Min", minBound.data(), kDragSpeed);
            changed |= ImGui::DragFloat3("Max", maxBound.data(), kDragSpeed);

            if (ImGui::Button("Reconstruir"))
            {
                changed = true;
            }

            return changed;
        }

        // Sem parâmetro de escala: os limites já vêm explícitos da UI.
        Octree build(int maxDepth) const
        {
            Octree tree(
                {minBound[0], minBound[1], minBound[2]},
                {maxBound[0], maxBound[1], maxBound[2]});

            std::size_t pos = 0;

            tree.build(
                [this, maxDepth, &pos](const AABB &, std::size_t depth)
                {
                    const auto result = parseNext(pos);

                    if (depth >= static_cast<std::size_t>(maxDepth) &&
                        result == OctreeState::Branch)
                    {
                        return OctreeState::Filled;
                    }

                    return result;
                });

            return tree;
        }

    private:
        bool drawExampleButton(const char *label, const char *pattern)
        {
            if (!ImGui::Button(label))
            {
                return false;
            }

            text = pattern;
            return true;
        }

        OctreeState parseNext(std::size_t &pos) const
        {
            if (pos >= text.size())
            {
                return OctreeState::Empty;
            }

            switch (text[pos++])
            {
            case '0':
                return OctreeState::Empty;
            case '1':
                return OctreeState::Filled;
            case '{':
                return OctreeState::Branch;
            case '}':
            default:
                return OctreeState::Empty;
            }
        }
    };

    using ShapeEditor =
        std::variant<SphereEditor, BlockEditor, CylinderEditor, StringEditor>;

    constexpr std::array<const char *, 4> kShapeLabels{
        SphereEditor::kLabel,
        BlockEditor::kLabel,
        CylinderEditor::kLabel,
        StringEditor::kLabel};

    ShapeEditor makeEditor(std::size_t index)
    {
        switch (index)
        {
        case 0: return SphereEditor{};
        case 1: return BlockEditor{};
        case 2: return CylinderEditor{};
        default: return StringEditor{};
        }
    }
}

class Trabalho01 : public RendererGlfwOpengl
{
public:
    Trabalho01() = default;

protected:
    void onInit(int width, int height, const std::string &) override
    {
        initImGui();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        camera().setPerspective(
            kFovDegrees,
            static_cast<float>(width) / height,
            kNearPlane,
            kFarPlane);

        rebuildMesh();
    }

    void onShutdown() override
    {
        shutdownImGui();
    }

    void onUpdate(float deltaTime) override
    {
        updateCameraOrbit();
        updateCameraMovement(deltaTime);
    }

    void onRender(Drawer &drawer) override
    {
        imguiStartRender();
        drawScene(drawer);
        imguiEndRender();
    }

private:
    ShapeEditor m_editor = SphereEditor{};

    Mesh3f m_mesh;
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;

    bool m_showFaces = true;
    bool m_showEdges = false;
    bool m_showVertices = false;

    Vec3f m_octreeScale{1.0f, 1.0f, 1.0f};
    int m_octreeDepth = 5;

    // ---- Câmera / input --------------------------------------------

    void updateCameraOrbit()
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

        if (input().scrollOffset() != 0.0)
        {
            camera().zoom(static_cast<float>(input().scrollOffset()));
        }
    }

    void updateCameraMovement(float deltaTime)
    {
        constexpr float speed = 10.0f;
        const float amount = speed * deltaTime;

        const auto forward = camera().rotation().forward();
        const auto right = camera().rotation().right();
        const auto up = camera().rotation().up();

        if (input().pressed('W')) camera().move(forward * amount);
        if (input().pressed('S')) camera().move(forward * -amount);
        if (input().pressed('D')) camera().move(right * amount);
        if (input().pressed('A')) camera().move(right * -amount);
        if (input().pressed('E')) camera().move(up * amount);
        if (input().pressed('Q')) camera().move(up * -amount);
    }

    // ---- UI -----------------------------------------------------------

    void renderUI()
    {
        ImGui::DockSpaceOverViewport(
            0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        drawPanel();
    }

    void drawPanel()
    {
        ImGui::Begin("Ferramentas");

        drawShapeSelector();

        bool changed = false;
        changed |= ImGui::DragFloat3(
            "Octree Scale", m_octreeScale.data_ptr(), 0.01f, 0.1f, 100.0f);
        changed |= ImGui::SliderInt("Octree Depth", &m_octreeDepth, 0, 5);

        ImGui::Separator();
        ImGui::Checkbox("Faces", &m_showFaces);
        ImGui::Checkbox("Arestas", &m_showEdges);
        ImGui::Checkbox("Vertices", &m_showVertices);
        ImGui::Separator();

        changed |= std::visit([](auto &editor) { return editor.drawUI(); }, m_editor);

        if (changed)
        {
            rebuildMesh();
        }

        ImGui::Separator();
        ImGui::Text("Vertices: %zu", m_mesh.vertices().size());
        ImGui::Text("Faces: %zu", m_mesh.faces().size());

        ImGui::End();
    }

    void drawShapeSelector()
    {
        int current = static_cast<int>(m_editor.index());

        if (ImGui::Combo(
                "Selecao", &current, kShapeLabels.data(),
                static_cast<int>(kShapeLabels.size())))
        {
            m_editor = makeEditor(static_cast<std::size_t>(current));
            rebuildMesh();
        }
    }

    void drawScene(Drawer &drawer)
    {
        const Color transparent = Color::Transparent();

        if (m_mesh.edges().empty())
        {
            m_mesh.buildEdgesFromFaces();
        }

        drawer.drawMesh(
            m_mesh,
            m_showFaces ? Color::DarkGreen() : transparent,
            m_showEdges ? Color::Yellow() : transparent,
            m_showVertices ? Color::Red() : transparent);
    }

    // ---- Construção da malha ------------------------------------------

    void rebuildMesh()
    {
        m_mesh = std::visit(
            [this](auto &editor) -> Mesh3f
            {
                Octree tree = editor.build(m_octreeDepth);

                using Editor = std::decay_t<decltype(editor)>;
                if constexpr (std::is_same_v<Editor, StringEditor>)
                {
                    return tree.toMesh();
                }
                else
                {
                    return tree.scaleBounds(m_octreeScale).toMesh();
                }
            },
            m_editor);
    }

    // ---- ImGui lifecycle ------------------------------------------------

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
    app.run(1280, 720, "Modelador Geometrico");
}