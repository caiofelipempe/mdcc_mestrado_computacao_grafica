#include "renderer_glfw_opengl.hpp"

#include <array>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <functional>

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
    Trabalho01(std::function<void(Drawer &)> draw)
        : m_drawLambda(draw)
    {
    }

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
    std::function<void(Drawer &)> m_drawLambda;
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
        m_drawLambda(drawer);
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

static OctreeState sphereBuild(AABB const &aabb, Sphere const &sphere)
{
    auto const sqrRadius = sphere.radius() * sphere.radius();
    auto const min = aabb.minimum();
    auto const max = aabb.maximum();
    Vec3 const furthestVertex{
        (std::abs(min[0]) > std::abs(max[0])) ? min[0] : max[0],
        (std::abs(min[1]) > std::abs(max[1])) ? min[1] : max[1],
        (std::abs(min[2]) > std::abs(max[2])) ? min[2] : max[2]};

    if (furthestVertex.sqrNorm() <= sqrRadius)
    {
        return OctreeState::Filled;
    }

    Vec3 const closestPoint{
        std::clamp(0.f, min[0], max[0]),
        std::clamp(0.f, min[1], max[1]),
        std::clamp(0.f, min[2], max[2])};

    if (closestPoint.sqrNorm() <= sqrRadius)
    {
        return OctreeState::Branch;
    }

    return OctreeState::Empty;
}

static OctreeState blockBuild(
    AABB const &aabb,
    Block const &block)
{
    auto const half =
        block.boundSize() * 0.5f;

    auto const min =
        aabb.minimum();

    auto const max =
        aabb.maximum();

    Vec3 const furthestVertex{
        (std::abs(min[0]) > std::abs(max[0]))
            ? min[0]
            : max[0],

        (std::abs(min[1]) > std::abs(max[1]))
            ? min[1]
            : max[1],

        (std::abs(min[2]) > std::abs(max[2]))
            ? min[2]
            : max[2]};

    if (
        std::abs(furthestVertex[0]) <= half[0] &&
        std::abs(furthestVertex[1]) <= half[1] &&
        std::abs(furthestVertex[2]) <= half[2])
    {
        return OctreeState::Filled;
    }

    if (
        max[0] < -half[0] ||
        min[0] > half[0] ||

        max[1] < -half[1] ||
        min[1] > half[1] ||

        max[2] < -half[2] ||
        min[2] > half[2])
    {
        return OctreeState::Empty;
    }

    return OctreeState::Branch;
}

static OctreeState cylinderBuild(
    AABB const &aabb,
    Cylinder const &cylinder)
{
    auto const sqrRadius =
        cylinder.radius() *
        cylinder.radius();

    auto const halfHeight =
        cylinder.height() * 0.5f;

    auto const min =
        aabb.minimum();

    auto const max =
        aabb.maximum();

    Vec3 const furthestVertex{
        (std::abs(min[0]) > std::abs(max[0]))
            ? min[0]
            : max[0],

        (std::abs(min[1]) > std::abs(max[1]))
            ? min[1]
            : max[1],

        (std::abs(min[2]) > std::abs(max[2]))
            ? min[2]
            : max[2]};

    auto const furthestRadialSquared =
        furthestVertex[0] *
            furthestVertex[0] +
        furthestVertex[2] *
            furthestVertex[2];

    if (
        furthestRadialSquared <= sqrRadius &&
        std::abs(furthestVertex[1]) <= halfHeight)
    {
        return OctreeState::Filled;
    }

    Vec3 const closestPoint{
        std::clamp(
            0.f,
            min[0],
            max[0]),

        std::clamp(
            0.f,
            min[1],
            max[1]),

        std::clamp(
            0.f,
            min[2],
            max[2])};

    auto const closestRadialSquared =
        closestPoint[0] *
            closestPoint[0] +
        closestPoint[2] *
            closestPoint[2];

    if (
        closestRadialSquared > sqrRadius ||
        closestPoint[1] < -halfHeight ||
        closestPoint[1] > halfHeight)
    {
        return OctreeState::Empty;
    }

    return OctreeState::Branch;
}

int main()
{
    using namespace geometry;

    auto sphere = Sphere(10);
    auto diagonal = sphere.radius();

    auto cylinder = Cylinder(5, 5);

    Octree tree(
        {-diagonal, -diagonal, -diagonal},
        {diagonal, diagonal, diagonal});

    tree.build(
        [&cylinder](const AABB &bounds,
                  std::size_t depth)
        {
            auto result = cylinderBuild(bounds, cylinder);
            if (depth > 4 && result == OctreeState::Branch)
                return OctreeState::Filled;
            return result;
        });

    auto mesh = tree.toMesh();

    Trabalho01 app([&mesh](Drawer &drawer)
                   { drawer.drawMesh(
                         mesh,
                         Color::Green()); });

    app.run(
        1280,
        720,
        "Modelador Geometrico");
}