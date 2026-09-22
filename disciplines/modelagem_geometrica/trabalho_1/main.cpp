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
    Trabalho01()
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

        init();
    }

    void onShutdown() override
    {
        shutdownImGui();
    }

    void onUpdate(float deltaTime) override
    {
        const double dx =
            input().m_mouseX -
            m_lastMouseX;

        const double dy =
            input().m_mouseY -
            m_lastMouseY;

        m_lastMouseX =
            input().m_mouseX;

        m_lastMouseY =
            input().m_mouseY;

        if (input().rightMouse())
        {
            camera().orbit(
                static_cast<float>(dx) *
                    kOrbitSensitivity,
                static_cast<float>(dy) *
                    kOrbitSensitivity);
        }

        if (input().scrollOffset() != 0.0)
        {
            camera().zoom(
                static_cast<float>(
                    input().scrollOffset()));
        }

        constexpr float speed =
            10.0f;

        const float amount =
            speed * deltaTime;

        const auto forward =
            camera().rotation().forward();

        const auto right =
            camera().rotation().right();

        const auto up =
            camera().rotation().up();

        if (input().pressed('W'))
        {
            camera().move(
                forward * amount);
        }

        if (input().pressed('S'))
        {
            camera().move(
                forward * -amount);
        }

        if (input().pressed('D'))
        {
            camera().move(
                right * amount);
        }

        if (input().pressed('A'))
        {
            camera().move(
                right * -amount);
        }

        if (input().pressed('E'))
        {
            camera().move(
                up * amount);
        }

        if (input().pressed('Q'))
        {
            camera().move(
                up * -amount);
        }
    }

    void onRender(Drawer &drawer) override
    {
        imguiStartRender();
        drawScene(drawer);
        imguiEndRender();
    }

