#include "renderer_glfw_opengl.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <numbers>
#include <string>
#include <vector>
#include <variant>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>

#include "shape.hpp"
#include "octree.hpp"

using namespace geometry;

namespace
{
    template <typename... Ts>
    struct overloaded : Ts...
    {
        using Ts::operator()...;
    };
    template <typename... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;

    struct HoveredVariants
    {
        struct Canvas
        {
        };

        struct ImGui
        {
            std::string windowName;
        };

        using Variant = std::variant<Canvas, ImGui>;
    };

    constexpr float kOrbitSensitivity = 0.25f;
    constexpr float kFovDegrees = 45.0f;
    constexpr float kNearPlane = 0.1f;
    constexpr float kFarPlane = 1000.0f;

    constexpr float kDragSpeed = 0.1f;
    constexpr float kDragMin = 0.1f;
    constexpr float kDragMax = 100.0f;

    constexpr float kDegToRad = std::numbers::pi_v<float> / 180.0f;

    // -------------------------------------------------------------------
    // Classificacao AABB-vs-forma
    // -------------------------------------------------------------------

    OctreeState classifySphere(const Sphere &shape, const AABB &aabb)
    {
        const auto min = aabb.minimum();
        const auto max = aabb.maximum();

        const Vec3 furthest{
            (std::abs(min[0]) > std::abs(max[0])) ? min[0] : max[0],
            (std::abs(min[1]) > std::abs(max[1])) ? min[1] : max[1],
            (std::abs(min[2]) > std::abs(max[2])) ? min[2] : max[2]};

        const auto sqrRadius = shape.radius() * shape.radius();

        if (furthest.dot(furthest) <= sqrRadius)
            return OctreeState::Filled;

        const Vec3 closest{
            std::clamp(0.0f, min[0], max[0]),
            std::clamp(0.0f, min[1], max[1]),
            std::clamp(0.0f, min[2], max[2])};

        if (closest.dot(closest) <= sqrRadius)
            return OctreeState::Branch;

        return OctreeState::Empty;
    }

    OctreeState classifyBlock(const Block &shape, const AABB &aabb)
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

    OctreeState classifyCylinder(const Cylinder &shape, const AABB &aabb)
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

    OctreeState parseNextStringToken(const std::string &text, std::size_t &pos)
    {
        if (pos >= text.size())
            return OctreeState::Empty;

        switch (text[pos++])
        {
        case '1':
        case 'W':
        case 'B':
        case 'w':
        case 'b':
            return OctreeState::Filled;
        case '(':
            return OctreeState::Branch;
        default:
            return OctreeState::Empty;
        }
    }

    // -------------------------------------------------------------------
    // Tipos de objeto
    // -------------------------------------------------------------------

    enum class ObjectKind
    {
        Sphere,
        Block,
        Cylinder,
        String
    };

    constexpr std::array<const char *, 4> kObjectKindLabels{
        "Esfera",
        "Bloco",
        "Cilindro",
        "String"};

    // Icone textual simples para o outliner.
    const char *kindIcon(ObjectKind kind)
    {
        switch (kind)
        {
        case ObjectKind::Sphere:
            return "(o)";
        case ObjectKind::Block:
            return "[#]";
        case ObjectKind::Cylinder:
            return "(=)";
        case ObjectKind::String:
            return "{ }";
        }
        return "(?)";
    }

    bool drawColorEdit(const char *label, Color &color)
    {
        float rgba[4] = {color.r, color.g, color.b, color.a};

        if (ImGui::ColorEdit4(label, rgba))
        {
            color = Color(rgba[0], rgba[1], rgba[2], rgba[3]);
            return true;
        }

        return false;
    }

    // -------------------------------------------------------------------
    // SceneObject (inalterado na logica; agora so com UI separada em
    // secoes para serem chamadas pelo painel de Propriedades).
    // -------------------------------------------------------------------

    struct SceneObject
    {
        std::string name = "Objeto";

