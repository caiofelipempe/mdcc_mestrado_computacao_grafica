#include "drawer_opengl.hpp"
#include "color.hpp"

#include <GL/glew.h>

using namespace geometry;

namespace
{
    inline void emitVertex(const Point3f& point)
    {
        glVertex3fv(point.data_ptr());
    }

    inline void setColor(const Color& color)
    {
        glColor4f(color.r, color.g, color.b, color.a);
    }
}

void DrawerOpengl::frameBegin()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void DrawerOpengl::frameEnd()
{
    glFlush();
}

void DrawerOpengl::drawVertices(
    const Point3f* points,
    std::size_t count,
    const Color& color,
    float size)
{
    if (!points || count == 0) return;

    glPointSize(size);
    setColor(color);

    glBegin(GL_POINTS);
    for (std::size_t i = 0; i < count; ++i)
    {
        emitVertex(points[i]);
    }
    glEnd();
}

void DrawerOpengl::drawLines(
    const Point3f* points,
    std::size_t count,
    const Color& color,
    float width)
{
    if (!points || count == 0) return;

    glLineWidth(width);
    setColor(color);

    glBegin(GL_LINES);
    const std::size_t totalPoints = count * 2;
    for (std::size_t i = 0; i < totalPoints; ++i)
    {
        emitVertex(points[i]);
    }
    glEnd();
}

void DrawerOpengl::drawFaces(
    const Point3f* points,
    std::size_t count,
    const Color& color)
{
    if (!points || count == 0) return;

    setColor(color);

    glBegin(GL_TRIANGLES);
    const std::size_t totalPoints = count * 3;
    for (std::size_t i = 0; i < totalPoints; ++i)
    {
        emitVertex(points[i]);
    }
    glEnd();
}

void DrawerOpengl::drawMeshes(
    const Mesh3f* meshes,
    std::size_t count,
    const Color& color)
{
    if (!meshes || count == 0) return;

    std::vector<Point3f> faceVertices;
    std::vector<Point3f> lineVertices;

    for (std::size_t m = 0; m < count; ++m)
    {
        const auto& mesh = meshes[m];
        const auto& vertices = mesh.getVertices();
        const auto& edges = mesh.getEdges();
        const auto& faces = mesh.getFaces();

        if (vertices.empty()) continue;

        // 1. Faces da malha
        if (!faces.empty())
        {
            faceVertices.clear();
            faceVertices.reserve(faces.size() * 3);

            for (const auto& face : faces)
            {
                faceVertices.push_back(vertices[face.indices[0]]);
                faceVertices.push_back(vertices[face.indices[1]]);
                faceVertices.push_back(vertices[face.indices[2]]);
            }

            drawFaces(faceVertices, color);
        }

        // 2. Arestas da malha (wireframe)
        if (!edges.empty())
        {
            lineVertices.clear();
            lineVertices.reserve(edges.size() * 2);

            for (const auto& edge : edges)
            {
                lineVertices.push_back(vertices[edge.v1]);
                lineVertices.push_back(vertices[edge.v2]);
            }

            drawLines(lineVertices, Color{0.0f, 0.0f, 0.0f, 1.0f}, 1.0f);
        }

        // 3. Vértices da malha
        drawVertices(vertices, Color{1.0f, 0.0f, 0.0f, 1.0f}, 5.0f);
    }
}