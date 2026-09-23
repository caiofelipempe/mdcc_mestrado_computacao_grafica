#pragma once

#include "drawer.hpp"
#include <vector>

class DrawerOpengl final :
    public Drawer
{
public:

    ~DrawerOpengl() override = default;

    using Drawer::drawVertices;
    using Drawer::drawLines;
    using Drawer::drawFaces;
    using Drawer::drawMeshes;

    void frameBegin() override;
    void frameEnd() override;

    void drawVertices(
        const geometry::Point3f* points,
        std::size_t count,
        const Color& color,
        float size = 5.0f) override;

    void drawLines(
        const geometry::Point3f* points,
        std::size_t count,
        const Color& color,
        float width = 1.0f) override;

    void drawFaces(
        const geometry::Point3f* points,
        std::size_t count,
        const Color& color) override;

    void drawMeshes(
        const geometry::Mesh3f* meshes,
        std::size_t count,
        const Color& faceColor,
        const Color& edgeColor,
        const Color& vertexColor) override;

private:

    struct FaceVertex
    {
        geometry::Point3f normal;
        geometry::Point3f position;
    };

    struct BatchVertices
    {
        Color color;
        float size;
        std::vector<geometry::Point3f> points;
    };

    struct BatchLines
    {
        Color color;
        float width;
        std::vector<geometry::Point3f> points;
    };

    struct BatchFaces
    {
        Color color;
        std::vector<FaceVertex> vertices;
    };

    std::vector<BatchVertices> m_pendingVertices;
    std::vector<BatchLines> m_pendingLines;
    std::vector<BatchFaces> m_pendingFaces;
};