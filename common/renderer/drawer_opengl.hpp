#pragma once

#include "drawer.hpp"

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
};