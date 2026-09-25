#include "renderer_glfw_opengl.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <numbers>
#include <string>
#include <vector>

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

    constexpr float kDegToRad =
        std::numbers::pi_v<float> / 180.0f;

    // -------------------------------------------------------------------
    // Classificacao AABB-vs-forma. Mesma logica de antes, so que agora
    // operando direto sobre Sphere/Block/Cylinder (sem uma classe
    // "editor" por forma) — a forma nao sabe nada sobre octree, transform
    // ou UI; isso tudo fica em SceneObject.
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

    // Parser sequencial da String ('0' vazio, '1' preenchido, '{...}'
    // subdividido) — mesma logica de antes. O consumo de `pos` acompanha a
    // ordem de visita (prefix/DFS) com que Octree::build chama o
    // classificador, entao nao depende de posicao/rotacao.
    OctreeState parseNextStringToken(const std::string &text, std::size_t &pos)
    {
        if (pos >= text.size())
        {
            return OctreeState::Empty;
        }

        switch (text[pos++])
        {
        case '1':
            return OctreeState::Filled;
        case '{':
            return OctreeState::Branch;
        case '0':
        case '}':
        default:
            return OctreeState::Empty;
        }
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

    // -------------------------------------------------------------------
    // SceneObject: unico tipo de item da lista de objetos. Guarda os
    // parametros de todas as formas possiveis (so os do `kind` atual sao
    // usados/mostrados) e, para formas (nao para String), uma origem e
    // rotacao proprias.
    //
    // Origem/rotacao NAO sao aplicadas como transform de malha depois de
    // gerar a octree: elas entram na propria classificacao (classify
    // recebe o AABB do nó em espaco de MUNDO, leva pro espaco local do
    // shape via worldToLocal, e testa o shape ali) — ou seja, a octree de
    // cada objeto ja "nasce" no lugar/orientacao certos.
    //
    // String nao tem origem/rotacao: seu conteudo já é posicional (ordem
    // de caracteres = ordem de visita da octree), aplicar uma transform
    // nao faria sentido sem reamostrar o padrao.
    // -------------------------------------------------------------------

    struct SceneObject
    {
        ObjectKind kind = ObjectKind::Sphere;

        Sphere sphere{10.0f};
        Block block{4.0f, 5.0f, 3.0f};
        Cylinder cylinder{5.0f, 5.0f};
        std::string text = "{11111111}";

        Vec3f position{0.0f, 0.0f, 0.0f};
        Vec3f rotationDegrees{0.0f, 0.0f, 0.0f};

        Color faceColor = Color::DarkGreen();
        Color edgeColor = Color::Yellow();
        Color vertexColor = Color::Red();

        bool hasTransform() const
        {
            return kind != ObjectKind::String;
        }

        Rot3f rotator() const
        {
            Rot3f rot = Rot3f::identity();
            rot.rotateWorld({1.0f, 0.0f, 0.0f}, rotationDegrees[0] * kDegToRad);
            rot.rotateWorld({0.0f, 1.0f, 0.0f}, rotationDegrees[1] * kDegToRad);
            rot.rotateWorld({0.0f, 0.0f, 1.0f}, rotationDegrees[2] * kDegToRad);
            return rot;
        }

        // Inversa da rotacao = conjugado do quaternion (x,y,z,w) ->
        // (-x,-y,-z,w), valido porque rotator() sempre devolve um
        // quaternion normalizado.
        Vec3f worldToLocal(const Vec3f &world) const
        {
            const auto rot = rotator();
            const auto q = rot.quaternion();
            const Rot3f inverse{Quatf{-q[0], -q[1], -q[2], q[3]}};
            return inverse.rotateVector(world - position);
        }

        // Transforma os 8 cantos do AABB do no (espaco de mundo) para o
        // espaco local do objeto e delega para o classify() da forma —
        // assim cada objeto e voxelizado na posicao/rotacao certas, sem
        // nenhum pos-processamento de malha.
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

        // Constroi a Octree deste objeto usando os bounds GLOBAIS
        // compartilhados por toda a cena (definidos no painel) — e por
        // isso que todas as octrees ficam consistentes entre si mesmo
        // sendo construidas/desenhadas separadamente.
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

        bool drawUI()
        {
            bool changed = false;

            int current = static_cast<int>(kind);
            if (ImGui::Combo(
                    "Tipo", &current, kObjectKindLabels.data(),
                    static_cast<int>(kObjectKindLabels.size())))
            {
                kind = static_cast<ObjectKind>(current);
                changed = true;
            }

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
                changed |= ImGui::InputTextMultiline(
                    "Padrao", &text, ImVec2(0.0f, 80.0f));
                break;
            }

            if (hasTransform())
            {
                changed |= ImGui::DragFloat3("Posicao", position.data_ptr(), kDragSpeed);
                changed |= ImGui::DragFloat3(
                    "Rotacao (graus)", rotationDegrees.data_ptr(), 1.0f);
            }

            ImGui::Separator();
            ImGui::Text("Cores");

            changed |= drawColorEdit("Face", faceColor);
            changed |= drawColorEdit("Aresta", edgeColor);
            changed |= drawColorEdit("Vertice", vertexColor);

            return changed;
        }

    private:
        static bool drawColorEdit(const char *label, Color &color)
        {
            float rgba[4] = {color.r, color.g, color.b, color.a};

            if (ImGui::ColorEdit4(label, rgba))
            {
                color = Color(rgba[0], rgba[1], rgba[2], rgba[3]);
                return true;
            }

            return false;
        }
    };
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

        m_objects.push_back(SceneObject{});

        rebuildMeshes();
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
        drawScene(drawer);
    }

    void onUI() override {
        imguiStartRender();
        imguiEndRender();
    }