        ObjectKind kind = ObjectKind::Sphere;

        Sphere sphere{5.0f};
        Block block{4.0f, 5.0f, 3.0f};
        Cylinder cylinder{3.0f, 5.0f};
        std::string text = "(WBWBWBWB";

        Vec3f position{0.0f, 0.0f, 0.0f};
        Vec3f rotationDegrees{0.0f, 0.0f, 0.0f};

        Color faceColor = Color::DarkGreen();
        Color edgeColor = Color::Yellow();
        Color vertexColor = Color::Red();

        bool hasTransform() const { return kind != ObjectKind::String; }

        Rot3f rotator() const
        {
            Rot3f rot = Rot3f::identity();
            rot.rotateWorld({1.0f, 0.0f, 0.0f}, rotationDegrees[0] * kDegToRad);
            rot.rotateWorld({0.0f, 1.0f, 0.0f}, rotationDegrees[1] * kDegToRad);
            rot.rotateWorld({0.0f, 0.0f, 1.0f}, rotationDegrees[2] * kDegToRad);
            return rot;
        }

        Vec3f worldToLocal(const Vec3f &world) const
        {
            const auto rot = rotator();
            const auto q = rot.quaternion();
            const Rot3f inverse{Quatf{-q[0], -q[1], -q[2], q[3]}};
            return inverse.rotateVector(world - position);
        }

        OctreeState classifyShape(const AABB &worldBounds) const
        {
            const Vec3f worldMin = worldBounds.minimum().to_vector();
            const Vec3f worldMax = worldBounds.maximum().to_vector();

            Vec3f localMin{};
            Vec3f localMax{};
            bool first = true;

            for (int i = 0; i < 8; ++i)
            {
                const Vec3f worldCorner{
                    (i & 1) ? worldMax[0] : worldMin[0],
                    (i & 2) ? worldMax[1] : worldMin[1],
                    (i & 4) ? worldMax[2] : worldMin[2]};

                const Vec3f local = worldToLocal(worldCorner);

                if (first)
                {
                    localMin = local;
                    localMax = local;
                    first = false;
                }
                else
                {
                    for (int a = 0; a < 3; ++a)
                    {
                        localMin[a] = std::min(localMin[a], local[a]);
                        localMax[a] = std::max(localMax[a], local[a]);
                    }
                }
            }

            AABB localBounds;
            localBounds.minimum() = Point3f{localMin[0], localMin[1], localMin[2]};
            localBounds.maximum() = Point3f{localMax[0], localMax[1], localMax[2]};

            switch (kind)
            {
            case ObjectKind::Sphere:
                return classifySphere(sphere, localBounds);
            case ObjectKind::Block:
                return classifyBlock(block, localBounds);
            case ObjectKind::Cylinder:
                return classifyCylinder(cylinder, localBounds);
            default:
                return OctreeState::Empty;
            }
        }

        Octree buildOctree(const Point3f &worldMin, const Point3f &worldMax, int maxDepth) const
        {
            Octree tree(worldMin, worldMax);

            if (kind == ObjectKind::String)
            {
                std::size_t pos = 0;

                tree.build(
                    [this, maxDepth, &pos](const AABB &, std::size_t depth)
                    {
                        const auto result = parseNextStringToken(text, pos);

                        if (depth >= static_cast<std::size_t>(maxDepth) &&
                            result == OctreeState::Branch)
                        {
                            return OctreeState::Filled;
                        }

                        return result;
                    });

                return tree;
            }

            tree.build(
                [this, maxDepth](const AABB &bounds, std::size_t depth)
                {
                    const auto result = classifyShape(bounds);

                    if (depth >= static_cast<std::size_t>(maxDepth) &&
                        result == OctreeState::Branch)
                    {
                        return OctreeState::Filled;
                    }

                    return result;
                });

            return tree;
        }

        // ---- Secoes de UI ------------------------------------------------

