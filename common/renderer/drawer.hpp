#pragma once

#include "point.hpp"
#include "mesh.hpp"
#include "color.hpp"

#include <array>
#include <cstddef>
#include <vector>

class Drawer
{
public:
    virtual ~Drawer() = default;

    virtual void frameBegin() = 0;
    virtual void frameEnd() = 0;

    // -------------------------------------------------------------
    // Primitivas em lote
    // -------------------------------------------------------------

    virtual void drawVertices(
        const geometry::Point3f* points,
        std::size_t count,
        const Color& color,
        float size = 5.0f) = 0;

    virtual void drawLines(
        const geometry::Point3f* points,
        std::size_t count,
        const Color& color,
        float width = 1.0f) = 0;

    virtual void drawFaces(
        const geometry::Point3f* points,
        std::size_t count,
        const Color& color) = 0;

    virtual void drawMeshes(
        const geometry::Mesh3f* meshes,
        std::size_t count,
        const Color& color) = 0;

    // -------------------------------------------------------------
    // Elementos individuais
    // -------------------------------------------------------------

    void drawVertex(
        const geometry::Point3f& point,
        const Color& color,
        float size = 5.0f)
    {
        drawVertices(
            &point,
            1,
            color,
            size);
    }

    void drawLine(
        const geometry::Point3f& a,
        const geometry::Point3f& b,
        const Color& color,
        float width = 1.0f)
    {
        const geometry::Point3f points[2] =
        {
            a,
            b
        };

        drawLines(
            points,
            1,
            color,
            width);
    }

    void drawFace(
        const geometry::Point3f& a,
        const geometry::Point3f& b,
        const geometry::Point3f& c,
        const Color& color)
    {
        const geometry::Point3f points[3] =
        {
            a,
            b,
            c
        };

        drawFaces(
            points,
            1,
            color);
    }

    void drawMesh(
        const geometry::Mesh3f& mesh,
        const Color& color)
    {
        drawMeshes(
            &mesh,
            1,
            color);
    }

    // -------------------------------------------------------------
    // Wrappers std::vector
    // -------------------------------------------------------------

    void drawVertices(
        const std::vector<geometry::Point3f>& points,
        const Color& color,
        float size = 5.0f)
    {
        if (!points.empty())
        {
            drawVertices(
                points.data(),
                points.size(),
                color,
                size);
        }
    }

    void drawLines(
        const std::vector<geometry::Point3f>& points,
        const Color& color,
        float width = 1.0f)
    {
        if (!points.empty())
        {
            drawLines(
                points.data(),
                points.size() / 2,
                color,
                width);
        }
    }

    void drawFaces(
        const std::vector<geometry::Point3f>& points,
        const Color& color)
    {
        if (!points.empty())
        {
            drawFaces(
                points.data(),
                points.size() / 3,
                color);
        }
    }

    void drawMeshes(
        const std::vector<geometry::Mesh3f>& meshes,
        const Color& color)
    {
        if (!meshes.empty())
        {
            drawMeshes(
                meshes.data(),
                meshes.size(),
                color);
        }
    }

    // -------------------------------------------------------------
    // Wrappers std::array
    // -------------------------------------------------------------

    template<std::size_t N>
    void drawVertices(
        const std::array<geometry::Point3f, N>& points,
        const Color& color,
        float size = 5.0f)
    {
        if constexpr (N > 0)
        {
            drawVertices(
                points.data(),
                N,
                color,
                size);
        }
    }

    template<std::size_t N>
    void drawLines(
        const std::array<geometry::Point3f, N>& points,
        const Color& color,
        float width = 1.0f)
    {
        if constexpr (N > 0)
        {
            drawLines(
                points.data(),
                N / 2,
                color,
                width);
        }
    }

    template<std::size_t N>
    void drawFaces(
        const std::array<geometry::Point3f, N>& points,
        const Color& color)
    {
        if constexpr (N > 0)
        {
            drawFaces(
                points.data(),
                N / 3,
                color);
        }
    }

    template<std::size_t N>
    void drawMeshes(
        const std::array<geometry::Mesh3f, N>& meshes,
        const Color& color)
    {
        if constexpr (N > 0)
        {
            drawMeshes(
                meshes.data(),
                N,
                color);
        }
    }
};