#include "drawer_opengl.hpp"
#include "color.hpp"

#include <GL/glew.h>

using namespace geometry;

namespace
{
    inline void emitVertex(const Point3f &point)
    {
        glVertex3fv(point.data_ptr());
    }

    inline void setColor(const Color &color)
    {
        glColor4f(color.r, color.g, color.b, color.a);
    }
}

void DrawerOpengl::frameBegin()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);

    glEnable(GL_COLOR_MATERIAL);

    glColorMaterial(
        GL_FRONT_AND_BACK,
        GL_AMBIENT_AND_DIFFUSE);

    glShadeModel(GL_SMOOTH);

    GLfloat lightPosition[] =
        {
            20.0f,
            20.0f,
            20.0f,
            1.0f};

    GLfloat ambient[] =
        {
            0.20f,
            0.20f,
            0.20f,
            1.0f};

    GLfloat diffuse[] =
        {
            1.0f,
            1.0f,
            1.0f,
            1.0f};

    GLfloat specular[] =
        {
            1.0f,
            1.0f,
            1.0f,
            1.0f};

    glLightfv(
        GL_LIGHT0,
        GL_POSITION,
        lightPosition);

    glLightfv(
        GL_LIGHT0,
        GL_AMBIENT,
        ambient);

    glLightfv(
        GL_LIGHT0,
        GL_DIFFUSE,
        diffuse);

    glLightfv(
        GL_LIGHT0,
        GL_SPECULAR,
        specular);

    GLfloat materialSpecular[] =
        {
            1.0f,
            1.0f,
            1.0f,
            1.0f};

    GLfloat shininess[] =
        {
            64.0f};

    glMaterialfv(
        GL_FRONT_AND_BACK,
        GL_SPECULAR,
        materialSpecular);

    glMaterialfv(
        GL_FRONT_AND_BACK,
        GL_SHININESS,
        shininess);

    glClearColor(
        0.05f,
        0.05f,
        0.05f,
        1.0f);

    glClear(
        GL_COLOR_BUFFER_BIT |
        GL_DEPTH_BUFFER_BIT);
}

void DrawerOpengl::frameEnd()
{
    glFlush();
}

void DrawerOpengl::drawVertices(
    const Point3f *points,
    std::size_t count,
    const Color &color,
    float size)
{
    if (!points ||
        count == 0 ||
        color.a <= 0.0f)
    {
        return;
    }

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
    const Point3f *points,
    std::size_t count,
    const Color &color,
    float width)
{
    if (!points || count == 0)
        return;

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

inline void emitNormal(
    const Point3f &a,
    const Point3f &b,
    const Point3f &c)
{
    auto u = b.to_vector() - a.to_vector();
    auto v = c.to_vector() - a.to_vector();

    auto n =
        u.cross(v).normalized();

    glNormal3fv(
        n.data_ptr());
}

void DrawerOpengl::drawFaces(
    const Point3f *points,
    std::size_t count,
    const Color &color)
{
    if (!points ||
        count == 0)
    {
        return;
    }

    setColor(color);

    glBegin(GL_TRIANGLES);

    const std::size_t totalPoints =
        count * 3;

    for (std::size_t i = 0;
         i < totalPoints;
         i += 3)
    {
        const auto &a =
            points[i + 0];

        const auto &b =
            points[i + 1];

        const auto &c =
            points[i + 2];

        emitNormal(a, b, c);

        emitVertex(a);
        emitVertex(b);
        emitVertex(c);
    }

    glEnd();
}

void DrawerOpengl::drawMeshes(
    const Mesh3f *meshes,
    std::size_t count,
    const Color &faceColor,
    const Color &edgeColor,
    const Color &vertexColor)
{
    if (!meshes || count == 0)
    {
        return;
    }

    std::vector<Point3f> faceVertices;
    std::vector<Point3f> lineVertices;

    for (std::size_t m = 0; m < count; ++m)
    {
        const auto &mesh =
            meshes[m];

        const auto &vertices =
            mesh.vertices();

        const auto &edges =
            mesh.edges();

        const auto &faces =
            mesh.faces();

        if (vertices.empty())
        {
            continue;
        }

        // Faces
        if (faceColor.a > 0.0f &&
            !faces.empty())
        {
            faceVertices.clear();

            faceVertices.reserve(
                faces.size() * 3);

            for (const auto &face : faces)
            {
                faceVertices.push_back(
                    vertices[face.indices[0]]);

                faceVertices.push_back(
                    vertices[face.indices[1]]);

                faceVertices.push_back(
                    vertices[face.indices[2]]);
            }

            drawFaces(
                faceVertices.data(),
                faces.size(),
                faceColor);
        }

        // Arestas
        if (edgeColor.a > 0.0f &&
            !edges.empty())
        {
            lineVertices.clear();

            lineVertices.reserve(
                edges.size() * 2);

            for (const auto &edge : edges)
            {
                lineVertices.push_back(
                    vertices[edge.v1]);

                lineVertices.push_back(
                    vertices[edge.v2]);
            }

            drawLines(
                lineVertices.data(),
                edges.size(),
                edgeColor,
                1.0f);
        }

        // Vértices
        if (vertexColor.a > 0.0f)
        {
            drawVertices(
                vertices.data(),
                vertices.size(),
                vertexColor,
                5.0f);
        }
    }
}