private:
    enum class ShapeType
    {
        Sphere,
        Block,
        Cylinder
    };

    ShapeType m_shapeType =
        ShapeType::Sphere;

    Sphere m_sphere{10.0f};
    Block m_block{4.0f, 5.0f, 3.0f};
    Cylinder m_cylinder{5.0f, 5.0f};

    Mesh3f mesh;
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;

    bool m_showFaces = true;
    bool m_showEdges = false;
    bool m_showVertices = false;

    Vec3f m_octreeScale{1.f, 1.f, 1.f};

    void init()
    {
        auto sphere = Sphere(10);
        auto cylinder = Cylinder(5, 5);
        auto block = Block(4, 5, 3);

        auto octree = octreeFromShape(cylinder);
        mesh = octree.toMesh();
    }

    void renderUI()
    {
        ImGui::DockSpaceOverViewport(
            0,
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode);

        drawPanel();
    }

    void drawScene(Drawer &drawer)
    {
        const Color transparent =
            Color::Transparent();

        if (mesh.edges().empty())
        {
            mesh.buildEdgesFromFaces();
        }
        drawer.drawMesh(
            mesh,

            m_showFaces
                ? Color::DarkGreen()
                : transparent,

            m_showEdges ? Color::Yellow() : transparent,

            m_showVertices ? Color::Red() : transparent);
    }

    void drawPanel()
    {
        ImGui::Begin("Ferramentas");

        int current =
            static_cast<int>(
                m_shapeType);

        static const char *items[] =
            {
                "Esfera",
                "Bloco",
                "Cilindro"};

        if (ImGui::Combo(
                "Forma",
                &current,
                items,
                IM_ARRAYSIZE(items)))
        {
            m_shapeType =
                static_cast<ShapeType>(
                    current);

            rebuildMesh();
        }

        if (
            ImGui::DragFloat3(
                "Octree Scale",
                m_octreeScale.data_ptr(),
                0.01f,
                0.1f,
                100.0f))
        {
            rebuildMesh();
        }

        ImGui::Separator();

        ImGui::Checkbox(
            "Faces",
            &m_showFaces);

        ImGui::Checkbox(
            "Arestas",
            &m_showEdges);

        ImGui::Checkbox(
            "Vertices",
            &m_showVertices);

        ImGui::Separator();

        switch (m_shapeType)
        {
        case ShapeType::Sphere:
        {
            if (ImGui::DragFloat(
                    "Raio",
                    &m_sphere.radius(),
                    0.1f,
                    0.1f,
                    100.0f))
            {
                rebuildMesh();
            }

            break;
        }

        case ShapeType::Cylinder:
        {
            bool changed = false;

            changed |=
                ImGui::DragFloat(
                    "Raio",
                    &m_cylinder.radius(),
                    0.1f,
                    0.1f,
                    100.0f);

            changed |=
                ImGui::DragFloat(
                    "Altura",
                    &m_cylinder.height(),
                    0.1f,
                    0.1f,
                    100.0f);

            if (changed)
            {
                rebuildMesh();
            }

            break;
        }

        case ShapeType::Block:
        {
            static float width =
                4.0f;

            static float height =
                5.0f;

            static float depth =
                3.0f;

            bool changed = false;

            changed |=
                ImGui::DragFloat(
                    "Largura",
                    &width,
                    0.1f,
                    0.1f,
                    100.0f);

            changed |=
                ImGui::DragFloat(
                    "Altura",
                    &height,
                    0.1f,
                    0.1f,
                    100.0f);

            changed |=
                ImGui::DragFloat(
                    "Profundidade",
                    &depth,
                    0.1f,
                    0.1f,
                    100.0f);

            if (changed)
            {
                m_block =
                    Block(
                        width,
                        height,
                        depth);

                rebuildMesh();
            }

            break;
        }
        }

        ImGui::Separator();

        ImGui::Text(
            "Vertices: %zu",
            mesh.vertices().size());

        ImGui::Text(
            "Faces: %zu",
            mesh.faces().size());

        ImGui::End();
    }

    void rebuildMesh()
    {
        switch (m_shapeType)
        {
        case ShapeType::Sphere:
        {
            mesh =
                octreeFromShape(
                    m_sphere)
                    .scaleBounds(m_octreeScale)
                    .toMesh();
            break;
        }

        case ShapeType::Block:
        {
            mesh =
                octreeFromShape(
                    m_block)
                    .scaleBounds(m_octreeScale)
                    .toMesh();
            break;
        }

        case ShapeType::Cylinder:
        {
            mesh =
                octreeFromShape(
                    m_cylinder)
                    .scaleBounds(m_octreeScale)
                    .toMesh();
            break;
        }
        }
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

    static OctreeState sphereBuild(AABB const &aabb, Sphere const &sphere)
    {
        auto const min = aabb.minimum();
        auto const max = aabb.maximum();

        Vec3 const furthestVertex{
            (std::abs(min[0]) > std::abs(max[0])) ? min[0] : max[0],
            (std::abs(min[1]) > std::abs(max[1])) ? min[1] : max[1],
            (std::abs(min[2]) > std::abs(max[2])) ? min[2] : max[2]};

        auto const sqrRadius = sphere.radius() * sphere.radius();
        if (furthestVertex.dot(furthestVertex) <= sqrRadius)
        {
            return OctreeState::Filled;
        }

        Vec3 const closestPoint{
            std::clamp(0.0f, min[0], max[0]),
            std::clamp(0.0f, min[1], max[1]),
            std::clamp(0.0f, min[2], max[2])};

        if (closestPoint.dot(closestPoint) <= sqrRadius)
        {
            return OctreeState::Branch;
        }

        return OctreeState::Empty;
    }

    static OctreeState blockBuild(AABB const &aabb, Block const &block)
    {
        auto const half = block.boundSize() * 0.5f;
        auto const min = aabb.minimum().to_vector();
        auto const max = aabb.maximum().to_vector();

        Vec3 const furthestVertex{
            (std::abs(min[0]) > std::abs(max[0])) ? min[0] : max[0],
            (std::abs(min[1]) > std::abs(max[1])) ? min[1] : max[1],
            (std::abs(min[2]) > std::abs(max[2])) ? min[2] : max[2]};

        Vec3 u{1.0f, 0.0f, 0.0f};
        Vec3 v{0.0f, 1.0f, 0.0f};
        Vec3 w{0.0f, 0.0f, 1.0f};

        if (std::abs(furthestVertex.dot(u)) <= half[0] &&
            std::abs(furthestVertex.dot(v)) <= half[1] &&
            std::abs(furthestVertex.dot(w)) <= half[2])
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

    static OctreeState cylinderBuild(AABB const &aabb, Cylinder const &cylinder)
    {
        auto const sqrRadius = cylinder.radius() * cylinder.radius();
        auto const halfHeight = cylinder.height() * 0.5f;

        auto const min = aabb.minimum();
        auto const max = aabb.maximum();

        Vec3 axisY{0.0f, 1.0f, 0.0f};

        Vec3 const furthestVertex{
            (std::abs(min[0]) > std::abs(max[0])) ? min[0] : max[0],
            (std::abs(min[1]) > std::abs(max[1])) ? min[1] : max[1],
            (std::abs(min[2]) > std::abs(max[2])) ? min[2] : max[2]};

        float const furthestHeightProj = std::abs(furthestVertex.dot(axisY));

        Vec3 const furthestRadial = furthestVertex - (furthestVertex.dot(axisY) * axisY);
        float const furthestRadialSqr = furthestRadial.dot(furthestRadial);

        if (furthestRadialSqr <= sqrRadius && furthestHeightProj <= halfHeight)
        {
            return OctreeState::Filled;
        }

        Vec3 const closestPoint{
            std::clamp(0.0f, min[0], max[0]),
            std::clamp(0.0f, min[1], max[1]),
            std::clamp(0.0f, min[2], max[2])};

        float const closestHeightProj = closestPoint.dot(axisY);
        Vec3 const closestRadial = closestPoint - (closestPoint.dot(axisY) * axisY);
        float const closestRadialSqr = closestRadial.dot(closestRadial);

        if (closestRadialSqr > sqrRadius ||
            closestHeightProj < -halfHeight ||
            closestHeightProj > halfHeight)
        {
            return OctreeState::Empty;
        }

        return OctreeState::Branch;
    }

    static Octree octreeFromShape(
        const Shape &shape,
        std::size_t maxDepth = 5)
    {
        const auto bounds =
            shape.boundSize();

        const auto half =
            bounds * 0.5f;

        Octree tree(
            {-half[0], -half[1], -half[2]},
            {half[0], half[1], half[2]});

        if (auto sphere = dynamic_cast<const Sphere *>(&shape))
        {
            tree.build(
                [sphere, maxDepth](const AABB &bounds,
                                   std::size_t depth)
                {
                    auto result =
                        sphereBuild(
                            bounds,
                            *sphere);

                    if (
                        depth > maxDepth &&
                        result == OctreeState::Branch)
                    {
                        return OctreeState::Filled;
                    }

                    return result;
                });

            return tree;
        }

        if (auto block = dynamic_cast<const Block *>(&shape))
        {
            tree.build(
                [block, maxDepth](const AABB &bounds,
                                  std::size_t depth)
                {
                    auto result =
                        blockBuild(
                            bounds,
                            *block);

                    if (
                        depth > maxDepth &&
                        result == OctreeState::Branch)
                    {
                        return OctreeState::Filled;
                    }

                    return result;
                });

            return tree;
        }

        if (auto cylinder = dynamic_cast<const Cylinder *>(&shape))
        {
            tree.build(
                [cylinder, maxDepth](const AABB &bounds,
                                     std::size_t depth)
                {
                    auto result =
                        cylinderBuild(
                            bounds,
                            *cylinder);

                    if (
                        depth > maxDepth &&
                        result == OctreeState::Branch)
                    {
                        return OctreeState::Filled;
                    }

                    return result;
                });

            return tree;
        }

        throw std::runtime_error(
            "Shape nao suportado pela voxelizacao");
    }
};

int main()
{
    Trabalho01 app;
    app.run(
        1280,
        720,
        "Modelador Geometrico");
}