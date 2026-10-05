#include "renderer_glfw_opengl.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>
#include <string>
#include <utility>
#include <vector>

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
    using Coverage = Octree::Coverage;

    constexpr float kOrbitSensitivity = 0.25f;
    constexpr float kFovDegrees = 45.0f;
    constexpr float kNearPlane = 0.1f;
    constexpr float kFarPlane = 1000.0f;

    constexpr float kDragSpeed = 0.1f;
    constexpr float kDragMin = 0.1f;
    constexpr float kDragMax = 100.0f;

    constexpr float kDegToRad = std::numbers::pi_v<float> / 180.0f;

    Vec3f furthestCorner(const AABB &box)
    {
        const auto min = box.minimum();
        const auto max = box.maximum();

        const auto pick = [&](int a)
        { return std::abs(min[a]) > std::abs(max[a]) ? min[a] : max[a]; };

        return Vec3f{pick(0), pick(1), pick(2)};
    }

    Vec3f closestToOrigin(const AABB &box)
    {
        const auto min = box.minimum();
        const auto max = box.maximum();

        const auto pick = [&](int a)
        { return std::clamp(0.0f, min[a], max[a]); };

        return Vec3f{pick(0), pick(1), pick(2)};
    }

    float radialSquared(const Vec3f &v)
    {
        return v[0] * v[0] + v[2] * v[2];
    }

    Coverage classifySphere(const Sphere &shape, const AABB &box)
    {
        const float sqrRadius = shape.radius() * shape.radius();

        const Vec3f farthest = furthestCorner(box);
        if (farthest.dot(farthest) <= sqrRadius)
            return Coverage::Filled;

        const Vec3f closest = closestToOrigin(box);
        if (closest.dot(closest) <= sqrRadius)
            return Coverage::Partial;

        return Coverage::Empty;
    }

    Coverage classifyBlock(const Block &shape, const AABB &box)
    {
        const auto half = shape.boundSize() * 0.5f;
        const Vec3f farthest = furthestCorner(box);

        bool inside = true;
        bool outside = false;

        for (int a = 0; a < 3; ++a)
        {
            inside = inside && std::abs(farthest[a]) <= half[a];
            outside = outside ||
                      box.maximum()[a] < -half[a] ||
                      box.minimum()[a] > half[a];
        }

        if (inside)
            return Coverage::Filled;

        return outside ? Coverage::Empty : Coverage::Partial;
    }

    Coverage classifyCylinder(const Cylinder &shape, const AABB &box)
    {
        const float sqrRadius = shape.radius() * shape.radius();
        const float halfHeight = shape.height() * 0.5f;

        const Vec3f farthest = furthestCorner(box);
        if (radialSquared(farthest) <= sqrRadius &&
            std::abs(farthest[1]) <= halfHeight)
        {
            return Coverage::Filled;
        }

        const Vec3f closest = closestToOrigin(box);
        if (radialSquared(closest) > sqrRadius ||
            std::abs(closest[1]) > halfHeight)
        {
            return Coverage::Empty;
        }

        return Coverage::Partial;
    }

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

    constexpr std::array<const char *, 4> kOrientationLabels{
        "Anti-horario, baixo para cima",
        "Anti-horario, cima para baixo",
        "Horario, baixo para cima",
        "Horario, cima para baixo"};

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

        if (!ImGui::ColorEdit4(label, rgba))
            return false;

        color = Color(rgba[0], rgba[1], rgba[2], rgba[3]);
        return true;
    }

    struct SceneObject
    {
        std::string name = "Objeto";
        ObjectKind kind = ObjectKind::Sphere;

        Sphere sphere{5.0f};
        Block block{4.0f, 5.0f, 3.0f};
        Cylinder cylinder{3.0f, 5.0f};
        std::string text = "(WBWBWBWB";

        AABB bounds{{-10.0f, -10.0f, -10.0f}, {10.0f, 10.0f, 10.0f}};
        Octree::Orientation orientation = Octree::Orientation::CounterClockwiseBottomToTop;

        Vec3f position{0.0f, 0.0f, 0.0f};
        Vec3f rotationDegrees{0.0f, 0.0f, 0.0f};

        Color faceColor = Color::DarkGreen();
        Color edgeColor = Color::Yellow();
        Color vertexColor = Color::Red();

        bool hasTransform() const { return kind != ObjectKind::String; }

        AABB scaledBounds(const Vec3f &scale) const
        {
            AABB scaled = bounds;

            for (int a = 0; a < 3; ++a)
            {
                scaled.minimum()[a] *= scale[a];
                scaled.maximum()[a] *= scale[a];
            }

            return scaled;
        }

        Octree buildOctree(int maxDepth) const
        {
            return buildOctree(maxDepth, orientation);
        }

        Octree buildOctree(int maxDepth, Octree::Orientation treeOrientation) const
        {
            const auto depth = static_cast<std::size_t>(maxDepth);

            if (kind == ObjectKind::String)
                return Octree::fromString(text, depth, treeOrientation);

            return Octree::fromShape(
                depth,
                [this](const AABB &unitBox)
                { return classifyShape(unitBox); },
                treeOrientation);
        }

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

            int currentOrientation = static_cast<int>(orientation);
            if (ImGui::Combo("Orientacao", &currentOrientation, kOrientationLabels.data(),
                             static_cast<int>(kOrientationLabels.size())))
            {
                orientation = static_cast<Octree::Orientation>(currentOrientation);
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

        bool drawColorsUI()
        {
            bool changed = false;
            changed |= drawColorEdit("Face", faceColor);
            changed |= drawColorEdit("Aresta", edgeColor);
            changed |= drawColorEdit("Vertice", vertexColor);
            return changed;
        }

    private:
        Rot3f rotator() const
        {
            Rot3f rot = Rot3f::identity();

            for (int a = 0; a < 3; ++a)
            {
                Vec3f axis{0.0f, 0.0f, 0.0f};
                axis[a] = 1.0f;
                rot.rotateWorld(axis, rotationDegrees[a] * kDegToRad);
            }

            return rot;
        }

        Vec3f worldToLocal(const Vec3f &world) const
        {
            const auto q = rotator().quaternion();
            const Rot3f inverse{Quatf{-q[0], -q[1], -q[2], q[3]}};
            return inverse.rotateVector(world - position);
        }

        Vec3f unitToWorld(const Vec3f &unit) const
        {
            const auto lo = bounds.minimum();
            const auto hi = bounds.maximum();

            const auto axis = [&](int a)
            { return lo[a] + (unit[a] + 1.0f) * 0.5f * (hi[a] - lo[a]); };

            return Vec3f{axis(0), axis(1), axis(2)};
        }

        AABB unitToLocal(const AABB &unitBox) const
        {
            const Vec3f lo = unitToWorld(unitBox.minimum());
            const Vec3f hi = unitToWorld(unitBox.maximum());

            constexpr float inf = std::numeric_limits<float>::infinity();
            Vec3f localMin{inf, inf, inf};
            Vec3f localMax{-inf, -inf, -inf};

            for (int i = 0; i < 8; ++i)
            {
                const Vec3f corner{
                    (i & 1) ? hi[0] : lo[0],
                    (i & 2) ? hi[1] : lo[1],
                    (i & 4) ? hi[2] : lo[2]};

                const Vec3f local = worldToLocal(corner);

                for (int a = 0; a < 3; ++a)
                {
                    localMin[a] = std::min(localMin[a], local[a]);
                    localMax[a] = std::max(localMax[a], local[a]);
                }
            }

            return AABB{
                Vec3f{localMin[0], localMin[1], localMin[2]},
                Vec3f{localMax[0], localMax[1], localMax[2]}};
        }

        Coverage classifyShape(const AABB &unitBox) const
        {
            const AABB local = unitToLocal(unitBox);

            switch (kind)
            {
            case ObjectKind::Sphere:
                return classifySphere(sphere, local);
            case ObjectKind::Block:
                return classifyBlock(block, local);
            case ObjectKind::Cylinder:
                return classifyCylinder(cylinder, local);
            default:
                return Coverage::Empty;
            }
        }
    };

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
protected:
    void onInit(int width, int height, const std::string &) override
    {
        initImGui();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        m_width = width;
        m_height = height;

        applyCameraProjection();

        SceneObject first;
        first.name = "Esfera 1";
        addObject(std::move(first));

        rebuildMeshes();
    }

    void onShutdown() override
    {
        shutdownImGui();
    }

    void onWindowResize(int width, int height) override
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
    std::vector<float> m_volumes;

    int m_selectedIndex = -1;
    bool m_needRebuild = false;

    AABB m_worldBounds{{-10.0f, -10.0f, -10.0f}, {10.0f, 10.0f, 10.0f}};
    Vec3f m_octreeScale{1.0f, 1.0f, 1.0f};
    int m_octreeDepth = 5;

    bool m_showFaces = true;
    bool m_showEdges = false;
    bool m_showVertices = false;
    bool m_showWorldBounds = true;
    Color m_worldBoundsColor = Color::White();

    bool m_canvasHovered = true;
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;

    float m_cameraFov = kFovDegrees;
    float m_cameraNear = kNearPlane;
    float m_cameraFar = kFarPlane;
    float m_orbitSensitivity = kOrbitSensitivity;
    float m_zoomSensitivity = 1.0f;
    float m_moveSpeed = 10.0f;

    bool hasSelection() const
    {
        return m_selectedIndex >= 0 &&
               m_selectedIndex < static_cast<int>(m_objects.size());
    }

    float aspect() const
    {
        return (m_height > 0)
                   ? static_cast<float>(m_width) / static_cast<float>(m_height)
                   : 1.0f;
    }

    void addObject(SceneObject object)
    {
        object.bounds = m_worldBounds;
        m_objects.push_back(std::move(object));
        m_selectedIndex = static_cast<int>(m_objects.size()) - 1;
    }

    void duplicateObject(int index)
    {
        SceneObject copy = m_objects[index];
        copy.name += " copia";

        m_objects.insert(m_objects.begin() + index + 1, std::move(copy));
        m_selectedIndex = index + 1;
        m_needRebuild = true;
    }

    void removeObject(int index)
    {
        m_objects.erase(m_objects.begin() + index);

        if (m_selectedIndex == index)
        {
            m_selectedIndex = m_objects.empty()
                                  ? -1
                                  : std::min(index, static_cast<int>(m_objects.size()) - 1);
        }
        else if (m_selectedIndex > index)
        {
            --m_selectedIndex;
        }

        m_needRebuild = true;
    }

    void applyCameraProjection()
    {
        camera().setPerspective(m_cameraFov, aspect(), m_cameraNear, m_cameraFar);
    }

    void updateCameraOrbit()
    {
        if (!m_canvasHovered)
            return;

        const double dx = input().m_mouseX - m_lastMouseX;
        const double dy = input().m_mouseY - m_lastMouseY;

        m_lastMouseX = input().m_mouseX;
        m_lastMouseY = input().m_mouseY;

        if (input().rightMouse())
        {
            camera().orbit(
                static_cast<float>(dx) * m_orbitSensitivity,
                static_cast<float>(dy) * m_orbitSensitivity);
        }

        if (input().scrollOffset() != 0.0)
        {
            camera().zoom(
                static_cast<float>(input().scrollOffset()) * m_zoomSensitivity);
        }
    }

    void updateCameraMovement(float deltaTime)
    {
        if (!m_canvasHovered)
            return;

        const float amount = m_moveSpeed * deltaTime;

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

    void rebuildMeshes()
    {
        m_meshes.clear();
        m_volumes.clear();

        m_meshes.reserve(m_objects.size());
        m_volumes.reserve(m_objects.size());

        for (const auto &object : m_objects)
        {
            const Octree tree = object.buildOctree(m_octreeDepth);

            m_volumes.push_back(tree.volume(object.bounds));

            Mesh3f mesh = tree.toMesh(object.scaledBounds(m_octreeScale));

            if (mesh.edges().empty())
                mesh.buildEdgesFromFaces();

            m_meshes.push_back(std::move(mesh));
        }
    }

    std::string unionToString() const
    {
        if (m_objects.empty())
            return {};

        const Octree::Orientation orientation = m_objects.front().orientation;

        Octree tree = m_objects.front().buildOctree(m_octreeDepth, orientation);

        for (std::size_t i = 1; i < m_objects.size(); ++i)
            tree |= m_objects[i].buildOctree(m_octreeDepth, orientation);

        return tree.toString();
    }

    void drawScene(Drawer &drawer)
    {
        const Color transparent = Color::Transparent();

        if (m_showWorldBounds)
        {
            Mesh3f boundsMesh = m_worldBounds.toMesh();
            drawer.drawMesh(boundsMesh, transparent, m_worldBoundsColor, transparent);
        }

        for (std::size_t i = 0; i < m_objects.size() && i < m_meshes.size(); ++i)
        {
            const auto &object = m_objects[i];

            drawer.drawMesh(
                m_meshes[i],
                m_showFaces ? object.faceColor : transparent,
                m_showEdges ? object.edgeColor : transparent,
                m_showVertices ? object.vertexColor : transparent);
        }
    }

    void renderUI()
    {
        ImGui::DockSpaceOverViewport(
            0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        drawOutlinerPanel();
        drawPropertiesPanel();

        m_canvasHovered = ImGui::GetCurrentContext()->HoveredWindow == nullptr;

        if (m_needRebuild)
        {
            rebuildMeshes();
            m_needRebuild = false;
        }
    }

    void drawOutlinerPanel()
    {
        ImGui::SetNextWindowSize(ImVec2(260.0f, 480.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Outliner");

        if (ImGui::TreeNodeEx("Cena",
                              ImGuiTreeNodeFlags_DefaultOpen |
                                  ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            drawAddObjectButton();
            ImGui::Separator();
            drawObjectList();
            ImGui::TreePop();
        }

        ImGui::End();
    }

    void drawAddObjectButton()
    {
        if (ImGui::Button("+ Adicionar Objeto", ImVec2(-1.0f, 0.0f)))
            ImGui::OpenPopup("AddObjectPopup");

        if (!ImGui::BeginPopup("AddObjectPopup"))
            return;

        for (int i = 0; i < static_cast<int>(kObjectKindLabels.size()); ++i)
        {
            if (!ImGui::MenuItem(kObjectKindLabels[i]))
                continue;

            SceneObject object;
            object.kind = static_cast<ObjectKind>(i);
            object.name = std::string(kObjectKindLabels[i]) + " " +
                          std::to_string(m_objects.size() + 1);

            addObject(std::move(object));
            m_needRebuild = true;
        }

        ImGui::EndPopup();
    }

    void drawObjectList()
    {
        ImGui::BeginChild("ObjectList", ImVec2(0.0f, 0.0f), false);

        int pendingRemove = -1;
        int pendingDuplicate = -1;

        for (int i = 0; i < static_cast<int>(m_objects.size()); ++i)
        {
            const SceneObject &object = m_objects[i];

            ImGui::PushID(i);

            const std::string label = std::string(kindIcon(object.kind)) + "  " + object.name;

            if (ImGui::Selectable(label.c_str(), m_selectedIndex == i))
                m_selectedIndex = i;

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
            duplicateObject(pendingDuplicate);

        if (pendingRemove >= 0)
            removeObject(pendingRemove);

        ImGui::EndChild();
    }

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

            if (ImGui::BeginTabItem("Camera"))
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

        if (ImGui::CollapsingHeader("Projecao", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool projChanged = false;
            projChanged |= ImGui::SliderFloat(
                "FOV (graus)", &m_cameraFov, 10.0f, 120.0f, "%.1f");
            projChanged |= ImGui::DragFloat(
                "Near", &m_cameraNear, 0.001f, 0.001f, m_cameraFar - 0.01f, "%.3f");
            projChanged |= ImGui::DragFloat(
                "Far", &m_cameraFar, 1.0f, m_cameraNear + 0.01f, 10000.0f, "%.1f");

            if (projChanged)
            {
                applyCameraProjection();
                changed = true;
            }

            ImGui::TextDisabled("Aspect: %.3f", aspect());
        }

        if (ImGui::CollapsingHeader("Controle", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat("Sens. Orbita", &m_orbitSensitivity, 0.01f, 2.0f, "%.2f");
            ImGui::SliderFloat("Sens. Zoom", &m_zoomSensitivity, 0.10f, 5.0f, "%.2f");
            ImGui::SliderFloat("Vel. Movimento (WASD)", &m_moveSpeed, 1.0f, 50.0f, "%.1f");
        }

        if (ImGui::CollapsingHeader("Estado", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const auto p = cam.position();
            ImGui::Text("Posicao:  %.3f, %.3f, %.3f", p[0], p[1], p[2]);

            const auto f = cam.rotation().forward();
            ImGui::Text("Forward:  %.3f, %.3f, %.3f", f[0], f[1], f[2]);

            const auto u = cam.rotation().up();
            ImGui::Text("Up:       %.3f, %.3f, %.3f", u[0], u[1], u[2]);
        }

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

    bool drawSceneSettingsUI()
    {
        bool changed = false;

        if (ImGui::CollapsingHeader("Espaco Global", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool boundsChanged = false;
            boundsChanged |= ImGui::DragFloat3("Minimo", m_worldBounds.minimum().data_ptr(), kDragSpeed);
            boundsChanged |= ImGui::DragFloat3("Maximo", m_worldBounds.maximum().data_ptr(), kDragSpeed);
            ImGui::TextDisabled("Aplicado ao AABB de todos os objetos.");

            if (boundsChanged)
            {
                for (auto &object : m_objects)
                    object.bounds = m_worldBounds;

                changed = true;
            }
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
            ImGui::Separator();
            ImGui::Checkbox("Limites do Mundo", &m_showWorldBounds);
            drawColorEdit("Cor dos Limites", m_worldBoundsColor);
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

        if (ImGui::Button("Copiar Uniao para Area de Transferencia", ImVec2(-1.0f, 0.0f)))
            ImGui::SetClipboardText(unionToString().c_str());

        return changed;
    }

    bool drawObjectSettingsUI()
    {
        if (!hasSelection())
        {
            ImGui::Spacing();
            ImGui::TextDisabled("Nenhum objeto selecionado.");
            ImGui::Spacing();
            ImGui::TextWrapped(
                "Selecione um objeto no Outliner para editar suas propriedades.");
            return false;
        }

        SceneObject &object = m_objects[m_selectedIndex];
        bool changed = false;

        ImGui::Text("%s", kindIcon(object.kind));
        ImGui::SameLine();
        ImGui::InputText("##name", &object.name);

        ImGui::Separator();
        ImGui::Separator();

        if (m_selectedIndex < static_cast<int>(m_volumes.size()))
            ImGui::Text("Volume: %.6f", m_volumes[m_selectedIndex]);

        ImGui::Separator();

        if (object.hasTransform() &&
            ImGui::CollapsingHeader("Transformacao", ImGuiTreeNodeFlags_DefaultOpen))
        {
            changed |= object.drawTransformUI();
        }

        if (ImGui::CollapsingHeader("Forma", ImGuiTreeNodeFlags_DefaultOpen))
            changed |= object.drawShapeUI();

        if (ImGui::CollapsingHeader("Cores", ImGuiTreeNodeFlags_DefaultOpen))
            changed |= object.drawColorsUI();

        return changed;
    }

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