        // Parametros da forma (inclui o seletor de tipo).
        bool drawShapeUI()
        {
            bool changed = false;

            int current = static_cast<int>(kind);
            if (ImGui::Combo("Tipo", &current, kObjectKindLabels.data(),
                             static_cast<int>(kObjectKindLabels.size())))
            {
                kind = static_cast<ObjectKind>(current);
                changed = true;
            }

            ImGui::Spacing();

            switch (kind)
            {
            case ObjectKind::Sphere:
                changed |= ImGui::DragFloat(
                    "Raio", &sphere.radius(), kDragSpeed, kDragMin, kDragMax);
                break;

            case ObjectKind::Block:
            {
                float width = block.boundSize()[0];
                float height = block.boundSize()[1];
                float depth = block.boundSize()[2];

                bool blockChanged = false;
                blockChanged |= ImGui::DragFloat("Largura", &width, kDragSpeed, kDragMin, kDragMax);
                blockChanged |= ImGui::DragFloat("Altura", &height, kDragSpeed, kDragMin, kDragMax);
                blockChanged |= ImGui::DragFloat("Profundidade", &depth, kDragSpeed, kDragMin, kDragMax);

                if (blockChanged)
                {
                    block = Block(width, height, depth);
                    changed = true;
                }
                break;
            }

            case ObjectKind::Cylinder:
                changed |= ImGui::DragFloat(
                    "Raio", &cylinder.radius(), kDragSpeed, kDragMin, kDragMax);
                changed |= ImGui::DragFloat(
                    "Altura", &cylinder.height(), kDragSpeed, kDragMin, kDragMax);
                break;

            case ObjectKind::String:
                changed |= ImGui::InputTextMultiline("Padrao", &text, ImVec2(0.0f, 90.0f));
                ImGui::TextDisabled("W/B = preenchido/vazio, '(' = subdividir");
                break;
            }

            return changed;
        }

        // Transformacao (posicao + rotacao).
        bool drawTransformUI()
        {
            bool changed = false;

            changed |= ImGui::DragFloat3("Posicao", position.data_ptr(), kDragSpeed);
            changed |= ImGui::DragFloat3("Rotacao (graus)", rotationDegrees.data_ptr(), 1.0f);

            if (ImGui::SmallButton("Resetar"))
            {
                position = Vec3f{0.0f, 0.0f, 0.0f};
                rotationDegrees = Vec3f{0.0f, 0.0f, 0.0f};
                changed = true;
            }

            return changed;
        }

        // Cores.
        bool drawColorsUI()
        {
            bool changed = false;
            changed |= drawColorEdit("Face", faceColor);
            changed |= drawColorEdit("Aresta", edgeColor);
            changed |= drawColorEdit("Vertice", vertexColor);
            return changed;
        }
    };

    // -------------------------------------------------------------------
    // Tema escuro estilo
    // -------------------------------------------------------------------

