#pragma once

#include "point.hpp"
#include "mesh.hpp"
#include "color.hpp"

class Drawer
{
public:
    virtual ~Drawer() = default;

    virtual void frameBegin() = 0;

    virtual void frameEnd() = 0;

    virtual void drawVertex(
        const geometry::Point3f& point,
        const geometry::Color& color,
        float size = 5.0f
    ) = 0;

    virtual void drawLine(
        const geometry::Point3f& a,
        const geometry::Point3f& b,
        const geometry::Color& color,
        float width = 1.0f
    ) = 0;

    virtual void drawFace(
        const geometry::Point3f& a,
        const geometry::Point3f& b,
        const geometry::Point3f& c,
        const geometry::Color& color
    ) = 0;

    virtual void drawMesh(
        const geometry::Mesh3f& mesh,
        const geometry::Color& color
    ) = 0;
};