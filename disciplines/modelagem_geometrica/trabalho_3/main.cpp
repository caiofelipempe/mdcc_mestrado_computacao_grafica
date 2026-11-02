#include "renderer_glfw_opengl.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
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

#include "mesh.hpp"
#include "shape.hpp"
#include "winged_edge.hpp"

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

    const char *shapeNameFromKind(ShapeKind kind)
    {
        switch (kind)
        {
        case ShapeKind::Sphere:
        case ShapeKind::Block:
        case ShapeKind::Cylinder:
        case ShapeKind::Cone:
        case ShapeKind::Capsule:
        case ShapeKind::Tetrahedron:
            return kShapeLabels[static_cast<int>(kind)];
        }
        return "Inválido";
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

    struct SceneObject
    {
        std::string name = "Objeto";

        ShapeKind kind = ShapeKind::Sphere;
        float radius = 3.0f;
        float height = 5.0f;
        float edge = 6.0f;
        Vec3f blockSize{4.0f, 5.0f, 3.0f};
        int segments = 16;

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
                                            kDragSpeed, kDragMin, kDragMax);
                break;

            case ShapeKind::Tetrahedron:
                changed |= ImGui::DragFloat("Aresta", &edge, kDragSpeed, kDragMin, kDragMax);
                break;
            }

            if (kind != ShapeKind::Block && kind != ShapeKind::Tetrahedron)
                changed |= ImGui::SliderInt("Segmentos", &segments, 3, 64);

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

    Rot3f rotatorOf(const SceneObject &obj)
    {
        Rot3f rot = Rot3f::identity();

        for (int a = 0; a < 3; ++a)
        {
            Vec3f axis{0.0f, 0.0f, 0.0f};
            axis[a] = 1.0f;
            rot.rotateWorld(axis, obj.rotationDegrees[a] * kDegToRad);
        }

        return rot;
    }

    std::unique_ptr<Shape> makeShape(const SceneObject &obj)
    {
        switch (obj.kind)
        {
        case ShapeKind::Sphere:
            return std::make_unique<Sphere>(obj.radius);
        case ShapeKind::Block:
            return std::make_unique<Block>(obj.blockSize[0], obj.blockSize[1], obj.blockSize[2]);
        case ShapeKind::Cylinder:
            return std::make_unique<Cylinder>(obj.radius, obj.height);
        case ShapeKind::Cone:
            return std::make_unique<Cone>(obj.radius, obj.height);
        case ShapeKind::Capsule:
            return std::make_unique<Capsule>(obj.radius, obj.height);
        case ShapeKind::Tetrahedron:
            return std::make_unique<RegularTetrahedron>(obj.edge);
        }
        return nullptr;
    }

    float volumeOf(const Mesh3f &mesh)
    {
        double volume = 0.0;
        const auto &v = mesh.vertices();

        for (const auto &f : mesh.faces())
            volume += v[f.indices[0]].dot(v[f.indices[1]].cross(v[f.indices[2]])) / 6.0;

        return static_cast<float>(volume);
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

class Trabalho03 : public RendererGlfwOpengl
{
protected:
    void onInit(int width, int height, const std::string &) override
    {
        initImGui();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        m_width = width;
        m_height = height;

        applyCameraProjection();

        m_objects.push_back(makeObject(ShapeKind::Sphere));
        m_selected = 0;

        rebuild();
    }

    void onShutdown() override
    {
        shutdownImGui();
    }

    void onWindowResize(int width, int height) override
    {
        m_width = width;
        m_height = height;
        applyCameraProjection();
    }

    void onUpdate(float deltaTime) override
    {
        updateCameraOrbit();
        updateCameraMovement(deltaTime);
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

    std::vector<SceneObject> m_objects;
    int m_selected = -1;
    int m_nextId = 1;

    std::vector<winged::Solid> m_solids;
    std::vector<Mesh3f> m_meshes;
    float m_volume = 0.0f;

    bool m_showFaces = true;
    bool m_showEdges = false;
    bool m_showVertices = false;

    Color m_faceColor = Color::DarkGreen();
    Color m_edgeColor = Color::Yellow();
    Color m_vertexColor = Color::Red();

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

    /* ================= CENA ================= */

    SceneObject makeObject(ShapeKind kind)
    {
        SceneObject obj;
        obj.kind = kind;
        obj.name = "obj " + std::to_string(m_nextId++);
        return obj;
    }

    void addObject(SceneObject obj)
    {
        m_objects.push_back(std::move(obj));
        m_selected = static_cast<int>(m_objects.size()) - 1;
        rebuild();
    }

    void duplicateObject(int index)
    {
        SceneObject copy = m_objects[static_cast<std::size_t>(index)];
        copy.name += " (cópia)";
        addObject(std::move(copy));
    }

    void removeObject(int index)
    {
        m_objects.erase(m_objects.begin() + index);
        m_selected = std::min(m_selected, static_cast<int>(m_objects.size()) - 1);
        rebuild();
    }

    void rebuild()
    {
        m_solids.clear();
        m_meshes.clear();
        m_volume = 0.0f;

        for (const SceneObject &obj : m_objects)
        {
            const auto shape = makeShape(obj);
            m_solids.emplace_back(*shape, obj.position, rotatorOf(obj),
                                  static_cast<std::size_t>(obj.segments));
            m_meshes.push_back(m_solids.back().toMesh());
            m_volume += volumeOf(m_meshes.back());
        }
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

    void drawScene(DrawerOpengl &drawer)
    {
        if (m_meshes.empty())
            return;

        const Color transparent = Color::Transparent();

        drawer.rastMeshes(
            m_meshes.data(), static_cast<int>(m_meshes.size()),
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
        ImGui::Begin("Objetos (Winged-Edge)");

        if (ImGui::Button("+ Nova Primitiva", ImVec2(-1.0f, 0.0f)))
            ImGui::OpenPopup("NewObjectPopup");

        if (ImGui::BeginPopup("NewObjectPopup"))
        {
            for (std::size_t i = 0; i < kShapeLabels.size(); ++i)
            {
                if (ImGui::MenuItem(kShapeLabels[i]))
                    addObject(makeObject(static_cast<ShapeKind>(i)));
            }
            ImGui::EndPopup();
        }

        ImGui::TextDisabled("Clique com o botao direito em um objeto para duplicar ou remover.");
        ImGui::Separator();

        int toDuplicate = -1;
        int toRemove = -1;

        for (int i = 0; i < static_cast<int>(m_objects.size()); ++i)
        {
            const SceneObject &obj = m_objects[static_cast<std::size_t>(i)];

            ImGui::PushID(i);

            const std::string label = std::string(shapeNameFromKind(obj.kind)) + "  " + obj.name;
            if (ImGui::Selectable(label.c_str(), i == m_selected))
                m_selected = i;

            if (ImGui::BeginPopupContextItem("ObjectContextMenu"))
            {
                m_selected = i;

                if (ImGui::MenuItem("Duplicar"))
                    toDuplicate = i;
                if (ImGui::MenuItem("Remover"))
                    toRemove = i;

                ImGui::EndPopup();
            }

            ImGui::PopID();
        }

        if (toDuplicate >= 0)
            duplicateObject(toDuplicate);
        else if (toRemove >= 0)
            removeObject(toRemove);

        ImGui::End();
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

            if (ImGui::BeginTabItem("Objeto"))
            {
                drawObjectSettingsUI();
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

    bool drawObjectSettingsUI()
    {
        if (m_selected < 0 || m_selected >= static_cast<int>(m_objects.size()))
        {
            ImGui::Spacing();
            ImGui::TextDisabled("Nenhum objeto selecionado.");
            ImGui::Spacing();
            ImGui::TextWrapped("Selecione um objeto na lista para editar suas propriedades.");
            return false;
        }

        const std::size_t index = static_cast<std::size_t>(m_selected);
        SceneObject &obj = m_objects[index];
        bool changed = false;

        ImGui::Text("%s", shapeNameFromKind(obj.kind));
        ImGui::SameLine();
        ImGui::InputText("##name", &obj.name);

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Transformacao", ImGuiTreeNodeFlags_DefaultOpen))
            changed |= obj.drawTransformUI();

        if (ImGui::CollapsingHeader("Forma", ImGuiTreeNodeFlags_DefaultOpen))
            changed |= obj.drawShapeUI();

        if (changed)
            rebuild();

        if (ImGui::CollapsingHeader("Topologia", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const winged::Solid &solid = m_solids[index];
            ImGui::Text("Vertices: %zu", m_meshes[index].vertexCount());
            ImGui::Text("Arestas: %zu", m_meshes[index].edgeCount());
            ImGui::Text("Faces: %zu", solid.F.size());
            ImGui::Text("V - E + F - R = %d", solid.euler());
            ImGui::Text("Volume: %.6f", volumeOf(m_meshes[index]));
        }

        return changed;
    }

    void drawSceneSettingsUI()
    {
        if (ImGui::CollapsingHeader("Exibicao", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Faces", &m_showFaces);
            ImGui::Checkbox("Arestas", &m_showEdges);
            ImGui::Checkbox("Vertices", &m_showVertices);
            ImGui::Separator();
            drawColorEdit("Face", m_faceColor);
            drawColorEdit("Aresta", m_edgeColor);
            drawColorEdit("Vertice", m_vertexColor);
        }

        if (ImGui::CollapsingHeader("Estatisticas"))
        {
            std::size_t vertices = 0, edges = 0, faces = 0;
            for (std::size_t i = 0; i < m_solids.size(); ++i)
            {
                vertices += m_meshes[i].vertexCount();
                edges += m_meshes[i].edgeCount();
                faces += m_solids[i].F.size();
            }

            ImGui::Text("Objetos: %zu", m_objects.size());
            ImGui::Text("Volume total: %.6f", m_volume);
            ImGui::Text("Vertices: %zu", vertices);
            ImGui::Text("Arestas: %zu", edges);
            ImGui::Text("Faces: %zu", faces);
        }
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
    Trabalho03 app;
    app.run(1280, 720, "Modelador Geometrico - Winged-Edge");
}