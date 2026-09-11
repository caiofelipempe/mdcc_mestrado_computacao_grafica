#pragma once

#include "drawer.hpp"

class DrawerOpenGL final : public Drawer
{
public:
    void drawVertex(
        const geometry::Point3f& point,
        const geometry::Color& color,
        float size = 5.0f
    ) override;

    void drawLine(
        const geometry::Point3f& a,
        const geometry::Point3f& b,
        const geometry::Color& color,
        float width = 1.0f
    ) override;

    void drawFace(
        const geometry::Point3f& a,
        const geometry::Point3f& b,
        const geometry::Point3f& c,
        const geometry::Color& color
    ) override;

    void drawMesh(
        const geometry::Mesh3f& mesh,
        const geometry::Color& color
    ) override;
};