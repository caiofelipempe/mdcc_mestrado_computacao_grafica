#include "renderer_glfw_opengl.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <memory>
#include <numbers>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>

#include "bounding_volume.hpp"
#include "csg.hpp"
#include "mesh.hpp"
#include "shape.hpp"
#include "shape_to_mesh.hpp"

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

    constexpr float kDegToRad = std::numbers::pi_v<float> / 180.0f;

    /* ================= ROTULOS ================= */

    enum class ShapeKind
    {
        Sphere,
        Block,
        Cylinder,
        Cone,
        Capsule,
        Tetrahedron
    };

    constexpr std::array<const char *, 6> kShapeLabels{
        "Esfera", "Bloco", "Cilindro", "Cone", "Capsula", "Tetraedro"};

    constexpr std::array<const char *, 3> kOperationLabels{
        "Uniao (A | B)", "Intersecao (A & B)", "Diferenca (A - B)"};

    constexpr std::array<const char *, 3> kOperationNames{
        "Uniao", "Intersecao", "Diferenca"};

    const char *shapeIcon(ShapeKind kind)
    {
        switch (kind)
        {
        case ShapeKind::Sphere:
            return "(o)";
        case ShapeKind::Block:
            return "[#]";
        case ShapeKind::Cylinder:
            return "(=)";
        case ShapeKind::Cone:
            return "/\\";
        case ShapeKind::Capsule:
            return "(|)";
        case ShapeKind::Tetrahedron:
            return "<>";
        }
        return "(?)";
    }

    const char *operationIcon(CsgOp op)
    {
        switch (op)
        {
        case CsgOp::Union:
            return "[U]";
        case CsgOp::Intersection:
            return "[I]";
        case CsgOp::Difference:
            return "[D]";
        }
        return "[?]";
    }

    bool drawColorEdit(const char *label, Color &color)
    {
        float rgba[4] = {color.r, color.g, color.b, color.a};

        if (!ImGui::ColorEdit4(label, rgba))
            return false;

        color = Color(rgba[0], rgba[1], rgba[2], rgba[3]);
        return true;
    }

    /* ================= MODELO DA CENA ================= */

    struct SceneNode;
    using NodePtr = std::unique_ptr<SceneNode>;

    struct SceneNode
    {
        std::string name = "No";
        bool operation = false;

        // No de operacao
        CsgOp op = CsgOp::Union;
        NodePtr left;  // A
        NodePtr right; // B

        // No de primitiva
        ShapeKind kind = ShapeKind::Sphere;
        float radius = 3.0f;
        float height = 5.0f;
        float edge = 6.0f;
        Vec3f blockSize{4.0f, 5.0f, 3.0f};

        Vec3f position{0.0f, 0.0f, 0.0f};
        Vec3f rotationDegrees{0.0f, 0.0f, 0.0f};

        bool drawShapeUI()
        {
            bool changed = false;

            int current = static_cast<int>(kind);
            if (ImGui::Combo("Tipo", &current, kShapeLabels.data(),
                             static_cast<int>(kShapeLabels.size())))
            {
                kind = static_cast<ShapeKind>(current);
                changed = true;
            }

            ImGui::Spacing();

            switch (kind)
            {
            case ShapeKind::Sphere:
                changed |= ImGui::DragFloat("Raio", &radius, kDragSpeed, kDragMin, kDragMax);
                break;

            case ShapeKind::Block:
                changed |= ImGui::DragFloat3("Dimensoes (L, A, P)", blockSize.data_ptr(),
                                             kDragSpeed, kDragMin, kDragMax);
                break;

            case ShapeKind::Cylinder:
            case ShapeKind::Cone:
                changed |= ImGui::DragFloat("Raio", &radius, kDragSpeed, kDragMin, kDragMax);
                changed |= ImGui::DragFloat("Altura", &height, kDragSpeed, kDragMin, kDragMax);
                break;

            case ShapeKind::Capsule:
                changed |= ImGui::DragFloat("Raio", &radius, kDragSpeed, kDragMin, kDragMax);
                changed |= ImGui::DragFloat("Altura (trecho cilindrico)", &height,
                                            kDragSpeed, 0.0f, kDragMax);
                break;

            case ShapeKind::Tetrahedron:
                changed |= ImGui::DragFloat("Aresta", &edge, kDragSpeed, kDragMin, kDragMax);
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
    };

    Rot3f rotatorOf(const SceneNode &node)
    {
        Rot3f rot = Rot3f::identity();

        for (int a = 0; a < 3; ++a)
        {
            Vec3f axis{0.0f, 0.0f, 0.0f};
            axis[a] = 1.0f;
            rot.rotateWorld(axis, node.rotationDegrees[a] * kDegToRad);
        }

        return rot;
    }

    CsgPtr buildCsg(const SceneNode &node)
    {
        if (node.operation)
            return csgOperation(node.op, buildCsg(*node.left), buildCsg(*node.right));

        const Rot3f rot = rotatorOf(node);

        switch (node.kind)
        {
        case ShapeKind::Sphere:
            return csgPrimitive(Sphere(node.radius), node.position, rot);
        case ShapeKind::Block:
            return csgPrimitive(Block(node.blockSize[0], node.blockSize[1], node.blockSize[2]),
                                node.position, rot);
        case ShapeKind::Cylinder:
            return csgPrimitive(Cylinder(node.radius, node.height), node.position, rot);
        case ShapeKind::Cone:
            return csgPrimitive(Cone(node.radius, node.height), node.position, rot);
        case ShapeKind::Capsule:
            return csgPrimitive(Capsule(node.radius, node.height), node.position, rot);
        case ShapeKind::Tetrahedron:
            return csgPrimitive(RegularTetrahedron(node.edge), node.position, rot);
        }

        return nullptr;
    }

    NodePtr *findSlot(NodePtr &slot, const SceneNode *target)
    {
        if (!slot)
            return nullptr;
        if (slot.get() == target)
            return &slot;
        if (!slot->operation)
            return nullptr;

        if (NodePtr *found = findSlot(slot->left, target))
            return found;
        return findSlot(slot->right, target);
    }

    bool removeNode(NodePtr &slot, const SceneNode *target)
    {
        if (!slot)
            return false;

        if (slot.get() == target)
        {
            slot.reset();
            return true;
        }

        if (!slot->operation)
            return false;

        for (NodePtr *child : {&slot->left, &slot->right})
        {
            if (child->get() != target)
                continue;

            NodePtr sibling = std::move(child == &slot->left ? slot->right : slot->left);
            slot = std::move(sibling);
            return true;
        }

        return removeNode(slot->left, target) || removeNode(slot->right, target);
    }

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

class Trabalho02 : public RendererGlfwOpengl
{
protected:
    void onInit(int width, int height, const std::string &) override
    {
        initImGui();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        m_width = width;
        m_height = height;

        applyCameraProjection();

        m_root = makePrimitive(ShapeKind::Sphere);
        m_selected = m_root.get();

        rebuildMesh();
    }

    void onShutdown() override
    {
        shutdownImGui();
    }

    void onWindowResize(int width, int height) override
    {
        m_width = width;
        m_height = height;
        rebuildMesh();
    }

    void onUpdate(float deltaTime) override
    {
        updateCameraOrbit();
        updateCameraMovement(deltaTime);

        // Reconstrói a cada frame para recalcular os raios a partir da posição atual da câmera
        rebuildMesh();
    }

    void onRender(DrawerOpengl &drawer) override
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

    NodePtr m_root;
    SceneNode *m_selected = nullptr;
    int m_nextId = 1;

    Mesh3f m_mesh;
    Mesh3f m_boundsMesh;
    float m_volume = 0.0f;

    AABB m_worldBounds{{-10.0f, -10.0f, -10.0f}, {10.0f, 10.0f, 10.0f}};
    int m_raycastResolutionScale = 4; // Controla a densidade dos quads na tela

    bool m_showFaces = true;
    bool m_showEdges = false;
    bool m_showVertices = false;
    bool m_showWorldBounds = true;

    Color m_faceColor = Color::DarkGreen();
    Color m_edgeColor = Color::Yellow();
    Color m_vertexColor = Color::Red();
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

    float aspect() const
    {
        return (m_height > 0)
                   ? static_cast<float>(m_width) / static_cast<float>(m_height)
                   : 1.0f;
    }

    /* ================= ARVORE ================= */

    NodePtr makePrimitive(ShapeKind kind)
    {
        auto node = std::make_unique<SceneNode>();
        node->kind = kind;
        node->name = std::string(kShapeLabels[static_cast<std::size_t>(kind)]) + " " +
                     std::to_string(m_nextId++);
        return node;
    }

    void combineNode(SceneNode *target, CsgOp op, ShapeKind kind)
    {
        NodePtr *slot = target ? findSlot(m_root, target) : nullptr;
        if (!slot)
            return;

        auto primitive = makePrimitive(kind);
        primitive->position = Vec3f{2.0f, 0.0f, 0.0f};
        SceneNode *added = primitive.get();

        auto node = std::make_unique<SceneNode>();
        node->operation = true;
        node->op = op;
        node->name = std::string(kOperationNames[static_cast<std::size_t>(op)]) + " " +
                     std::to_string(m_nextId++);
        node->left = std::move(*slot);
        node->right = std::move(primitive);

        *slot = std::move(node);
        m_selected = added;
        rebuildMesh();
    }

    void removeNodeTarget(SceneNode *target)
    {
        if (!target)
            return;

        removeNode(m_root, target);
        m_selected = m_root ? m_root.get() : nullptr;
        rebuildMesh();
    }

    /* ================= CAMERA ================= */

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

    /* ================= GERAÇÃO POR RAYCAST DE QUADDS ================= */

    void rebuildMesh()
    {
        m_mesh = Mesh3f{};
        m_volume = 0.0f;

        if (m_root)
        {
            const CsgPtr csg = buildCsg(*m_root);

            const int w = std::max(1, m_width / m_raycastResolutionScale);
            const int h = std::max(1, m_height / m_raycastResolutionScale);

            const float tanHalfFov = std::tan(m_cameraFov * kDegToRad * 0.5f);
            const float aspectVal = aspect();

            const Vec3f origin = camera().position();
            const Vec3f forward = camera().rotation().forward();
            const Vec3f right = camera().rotation().right();
            const Vec3f up = camera().rotation().up();

            const float pixelW = (2.0f * tanHalfFov * aspectVal) / static_cast<float>(w);
            const float pixelH = (2.0f * tanHalfFov) / static_cast<float>(h);

            auto dirAt = [&](long x, long y)
            {
                const float py = (1.0f - 2.0f * (static_cast<float>(y) + 0.5f) / static_cast<float>(h)) * tanHalfFov;
                const float px = (2.0f * (static_cast<float>(x) + 0.5f) / static_cast<float>(w) - 1.0f) * aspectVal * tanHalfFov;
                return (forward + right * px + up * py).normalized();
            };

            for (long y = 0; y < h; ++y)
            {
                for (long x = 0; x < w; ++x)
                {
                    const Vec3f dir = dirAt(x, y);

                    const std::optional<RayHit> hit = csg->raycast(origin, dir);
                    if (!hit)
                        continue;

                    const Vec3f hitPoint = origin + dir * hit->tEnter;

                    const float cosF = dir.dot(forward);
                    const float zIn = hit->tEnter * cosF;
                    const float zOut = hit->tExit * cosF;

                    const float halfW = 0.5f * pixelW * zIn;
                    const float halfH = 0.5f * pixelH * zIn;

                    Vec3f normal = hit->normalEnter;
                    normal = normal.sqrNorm() < 1e-12f ? dir * -1.0f : normal.normalized();

                    // Quad sempre voltado para a câmera
                    const Vec3f p0 = hitPoint - right * halfW - up * halfH;
                    const Vec3f p1 = hitPoint + right * halfW - up * halfH;
                    const Vec3f p2 = hitPoint + right * halfW + up * halfH;
                    const Vec3f p3 = hitPoint - right * halfW + up * halfH;

                    const std::size_t i0 = m_mesh.addVertex(p0);
                    const std::size_t i1 = m_mesh.addVertex(p1);
                    const std::size_t i2 = m_mesh.addVertex(p2);
                    const std::size_t i3 = m_mesh.addVertex(p3);

                    m_mesh.addFace(i0, i1, i2);
                    m_mesh.faces().back().lightNormal = normal;
                    m_mesh.addFace(i0, i2, i3);
                    m_mesh.faces().back().lightNormal = normal;

                    m_mesh.addEdge(i0, i1);
                    m_mesh.addEdge(i1, i2);
                    m_mesh.addEdge(i2, i3);
                    m_mesh.addEdge(i3, i0);

                    // Volume do tronco de frustum deste pixel entre a entrada e a saída
                    m_volume += pixelW * pixelH * (zOut * zOut * zOut - zIn * zIn * zIn) / 3.0f;
                }
            }
        }

        const Vec3f size = m_worldBounds.maximum() - m_worldBounds.minimum();
        m_boundsMesh = geometry::toMesh(Block(size[0], size[1], size[2]));
        m_boundsMesh.translate(m_worldBounds.center());
    }

    void drawScene(DrawerOpengl &drawer)
    {
        const Color transparent = Color::Transparent();

        if (m_showWorldBounds)
        {
            drawer.rastMeshes(&m_boundsMesh, 1, transparent,
                              m_worldBoundsColor, transparent);
        }

        drawer.rastMeshes(
            &m_mesh, 1,
            m_showFaces ? m_faceColor : transparent,
            m_showEdges ? m_edgeColor : transparent,
            m_showVertices ? m_vertexColor : transparent);
    }

    /* ================= UI ================= */

    void renderUI()
    {
        ImGui::DockSpaceOverViewport(
            0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        drawOutlinerPanel();
        drawPropertiesPanel();

        m_canvasHovered = ImGui::GetCurrentContext()->HoveredWindow == nullptr;
    }

    void drawOutlinerPanel()
    {
        ImGui::SetNextWindowSize(ImVec2(300.0f, 500.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Outliner (Arvore CSG)");

        if (!m_root)
        {
            if (ImGui::Button("+ Nova Primitiva Raiz", ImVec2(-1.0f, 0.0f)))
                ImGui::OpenPopup("NewRootPopup");

            if (ImGui::BeginPopup("NewRootPopup"))
            {
                for (std::size_t i = 0; i < kShapeLabels.size(); ++i)
                {
                    if (ImGui::MenuItem(kShapeLabels[i]))
                    {
                        m_root = makePrimitive(static_cast<ShapeKind>(i));
                        m_selected = m_root.get();
                        rebuildMesh();
                    }
                }
                ImGui::EndPopup();
            }
        }
        else
        {
            ImGui::TextDisabled("Dica: Clique com o botao direito em um no para adicionar operacoes ou remove-lo.");
        }

        ImGui::Separator();

        if (m_root)
            drawNodeTree(*m_root, "");

        ImGui::End();
    }

    void drawNodeTree(SceneNode &node, const char *role)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen |
                                   ImGuiTreeNodeFlags_OpenOnArrow |
                                   ImGuiTreeNodeFlags_SpanAvailWidth;

        if (!node.operation)
            flags |= ImGuiTreeNodeFlags_Leaf;
        if (&node == m_selected)
            flags |= ImGuiTreeNodeFlags_Selected;

        ImGui::PushID(&node);

        const std::string label = std::string(role) +
                                  (node.operation ? operationIcon(node.op) : shapeIcon(node.kind)) +
                                  "  " + node.name;

        const bool open = ImGui::TreeNodeEx(label.c_str(), flags);

        if (ImGui::IsItemClicked())
            m_selected = &node;

        if (ImGui::BeginPopupContextItem("NodeContextMenu"))
        {
            m_selected = &node;

            if (node.operation)
            {
                int currentOp = static_cast<int>(node.op);
                if (ImGui::Combo("Mudar Operacao", &currentOp, kOperationLabels.data(),
                                 static_cast<int>(kOperationLabels.size())))
                {
                    node.op = static_cast<CsgOp>(currentOp);
                    rebuildMesh();
                }
                ImGui::Separator();
            }

            if (ImGui::BeginMenu("Adicionar Operacao..."))
            {
                for (std::size_t op = 0; op < kOperationLabels.size(); ++op)
                {
                    if (ImGui::BeginMenu(kOperationLabels[op]))
                    {
                        for (std::size_t kind = 0; kind < kShapeLabels.size(); ++kind)
                        {
                            if (ImGui::MenuItem(kShapeLabels[kind]))
                            {
                                combineNode(&node, static_cast<CsgOp>(op), static_cast<ShapeKind>(kind));
                            }
                        }
                        ImGui::EndMenu();
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Remover No"))
            {
                removeNodeTarget(&node);
                ImGui::EndPopup();
                ImGui::TreePop();
                ImGui::PopID();
                return;
            }

            ImGui::EndPopup();
        }

        if (open)
        {
            if (node.operation)
            {
                drawNodeTree(*node.left, "A: ");
                drawNodeTree(*node.right, "B: ");
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    void drawPropertiesPanel()
    {
        ImGui::SetNextWindowSize(ImVec2(340.0f, 540.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Propriedades");

        if (ImGui::BeginTabBar("PropertiesTabs"))
        {
            if (ImGui::BeginTabItem("Cena"))
            {
                drawSceneSettingsUI();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("No"))
            {
                drawNodeSettingsUI();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Camera"))
            {
                drawCameraSettingsUI();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    bool drawNodeSettingsUI()
    {
        if (!m_selected)
        {
            ImGui::Spacing();
            ImGui::TextDisabled("Nenhum no selecionado.");
            ImGui::Spacing();
            ImGui::TextWrapped("Selecione um no na arvore do Outliner para editar suas propriedades.");
            return false;
        }

        SceneNode &node = *m_selected;
        bool changed = false;

        ImGui::Text("%s", node.operation ? operationIcon(node.op) : shapeIcon(node.kind));
        ImGui::SameLine();
        ImGui::InputText("##name", &node.name);

        ImGui::Separator();

        if (node.operation)
        {
            int current = static_cast<int>(node.op);
            if (ImGui::Combo("Operacao", &current, kOperationLabels.data(),
                             static_cast<int>(kOperationLabels.size())))
            {
                node.op = static_cast<CsgOp>(current);
                changed = true;
            }

            ImGui::Text("A: %s", node.left->name.c_str());
            ImGui::Text("B: %s", node.right->name.c_str());

            if (ImGui::Button("Trocar A e B", ImVec2(-1.0f, 0.0f)))
            {
                std::swap(node.left, node.right);
                changed = true;
            }

            if (changed)
                rebuildMesh();

            return changed;
        }

        if (ImGui::CollapsingHeader("Transformacao", ImGuiTreeNodeFlags_DefaultOpen))
            changed |= node.drawTransformUI();

        if (ImGui::CollapsingHeader("Forma", ImGuiTreeNodeFlags_DefaultOpen))
            changed |= node.drawShapeUI();

        if (changed)
            rebuildMesh();

        return changed;
    }

    bool drawSceneSettingsUI()
    {
        bool changed = false;

        if (ImGui::CollapsingHeader("Espaço de Amostragem", ImGuiTreeNodeFlags_DefaultOpen))
        {
            changed |= ImGui::DragFloat3("Minimo", m_worldBounds.minimum().data_ptr(), kDragSpeed);
            changed |= ImGui::DragFloat3("Maximo", m_worldBounds.maximum().data_ptr(), kDragSpeed);
            ImGui::TextDisabled("Regiao em que o CSG e amostrado.");
        }

        if (ImGui::CollapsingHeader("Raycast de Quads", ImGuiTreeNodeFlags_DefaultOpen))
        {
            changed |= ImGui::SliderInt("Escala de Resolucao", &m_raycastResolutionScale, 1, 8);
            ImGui::TextDisabled("Controla a densidade dos quads gerados a cada frame.");
        }

        if (ImGui::CollapsingHeader("Exibicao", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Faces", &m_showFaces);
            ImGui::Checkbox("Arestas", &m_showEdges);
            ImGui::Checkbox("Vertices", &m_showVertices);
            ImGui::Separator();
            drawColorEdit("Face", m_faceColor);
            drawColorEdit("Aresta", m_edgeColor);
            drawColorEdit("Vertice", m_vertexColor);
            ImGui::Separator();
            ImGui::Checkbox("Limites do Mundo", &m_showWorldBounds);
            drawColorEdit("Cor dos Limites", m_worldBoundsColor);
        }

        if (ImGui::CollapsingHeader("Estatisticas"))
        {
            ImGui::Text("Volume aprox: %.6f", m_volume);
            ImGui::Text("Vertices: %zu", m_mesh.vertices().size());
            ImGui::Text("Faces: %zu", m_mesh.faces().size());
        }

        if (changed)
            rebuildMesh();

        return changed;
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

        if (changed)
            rebuildMesh();

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
    Trabalho02 app;
    app.run(1280, 720, "Modelador Geometrico - CSG Raycast Quads");
}