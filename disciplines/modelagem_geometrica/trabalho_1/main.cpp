#include "renderer_glfw_opengl.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <numbers>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
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

    // Aplica rotação (em torno da origem) e depois translação a cada
    // vértice, preservando a topologia (edges/faces) da malha de origem.
    Mesh3f transformMesh(const Mesh3f &src, const Vec3f &position, const Rot3f &rotation)
    {
        Mesh3f dst;

        for (const auto &vertex : src.vertices())
        {
            const Vec3f transformed = rotation.rotateVector(vertex.to_vector()) + position;
            (void)dst.addVertex(Point3f{transformed[0], transformed[1], transformed[2]});
        }

        for (const auto &edge : src.edges())
        {
            dst.addEdge(edge.v1, edge.v2);
        }

        for (const auto &face : src.faces())
        {
            dst.addFace(face.indices[0], face.indices[1], face.indices[2]);
        }

        return dst;
    }

    // Concatena src em dst reindexando os vértices — mesma lógica do
    // appendMesh privado usado internamente por Octree::toMesh.
    void appendMesh(Mesh3f &dst, const Mesh3f &src)
    {
        const auto offset = dst.vertexCount();

        for (const auto &vertex : src.vertices())
        {
            (void)dst.addVertex(vertex);
        }

        for (const auto &edge : src.edges())
        {
            dst.addEdge(edge.v1 + offset, edge.v2 + offset);
        }

        for (const auto &face : src.faces())
        {
            dst.addFace(
                face.indices[0] + offset,
                face.indices[1] + offset,
                face.indices[2] + offset);
        }
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
                [this](const AABB &bounds)
                { return classify(bounds); });
        }

        // Público para ser reaproveitado pelo ModelEditor, que testa cada
        // instância diretamente (sem passar por buildOctreeFromTest).
        OctreeState classify(const AABB &aabb) const
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
                [this](const AABB &bounds)
                { return classify(bounds); });
        }

        // Público para ser reaproveitado pelo ModelEditor, que testa cada
        // instância diretamente (sem passar por buildOctreeFromTest).
        OctreeState classify(const AABB &aabb) const
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
                [this](const AABB &bounds)
                { return classify(bounds); });
        }

        // Público para ser reaproveitado pelo ModelEditor, que testa cada
        // instância diretamente (sem passar por buildOctreeFromTest).
        OctreeState classify(const AABB &aabb) const
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

    enum class BooleanOp
    {
        OnlyA,
        OnlyB,
        Union,
        Intersection,
        Difference
    };

    constexpr std::array<const char *, 5> kBooleanOpLabels{
        "A",
        "B",
        "Uniao (A + B)",
        "Intersecao (A & B)",
        "Diferenca (A - B)"};

    // Duas octrees descritas por strings ('0' vazio, '1' preenchido,
    // '{...}' subdividido) que compartilham o mesmo AABB, combinadas pela
    // operação booleana escolhida. O compartilhamento do AABB é o que
    // garante que os filhos de A e B cobrem as mesmas regiões.
    struct StringEditor
    {
        static constexpr const char *kLabel = "String";

        std::string textA = "{11111111}";
        std::string textB = "{10101010}";
        BooleanOp operation = BooleanOp::Union;

        std::array<float, 3> minBound{-10.0f, -10.0f, -10.0f};
        std::array<float, 3> maxBound{10.0f, 10.0f, 10.0f};

        bool drawUI()
        {
            bool changed = false;

            changed |= ImGui::InputTextMultiline(
                "Octree A", &textA, ImVec2(0.0f, 80.0f));
            changed |= ImGui::InputTextMultiline(
                "Octree B", &textB, ImVec2(0.0f, 80.0f));

            int current = static_cast<int>(operation);
            if (ImGui::Combo(
                    "Operacao", &current, kBooleanOpLabels.data(),
                    static_cast<int>(kBooleanOpLabels.size())))
            {
                operation = static_cast<BooleanOp>(current);
                changed = true;
            }

            ImGui::Separator();
            ImGui::Text("Tamanho do AABB (compartilhado)");
            changed |= ImGui::DragFloat3("Min", minBound.data(), kDragSpeed);
            changed |= ImGui::DragFloat3("Max", maxBound.data(), kDragSpeed);

            if (ImGui::Button("Reconstruir"))
            {
                changed = true;
            }

            return changed;
        }

        Octree build(int maxDepth) const
        {
            Octree a = buildFromText(textA, maxDepth);
            Octree b = buildFromText(textB, maxDepth);

            switch (operation)
            {
            case BooleanOp::OnlyA:
                return a;
            case BooleanOp::OnlyB:
                return b;
            case BooleanOp::Union:
                a.unite(b);
                break;
            case BooleanOp::Intersection:
                a.intersect(b);
                break;
            case BooleanOp::Difference:
                a.subtract(b);
                break;
            }

            return a;
        }

    private:
        Octree buildFromText(const std::string &text, int maxDepth) const
        {
            Octree tree(
                {minBound[0], minBound[1], minBound[2]},
                {maxBound[0], maxBound[1], maxBound[2]});

            std::size_t pos = 0;

            tree.build(
                [&text, maxDepth, &pos](const AABB &, std::size_t depth)
                {
                    const auto result = parseNext(text, pos);

                    if (depth >= static_cast<std::size_t>(maxDepth) &&
                        result == OctreeState::Branch)
                    {
                        return OctreeState::Filled;
                    }

                    return result;
                });

            return tree;
        }

        static OctreeState parseNext(const std::string &text, std::size_t &pos)
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
    };

    // -------------------------------------------------------------------
    // Variante "octree completa": os mesmos 4 tipos do combo principal
    // (inclui StringEditor, ao contrário de PrimitiveEditor). Usada pelo
    // OctreeGroupEditor, cujos itens são octrees construídas de verdade
    // (com seus próprios limites — ver StringEditor), não formas testadas
    // por AABB.
    // -------------------------------------------------------------------

    using OctreeShapeEditor =
        std::variant<SphereEditor, BlockEditor, CylinderEditor, StringEditor>;

    constexpr std::array<const char *, 4> kOctreeShapeLabels{
        SphereEditor::kLabel,
        BlockEditor::kLabel,
        CylinderEditor::kLabel,
        StringEditor::kLabel};

    OctreeShapeEditor makeOctreeShape(std::size_t index)
    {
        switch (index)
        {
        case 0:
            return SphereEditor{};
        case 1:
            return BlockEditor{};
        case 2:
            return CylinderEditor{};
        default:
            return StringEditor{};
        }
    }

    // Constrói a Octree do editor e converte pra malha, aplicando a escala
    // global (exceto pro StringEditor, cujos limites já são explícitos na
    // própria UI) — a mesma regra usada no dispatch de Trabalho01::rebuildMesh.
    template <typename Editor>
    Mesh3f buildMeshFor(Editor &editor, int maxDepth, const Vec3f &scale)
    {
        Octree tree = editor.build(maxDepth);

        if constexpr (std::is_same_v<std::decay_t<Editor>, StringEditor>)
        {
            return tree.toMesh();
        }
        else
        {
            return tree.scaleBounds(scale).toMesh();
        }
    }

    // -------------------------------------------------------------------
    // Modelo: lista de shapes primitivos, cada um com sua própria posição
    // e rotação. Reaproveita Sphere/Block/CylinderEditor (dados + UI +
    // classify) — só não inclui StringEditor, que não representa um shape
    // transformável no sentido geométrico usado aqui.
    // -------------------------------------------------------------------

    using PrimitiveEditor = std::variant<SphereEditor, BlockEditor, CylinderEditor>;
    constexpr std::array<const char *, 3> kPrimitiveLabels{
        SphereEditor::kLabel,
        BlockEditor::kLabel,
        CylinderEditor::kLabel};

    PrimitiveEditor makePrimitive(std::size_t index)
    {
        switch (index)
        {
        case 0:
            return SphereEditor{};
        case 1:
            return BlockEditor{};
        default:
            return CylinderEditor{};
        }
    }

    constexpr float kDegToRad =
        std::numbers::pi_v<float> / 180.0f;

    struct ModelInstance
    {
        PrimitiveEditor editor = SphereEditor{};
        Vec3f position{0.0f, 0.0f, 0.0f};

        // Graus, aplicados em ordem X (pitch) -> Y (yaw) -> Z (roll) via
        // Rot3f::rotateWorld — ver rotator().
        Vec3f rotationDegrees{0.0f, 0.0f, 0.0f};

        bool drawUI()
        {
            bool changed = false;

            int current = static_cast<int>(editor.index());
            if (ImGui::Combo(
                    "Tipo", &current, kPrimitiveLabels.data(),
                    static_cast<int>(kPrimitiveLabels.size())))
            {
                editor = makePrimitive(static_cast<std::size_t>(current));
                changed = true;
            }

            changed |= std::visit([](auto &e)
                                  { return e.drawUI(); }, editor);
            changed |= ImGui::DragFloat3("Posicao", position.data_ptr(), kDragSpeed);
            changed |= ImGui::DragFloat3(
                "Rotacao (graus)", rotationDegrees.data_ptr(), 1.0f);

            return changed;
        }

        Vec3f localHalfExtents() const
        {
            return std::visit(
                [](auto &e)
                { return e.shape.boundSize() * 0.5f; }, editor);
        }

        Rot3f rotator() const
        {
            Rot3f rot = Rot3f::identity();
            rot.rotateWorld({1.0f, 0.0f, 0.0f}, rotationDegrees[0] * kDegToRad);
            rot.rotateWorld({0.0f, 1.0f, 0.0f}, rotationDegrees[1] * kDegToRad);
            rot.rotateWorld({0.0f, 0.0f, 1.0f}, rotationDegrees[2] * kDegToRad);
            return rot;
        }

        Vec3f localToWorld(const Vec3f &local) const
        {
            return rotator().rotateVector(local) + position;
        }

        // Inversa da rotação = conjugado do quaternion (x,y,z,w) -> (-x,-y,-z,w),
        // válido porque rotator() sempre devolve um quaternion normalizado.
        Vec3f worldToLocal(const Vec3f &world) const
        {
            const auto rot = rotator();
            const auto q = rot.quaternion();
            const Rot3f inverse{Quatf{-q[0], -q[1], -q[2], q[3]}};
            return inverse.rotateVector(world - position);
        }

        // Transforma os 8 cantos da AABB do nó (espaço "mundo" do modelo)
        // para o espaço local do shape e delega pro classify() dele — assim
        // cada instância é voxelizada exatamente como o shape avulso seria,
        // só que na posição/rotação certas.
        OctreeState classify(const AABB &worldBounds) const
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

            return std::visit(
                [&localBounds](auto &e)
                { return e.classify(localBounds); }, editor);
        }
    };

    struct ModelEditor
    {
        static constexpr const char *kLabel = "Modelo";

        std::vector<ModelInstance> instances;

        bool drawUI()
        {
            bool changed = false;

            if (ImGui::Button("Adicionar Modelo"))
            {
                instances.push_back(ModelInstance{});
                changed = true;
            }

            ImGui::BeginChild("ModelInstances", ImVec2(0.0f, 260.0f), true);

            int removeIndex = -1;

            for (int i = 0; i < static_cast<int>(instances.size()); ++i)
            {
                ImGui::PushID(i);
                ImGui::Text("Shape %d", i + 1);

                changed |= instances[i].drawUI();

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
                instances.erase(instances.begin() + removeIndex);
                changed = true;
            }

            return changed;
        }

        Octree build(int maxDepth) const
        {
            const auto [worldMin, worldMax] = computeWorldBounds();

            Octree tree(worldMin, worldMax);

            tree.build(
                [this, maxDepth](const AABB &bounds, std::size_t depth)
                {
                    const auto result = classify(bounds);

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
        std::pair<Point3f, Point3f> computeWorldBounds() const
        {
            if (instances.empty())
            {
                return {Point3f{-1.0f, -1.0f, -1.0f}, Point3f{1.0f, 1.0f, 1.0f}};
            }

            Vec3f minCorner{};
            Vec3f maxCorner{};
            bool first = true;

            for (const auto &instance : instances)
            {
                const auto half = instance.localHalfExtents();

                for (int i = 0; i < 8; ++i)
                {
                    const Vec3f localCorner{
                        (i & 1) ? half[0] : -half[0],
                        (i & 2) ? half[1] : -half[1],
                        (i & 4) ? half[2] : -half[2]};

                    const Vec3f worldCorner = instance.localToWorld(localCorner);

                    if (first)
                    {
                        minCorner = worldCorner;
                        maxCorner = worldCorner;
                        first = false;
                    }
                    else
                    {
                        for (int a = 0; a < 3; ++a)
                        {
                            minCorner[a] = std::min(minCorner[a], worldCorner[a]);
                            maxCorner[a] = std::max(maxCorner[a], worldCorner[a]);
                        }
                    }
                }
            }

            return {Point3f{minCorner[0], minCorner[1], minCorner[2]},
                    Point3f{maxCorner[0], maxCorner[1], maxCorner[2]}};
        }

        // União dos shapes: um nó é Filled se QUALQUER instância o preenche
        // totalmente. Não subtrai sobreposição entre shapes (aproximação
        // consistente com o resto do app, que já testa contra AABB).
        OctreeState classify(const AABB &worldBounds) const
        {
            bool anyFilled = false;
            bool anyNonEmpty = false;

            for (const auto &instance : instances)
            {
                const auto state = instance.classify(worldBounds);
                anyFilled |= (state == OctreeState::Filled);
                anyNonEmpty |= (state != OctreeState::Empty);
            }

            if (anyFilled)
                return OctreeState::Filled;
            if (anyNonEmpty)
                return OctreeState::Branch;
            return OctreeState::Empty;
        }
    };

    // -------------------------------------------------------------------
    // Grupo de octrees: cada item é uma octree "de verdade", com os mesmos
    // parâmetros do editor avulso (inclui String), mais posição e rotação.
    // Ao contrário do ModelEditor (que funde tudo numa única octree via
    // classify), aqui cada instância constrói sua própria Octree/Mesh
    // isoladamente e as malhas resultantes são só transformadas e
    // concatenadas — sem união em nível de voxel.
    // -------------------------------------------------------------------

    struct OctreeGroupInstance
    {
        OctreeShapeEditor editor = SphereEditor{};

        Vec3f position{0.0f, 0.0f, 0.0f};
        Vec3f rotationDegrees{0.0f, 0.0f, 0.0f};

        Color color = Color::DarkGreen();

        bool drawUI()
        {
            bool changed = false;

            int current = static_cast<int>(editor.index());

            if (ImGui::Combo(
                    "Tipo",
                    &current,
                    kOctreeShapeLabels.data(),
                    static_cast<int>(kOctreeShapeLabels.size())))
            {
                editor = makeOctreeShape(static_cast<std::size_t>(current));
                changed = true;
            }

            changed |= std::visit([](auto &e)
                                  { return e.drawUI(); },
                                  editor);

            changed |= ImGui::DragFloat3(
                "Posicao",
                position.data_ptr(),
                kDragSpeed);

            changed |= ImGui::DragFloat3(
                "Rotacao (graus)",
                rotationDegrees.data_ptr(),
                1.0f);

            float rgba[4] =
                {
                    color.r,
                    color.g,
                    color.b,
                    color.a};

            if (ImGui::ColorEdit4("Cor", rgba))
            {
                color = Color(
                    rgba[0],
                    rgba[1],
                    rgba[2],
                    rgba[3]);

                changed = true;
            }

            return changed;
        }

        Rot3f rotator() const
        {
            Rot3f rot = Rot3f::identity();

            rot.rotateWorld(
                {1.0f, 0.0f, 0.0f},
                rotationDegrees[0] * kDegToRad);

            rot.rotateWorld(
                {0.0f, 1.0f, 0.0f},
                rotationDegrees[1] * kDegToRad);

            rot.rotateWorld(
                {0.0f, 0.0f, 1.0f},
                rotationDegrees[2] * kDegToRad);

            return rot;
        }

        Mesh3f buildTransformedMesh(
            int maxDepth,
            const Vec3f &scale) const
        {

            Mesh3f localMesh = std::visit(
                [maxDepth, &scale](auto &e)
                { return buildMeshFor(e, maxDepth, scale); },
                editor);

            return transformMesh(
                localMesh,
                position,
                rotator());
        }
    };

    struct OctreeGroupEditor
    {
        static constexpr const char *kLabel = "Grupo de Octrees";

        std::vector<OctreeGroupInstance> instances;

        bool drawUI()
        {
            bool changed = false;

            if (ImGui::Button("Adicionar Octree"))
            {
                instances.push_back(OctreeGroupInstance{});
                changed = true;
            }

            ImGui::BeginChild("OctreeGroupInstances", ImVec2(0.0f, 260.0f), true);

            int removeIndex = -1;

            for (int i = 0; i < static_cast<int>(instances.size()); ++i)
            {
                ImGui::PushID(i);
                ImGui::Text("Octree %d", i + 1);

                changed |= instances[i].drawUI();

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
                instances.erase(instances.begin() + removeIndex);
                changed = true;
            }

            return changed;
        }

        // Assinatura própria (Mesh3f, não Octree): cada instância já é uma
        // malha completa, então não há uma Octree única pra devolver — só
        // faz sentido tratada como caso especial em Trabalho01::rebuildMesh.
        Mesh3f build(int maxDepth, const Vec3f &scale) const
        {
            Mesh3f result;

            for (const auto &instance : instances)
            {
                appendMesh(result, instance.buildTransformedMesh(maxDepth, scale));
            }

            return result;
        }
    };

    using ShapeEditor = std::variant<
        SphereEditor, BlockEditor, CylinderEditor, StringEditor, ModelEditor,
        OctreeGroupEditor>;

    constexpr std::array<const char *, 6> kShapeLabels{
        SphereEditor::kLabel,
        BlockEditor::kLabel,
        CylinderEditor::kLabel,
        StringEditor::kLabel,
        ModelEditor::kLabel,
        OctreeGroupEditor::kLabel};

    ShapeEditor makeEditor(std::size_t index)
    {
        switch (index)
        {
        case 0:
            return SphereEditor{};
        case 1:
            return BlockEditor{};
        case 2:
            return CylinderEditor{};
        case 3:
            return StringEditor{};
        case 4:
            return ModelEditor{};
        default:
            return OctreeGroupEditor{};
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
        drawScene(drawer);
    }

    void onUI() override {
        imguiStartRender();
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

        changed |= std::visit([](auto &editor)
                              { return editor.drawUI(); }, m_editor);

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

        if (auto *group = std::get_if<OctreeGroupEditor>(&m_editor))
        {
            for (const auto &instance : group->instances)
            {
                Mesh3f mesh =
                    instance.buildTransformedMesh(
                        m_octreeDepth,
                        m_octreeScale);

                if (mesh.edges().empty())
                {
                    mesh.buildEdgesFromFaces();
                }

                drawer.drawMesh(
                    mesh,
                    m_showFaces
                        ? instance.color
                        : transparent,
                    m_showEdges
                        ? Color::Yellow()
                        : transparent,
                    m_showVertices
                        ? Color::Red()
                        : transparent);
            }

            return;
        }

        if (m_mesh.edges().empty())
        {
            m_mesh.buildEdgesFromFaces();
        }

        drawer.drawMesh(
            m_mesh,
            m_showFaces
                ? Color::DarkGreen()
                : transparent,
            m_showEdges
                ? Color::Yellow()
                : transparent,
            m_showVertices
                ? Color::Red()
                : transparent);
    }

    // ---- Construção da malha ------------------------------------------

    void rebuildMesh()
    {
        m_mesh = std::visit(
            [this](auto &editor) -> Mesh3f
            {
                using Editor = std::decay_t<decltype(editor)>;

                if constexpr (std::is_same_v<Editor, OctreeGroupEditor>)
                {
                    return editor.build(m_octreeDepth, m_octreeScale);
                }
                else
                {
                    return buildMeshFor(editor, m_octreeDepth, m_octreeScale);
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