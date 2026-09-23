#include "drawer_opengl.hpp"
#include "color.hpp"

#include <GL/glew.h>

using namespace geometry;

namespace
{
    inline void setColor(const Color &color)
    {
        glColor4f(color.r, color.g, color.b, color.a);
    }

    inline Point3f computeNormal(
        const Point3f &a,
        const Point3f &b,
        const Point3f &c)
    {
        auto u = b.to_vector() - a.to_vector();
        auto v = c.to_vector() - a.to_vector();
        auto n = u.cross(v).normalized();
        return Point3f({n[0], n[1], n[2]});
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

    m_pendingVertices.clear();
    m_pendingLines.clear();
    m_pendingFaces.clear();
}

void DrawerOpengl::frameEnd()
{
    glEnableClientState(GL_VERTEX_ARRAY);

    // Renderiza todas as faces acumuladas com suporte a normais
    if (!m_pendingFaces.empty())
    {
        glEnableClientState(GL_NORMAL_ARRAY);

        for (const auto &batch : m_pendingFaces)
        {
            setColor(batch.color);

            glNormalPointer(
                GL_FLOAT,
                sizeof(FaceVertex),
                batch.vertices[0].normal.data_ptr());

            glVertexPointer(
                3,
                GL_FLOAT,
                sizeof(FaceVertex),
                batch.vertices[0].position.data_ptr());

            glDrawArrays(
                GL_TRIANGLES,
                0,
                static_cast<GLsizei>(batch.vertices.size()));
        }

        glDisableClientState(GL_NORMAL_ARRAY);
    }

    // Desativa iluminação para renderizar linhas e pontos com cores sólidas
    glDisable(GL_LIGHTING);

    // Renderiza todas as linhas acumuladas
    for (const auto &batch : m_pendingLines)
    {
        glLineWidth(batch.width);
        setColor(batch.color);

        glVertexPointer(
            3,
            GL_FLOAT,
            sizeof(Point3f),
            batch.points[0].data_ptr());

        glDrawArrays(
            GL_LINES,
            0,
            static_cast<GLsizei>(batch.points.size()));
    }

    // Renderiza todos os vértices acumulados
    for (const auto &batch : m_pendingVertices)
    {
        glPointSize(batch.size);
        setColor(batch.color);

        glVertexPointer(
            3,
            GL_FLOAT,
            sizeof(Point3f),
            batch.points[0].data_ptr());

        glDrawArrays(
            GL_POINTS,
            0,
            static_cast<GLsizei>(batch.points.size()));
    }

    glDisableClientState(GL_VERTEX_ARRAY);

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

    BatchVertices batch;
    batch.color = color;
    batch.size = size;
    batch.points.assign(points, points + count);

    m_pendingVertices.push_back(std::move(batch));
}

void DrawerOpengl::drawLines(
    const Point3f *points,
    std::size_t count,
    const Color &color,
    float width)
{
    if (!points ||
        count == 0 ||
        color.a <= 0.0f)
    {
        return;
    }

    BatchLines batch;
    batch.color = color;
    batch.width = width;
    batch.points.assign(points, points + (count * 2));

    m_pendingLines.push_back(std::move(batch));
}

void DrawerOpengl::drawFaces(
    const Point3f *points,
    std::size_t count,
    const Color &color)
{
    if (!points ||
        count == 0 ||
        color.a <= 0.0f)
    {
        return;
    }

    BatchFaces batch;
    batch.color = color;
    batch.vertices.reserve(count * 3);

    const std::size_t totalPoints = count * 3;

    for (std::size_t i = 0;
         i < totalPoints;
         i += 3)
    {
        const auto &a = points[i + 0];
        const auto &b = points[i + 1];
        const auto &c = points[i + 2];

        Point3f norm = computeNormal(a, b, c);

        batch.vertices.push_back({norm, a});
        batch.vertices.push_back({norm, b});
        batch.vertices.push_back({norm, c});
    }

    m_pendingFaces.push_back(std::move(batch));
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
            BatchFaces batch;
            batch.color = faceColor;
            batch.vertices.reserve(faces.size() * 3);

            for (const auto &face : faces)
            {
                const auto &a = vertices[face.indices[0]];
                const auto &b = vertices[face.indices[1]];
                const auto &c = vertices[face.indices[2]];

                Point3f norm = computeNormal(a, b, c);

                batch.vertices.push_back({norm, a});
                batch.vertices.push_back({norm, b});
                batch.vertices.push_back({norm, c});
            }

            m_pendingFaces.push_back(std::move(batch));
        }

        // Arestas
        if (edgeColor.a > 0.0f &&
            !edges.empty())
        {
            BatchLines batch;
            batch.color = edgeColor;
            batch.width = 1.0f;
            batch.points.reserve(edges.size() * 2);

            for (const auto &edge : edges)
            {
                batch.points.push_back(vertices[edge.v1]);
                batch.points.push_back(vertices[edge.v2]);
            }

            m_pendingLines.push_back(std::move(batch));
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