private:
    std::vector<SceneObject> m_objects;
    std::vector<Mesh3f> m_meshes;

    // Espaco global compartilhado por TODAS as octrees da cena — e o que
    // garante que ficam consistentes entre si mesmo desenhadas
    // separadamente.
    Vec3f m_worldMin{-10.0f, -10.0f, -10.0f};
    Vec3f m_worldMax{10.0f, 10.0f, 10.0f};

    Vec3f m_octreeScale{1.0f, 1.0f, 1.0f};
    int m_octreeDepth = 5;

    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;

    bool m_showFaces = true;
    bool m_showEdges = false;
    bool m_showVertices = false;

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

        bool changed = false;

        ImGui::Text("Espaco Global (bounds compartilhados)");
        changed |= ImGui::DragFloat3("Min", m_worldMin.data_ptr(), kDragSpeed);
        changed |= ImGui::DragFloat3("Max", m_worldMax.data_ptr(), kDragSpeed);
        changed |= ImGui::DragFloat3(
            "Octree Scale", m_octreeScale.data_ptr(), 0.01f, 0.1f, 100.0f);
        changed |= ImGui::SliderInt("Octree Depth", &m_octreeDepth, 0, 5);

        ImGui::Separator();
        ImGui::Checkbox("Faces", &m_showFaces);
        ImGui::Checkbox("Arestas", &m_showEdges);
        ImGui::Checkbox("Vertices", &m_showVertices);
        ImGui::Separator();

        if (ImGui::Button("Adicionar Objeto"))
        {
            m_objects.push_back(SceneObject{});
            changed = true;
        }

        ImGui::BeginChild("SceneObjects", ImVec2(0.0f, 360.0f), true);

        int removeIndex = -1;

        for (int i = 0; i < static_cast<int>(m_objects.size()); ++i)
        {
            ImGui::PushID(i);
            ImGui::Text("Objeto %d", i + 1);

            changed |= m_objects[i].drawUI();

            if (ImGui::Button("Remover"))
            {
                removeIndex = i;
            }

            ImGui::Separator();
            ImGui::PopID();
        }

        ImGui::EndChild();

        if (removeIndex >= 0)
        {
            m_objects.erase(m_objects.begin() + removeIndex);
            changed = true;
        }

        if (changed)
        {
            rebuildMeshes();
        }

        std::size_t totalVertices = 0;
        std::size_t totalFaces = 0;

        for (const auto &mesh : m_meshes)
        {
            totalVertices += mesh.vertices().size();
            totalFaces += mesh.faces().size();
        }

        ImGui::Separator();
        ImGui::Text("Objetos: %zu", m_objects.size());
        ImGui::Text("Vertices: %zu", totalVertices);
        ImGui::Text("Faces: %zu", totalFaces);

        ImGui::End();
    }

    void drawScene(Drawer &drawer)
    {
        const Color transparent = Color::Transparent();

        // Cada octree/malha foi construida com os MESMOS bounds globais,
        // entao desenha-las separadamente ainda resulta numa cena
        // espacialmente consistente.
        for (std::size_t i = 0; i < m_objects.size() && i < m_meshes.size(); ++i)
        {
            const auto &object = m_objects[i];
            const auto &mesh = m_meshes[i];

            drawer.drawMesh(
                mesh,
                m_showFaces ? object.faceColor : transparent,
                m_showEdges ? object.edgeColor : transparent,
                m_showVertices ? object.vertexColor : transparent);
        }
    }

    // ---- Construção das malhas ------------------------------------------

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
            {
                mesh.buildEdgesFromFaces();
            }

            m_meshes.push_back(std::move(mesh));
        }
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