    void applyStyle()
    {
        ImGuiStyle &style = ImGui::GetStyle();

        style.WindowPadding = ImVec2(8.0f, 8.0f);
        style.FramePadding = ImVec2(6.0f, 3.0f);
        style.ItemSpacing = ImVec2(6.0f, 5.0f);
        style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
        style.IndentSpacing = 18.0f;
        style.ScrollbarSize = 12.0f;
        style.GrabMinSize = 10.0f;

        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.PopupBorderSize = 1.0f;

        style.WindowRounding = 4.0f;
        style.FrameRounding = 3.0f;
        style.PopupRounding = 3.0f;
        style.ScrollbarRounding = 3.0f;
        style.GrabRounding = 3.0f;
        style.TabRounding = 4.0f;

        ImVec4 *c = style.Colors;
        c[ImGuiCol_WindowBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        c[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
        c[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.98f);
        c[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        c[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
        c[ImGuiCol_HeaderActive] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
        c[ImGuiCol_Button] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        c[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        c[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        c[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        c[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
        c[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
        c[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        c[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        c[ImGuiCol_SliderGrabActive] = ImVec4(0.36f, 0.69f, 1.00f, 1.00f);
        c[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
        c[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        c[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        c[ImGuiCol_TabActive] = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
        c[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        c[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.30f, 0.48f, 1.00f);
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

        m_width = width;
        m_height = height;

        applyCameraProjection(); // antes: camera().setPerspective(kFovDegrees, ...)

        SceneObject first;
        first.name = "Esfera 1";
        m_objects.push_back(first);
        m_selectedIndex = 0;

        rebuildMeshes();
    }

    void onShutdown() override
    {
        shutdownImGui();
    }

    void onWindowResize(
        int width,
        int height) override
    {
        m_width = width;
        m_height = height;
    }

    void onUpdate(float deltaTime) override
    {
        updateCameraOrbit();
        updateCameraMovement(deltaTime);
    }

    void onRender(Drawer &drawer) override
    {
        drawScene(drawer);
    }

    void onUI() override
    {
        imguiStartRender();
        imguiEndRender();
    }

private:
    int m_width = 1280;
    int m_height = 720;

    std::vector<SceneObject> m_objects;
    std::vector<Mesh3f> m_meshes;

    int m_selectedIndex = -1;
    bool m_needRebuild = false;

    Vec3f m_worldMin{-10.0f, -10.0f, -10.0f};
    Vec3f m_worldMax{10.0f, 10.0f, 10.0f};

    Vec3f m_octreeScale{1.0f, 1.0f, 1.0f};
    int m_octreeDepth = 5;

    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;

    bool m_showFaces = true;
    bool m_showEdges = false;
    bool m_showVertices = false;
    HoveredVariants::Variant m_hoveredPanel = HoveredVariants::Canvas{};

    // ---- Membros - Camera ------------------------------------------
    float m_cameraFov = kFovDegrees;
    float m_cameraNear = kNearPlane;
    float m_cameraFar = kFarPlane;

    float m_orbitSensitivity = kOrbitSensitivity; // ja existia como const
    float m_zoomSensitivity = 1.0f;
    float m_moveSpeed = 10.0f;

    // ---- Câmera / input --------------------------------------------

    void updateCameraOrbit()
    {
        if (!std::holds_alternative<HoveredVariants::Canvas>(m_hoveredPanel))
            return;

        const double dx = input().m_mouseX - m_lastMouseX;
        const double dy = input().m_mouseY - m_lastMouseY;

        m_lastMouseX = input().m_mouseX;
        m_lastMouseY = input().m_mouseY;

        if (input().rightMouse())
        {
            camera().orbit(
                static_cast<float>(dx) * m_orbitSensitivity,  // <- membro
                static_cast<float>(dy) * m_orbitSensitivity); // <- membro
        }

        if (input().scrollOffset() != 0.0)
        {
            camera().zoom(
                static_cast<float>(input().scrollOffset()) * m_zoomSensitivity);
        }
    }

    void updateCameraMovement(float deltaTime)
    {
        if (!std::holds_alternative<HoveredVariants::Canvas>(m_hoveredPanel))
            return;

        const float amount = m_moveSpeed * deltaTime; // <- membro

        const auto forward = camera().rotation().forward();
        const auto right = camera().rotation().right();
        const auto up = camera().rotation().up();

        if (input().pressed('W'))
            camera().move(forward * amount);
        if (input().pressed('S'))
            camera().move(forward * -amount);
        if (input().pressed('D'))
            camera().move(right * amount);
        if (input().pressed('A'))
            camera().move(right * -amount);
        if (input().pressed('E'))
            camera().move(up * amount);
        if (input().pressed('Q'))
            camera().move(up * -amount);
    }

    void applyCameraProjection()
    {
        const float aspect = (m_height > 0)
                                 ? static_cast<float>(m_width) / static_cast<float>(m_height)
                                 : 1.0f;

        camera().setPerspective(m_cameraFov, aspect, m_cameraNear, m_cameraFar);
    }

    // ---- UI -----------------------------------------------------------

    void renderUI()
    {
        ImGui::DockSpaceOverViewport(
            0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        drawOutlinerPanel();
        drawPropertiesPanel();

        ImGuiContext *ctx = ImGui::GetCurrentContext();
        ImGuiWindow *w = ctx->HoveredWindow;

        if (w == nullptr)
        {
            m_hoveredPanel = HoveredVariants::Canvas{};
        }
        else
        {
            while (w->ParentWindow)
                w = w->ParentWindow;

            m_hoveredPanel = HoveredVariants::ImGui{std::string(w->Name)};
        }

        if (m_needRebuild)
        {
            rebuildMeshes();
            m_needRebuild = false;
        }
    }

    // ---- Painel "Outliner" --------------------------------------------

    void drawOutlinerPanel()
    {
        ImGui::SetNextWindowSize(ImVec2(260.0f, 480.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Outliner");

        // Cabecalho: raiz "Cena"
        if (ImGui::TreeNodeEx("Cena",
                              ImGuiTreeNodeFlags_DefaultOpen |
                                  ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            // Botao "+ Adicionar" com popup de tipo
            if (ImGui::Button("+ Adicionar Objeto", ImVec2(-1.0f, 0.0f)))
            {
                ImGui::OpenPopup("AddObjectPopup");
            }

            if (ImGui::BeginPopup("AddObjectPopup"))
            {
                for (int i = 0; i < static_cast<int>(kObjectKindLabels.size()); ++i)
                {
                    if (ImGui::MenuItem(kObjectKindLabels[i]))
                    {
                        SceneObject obj;
                        obj.kind = static_cast<ObjectKind>(i);
                        obj.name = std::string(kObjectKindLabels[i]) + " " +
                                   std::to_string(m_objects.size() + 1);

                        if (obj.kind == ObjectKind::String)
                            obj.text = "(WBWBWBWB";

                        m_objects.push_back(obj);
                        m_selectedIndex = static_cast<int>(m_objects.size()) - 1;
                        m_needRebuild = true;
                    }
                }
                ImGui::EndPopup();
            }

            ImGui::Separator();

            // Lista de objetos
            ImGui::BeginChild("ObjectList", ImVec2(0.0f, 0.0f), false);

            int pendingRemove = -1;
            int pendingDuplicate = -1;

            for (int i = 0; i < static_cast<int>(m_objects.size()); ++i)
            {
                SceneObject &obj = m_objects[i];

                ImGui::PushID(i);

                const bool isSelected = (m_selectedIndex == i);
                const std::string label =
                    std::string(kindIcon(obj.kind)) + "  " + obj.name;

                if (ImGui::Selectable(label.c_str(), isSelected))
                {
                    m_selectedIndex = i;
                }

                if (ImGui::BeginPopupContextItem("object_ctx"))
                {
                    if (ImGui::MenuItem("Duplicar"))
                        pendingDuplicate = i;
                    if (ImGui::MenuItem("Remover"))
                        pendingRemove = i;
                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }

            if (pendingDuplicate >= 0)
            {
                SceneObject copy = m_objects[pendingDuplicate];
                copy.name += " copia";
                m_objects.insert(m_objects.begin() + pendingDuplicate + 1, copy);
                m_selectedIndex = pendingDuplicate + 1;
                m_needRebuild = true;
            }

            if (pendingRemove >= 0)
            {
                m_objects.erase(m_objects.begin() + pendingRemove);

                if (m_selectedIndex == pendingRemove)
                {
                    m_selectedIndex = m_objects.empty()
                                          ? -1
                                          : std::min(pendingRemove,
                                                     static_cast<int>(m_objects.size()) - 1);
                }
                else if (m_selectedIndex > pendingRemove)
                {
                    --m_selectedIndex;
                }

                m_needRebuild = true;
            }

            ImGui::EndChild();
            ImGui::TreePop();
        }

        ImGui::End();
    }

    // ---- Painel "Propriedades" ----------------------------------------

    void drawPropertiesPanel()
    {
        ImGui::SetNextWindowSize(ImVec2(340.0f, 540.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Propriedades");

        bool changed = false;

        if (ImGui::BeginTabBar("PropertiesTabs"))
        {
            if (ImGui::BeginTabItem("Cena"))
            {
                changed |= drawSceneSettingsUI();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Objeto"))
            {
                changed |= drawObjectSettingsUI();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Camera")) // <-- nova aba
            {
                changed |= drawCameraSettingsUI();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        if (changed)
            m_needRebuild = true;

        ImGui::End();
    }

    bool drawCameraSettingsUI()
    {
        bool changed = false;
        auto &cam = camera();

        // ---- Projecao --------------------------------------------------
        if (ImGui::CollapsingHeader("Projecao", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool projChanged = false;
            projChanged |= ImGui::SliderFloat(
                "FOV (graus)", &m_cameraFov, 10.0f, 120.0f, "%.1f");

            projChanged |= ImGui::DragFloat(
                "Near", &m_cameraNear, 0.001f,
                0.001f, m_cameraFar - 0.01f, "%.3f");

            projChanged |= ImGui::DragFloat(
                "Far", &m_cameraFar, 1.0f,
                m_cameraNear + 0.01f, 10000.0f, "%.1f");

            if (projChanged)
            {
                applyCameraProjection();
                changed = true;
            }

            ImGui::TextDisabled("Aspect: %.3f",
                                (m_height > 0) ? static_cast<float>(m_width) / m_height : 1.0f);
        }

        // ---- Controle --------------------------------------------------
        if (ImGui::CollapsingHeader("Controle", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat("Sens. Orbita", &m_orbitSensitivity,
                               0.01f, 2.0f, "%.2f");
            ImGui::SliderFloat("Sens. Zoom", &m_zoomSensitivity,
                               0.10f, 5.0f, "%.2f");
            ImGui::SliderFloat("Vel. Movimento (WASD)", &m_moveSpeed,
                               1.0f, 50.0f, "%.1f");
        }

        // ---- Estado atual ----------------------------------------------
        if (ImGui::CollapsingHeader("Estado", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const auto p = cam.position();
            ImGui::Text("Posicao:  %.3f, %.3f, %.3f", p[0], p[1], p[2]);

            const auto f = cam.rotation().forward();
            ImGui::Text("Forward:  %.3f, %.3f, %.3f", f[0], f[1], f[2]);

            const auto u = cam.rotation().up();
            ImGui::Text("Up:       %.3f, %.3f, %.3f", u[0], u[1], u[2]);
        }

        // ---- Reset -----------------------------------------------------
        ImGui::Separator();

        if (ImGui::Button("Resetar camera", ImVec2(-1.0f, 0.0f)))
        {
            m_cameraFov = kFovDegrees;
            m_cameraNear = kNearPlane;
            m_cameraFar = kFarPlane;
            m_orbitSensitivity = kOrbitSensitivity;
            m_zoomSensitivity = 1.0f;
            m_moveSpeed = 10.0f;

            applyCameraProjection();
            changed = true;
        }

        return changed;
    }

    // ---- Secao "Cena" --------------------------------------------------

    bool drawSceneSettingsUI()
    {
        bool changed = false;

        if (ImGui::CollapsingHeader("Espaco Global", ImGuiTreeNodeFlags_DefaultOpen))
        {
            changed |= ImGui::DragFloat3("Minimo", m_worldMin.data_ptr(), kDragSpeed);
            changed |= ImGui::DragFloat3("Maximo", m_worldMax.data_ptr(), kDragSpeed);
            ImGui::TextDisabled("Bounds compartilhados por todas as octrees.");
        }

        if (ImGui::CollapsingHeader("Octree", ImGuiTreeNodeFlags_DefaultOpen))
        {
            changed |= ImGui::DragFloat3(
                "Escala", m_octreeScale.data_ptr(), 0.01f, 0.1f, 100.0f);
            changed |= ImGui::SliderInt("Profundidade", &m_octreeDepth, 0, 5);
        }

        if (ImGui::CollapsingHeader("Exibicao", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Faces", &m_showFaces);
            ImGui::Checkbox("Arestas", &m_showEdges);
            ImGui::Checkbox("Vertices", &m_showVertices);
        }

        if (ImGui::CollapsingHeader("Estatisticas"))
        {
            std::size_t totalVertices = 0;
            std::size_t totalFaces = 0;

            for (const auto &mesh : m_meshes)
            {
                totalVertices += mesh.vertices().size();
                totalFaces += mesh.faces().size();
            }

            ImGui::Text("Objetos: %zu", m_objects.size());
            ImGui::Text("Vertices: %zu", totalVertices);
            ImGui::Text("Faces: %zu", totalFaces);
        }

        return changed;
    }

    // ---- Secao "Objeto" ------------------------------------------------

    bool drawObjectSettingsUI()
    {
        if (m_selectedIndex < 0 ||
            m_selectedIndex >= static_cast<int>(m_objects.size()))
        {
            ImGui::Spacing();
            ImGui::TextDisabled("Nenhum objeto selecionado.");
            ImGui::Spacing();
            ImGui::TextWrapped(
                "Selecione um objeto no Outliner para editar suas propriedades.");
            return false;
        }

        SceneObject &obj = m_objects[m_selectedIndex];
        bool changed = false;

        // Cabecalho do objeto: icone + nome editavel
        ImGui::Text("%s", kindIcon(obj.kind));
        ImGui::SameLine();

        char nameBuf[128];
        std::snprintf(nameBuf, sizeof(nameBuf), "%s", obj.name.c_str());

        if (ImGui::InputText("##name", nameBuf, sizeof(nameBuf)))
            obj.name = nameBuf;

        ImGui::Separator();

        if (obj.hasTransform())
        {
            if (ImGui::CollapsingHeader("Transformacao",
                                        ImGuiTreeNodeFlags_DefaultOpen))
            {
                changed |= obj.drawTransformUI();
            }
        }

        if (ImGui::CollapsingHeader("Forma", ImGuiTreeNodeFlags_DefaultOpen))
        {
            changed |= obj.drawShapeUI();
        }

        if (ImGui::CollapsingHeader("Cores", ImGuiTreeNodeFlags_DefaultOpen))
        {
            changed |= obj.drawColorsUI();
        }

        return changed;
    }

    // ---- Renderizacao da cena -----------------------------------------

    void drawScene(Drawer &drawer)
    {
        const Color transparent = Color::Transparent();

        for (std::size_t i = 0;
             i < m_objects.size() && i < m_meshes.size(); ++i)
        {
            const auto &object = m_objects[i];
            Mesh3f &mesh = m_meshes[i];

            if (mesh.edges().empty())
                mesh.buildEdgesFromFaces();

            drawer.drawMesh(
                mesh,
                m_showFaces ? object.faceColor : transparent,
                m_showEdges ? object.edgeColor : transparent,
                m_showVertices ? object.vertexColor : transparent);
        }
    }

    // ---- Construcao das malhas -----------------------------------------

    void rebuildMeshes()
    {
        m_meshes.clear();
        m_meshes.reserve(m_objects.size());

        const Point3f worldMin{m_worldMin[0], m_worldMin[1], m_worldMin[2]};
        const Point3f worldMax{m_worldMax[0], m_worldMax[1], m_worldMax[2]};

        for (const auto &object : m_objects)
        {
            Octree tree = object.buildOctree(worldMin, worldMax, m_octreeDepth);

            Mesh3f mesh = tree.scaleBounds(m_octreeScale).toMesh();

            if (mesh.edges().empty())
                mesh.buildEdgesFromFaces();

            m_meshes.push_back(std::move(mesh));
        }
    }

    // ---- ImGui lifecycle ------------------------------------------------

    void initImGui()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        applyStyle();

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