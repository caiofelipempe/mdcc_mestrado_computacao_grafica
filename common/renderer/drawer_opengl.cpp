#include "drawer_opengl.hpp"
#include "color.hpp"

#include <GL/glew.h>

#include <vector>

using namespace geometry;

namespace
{
    inline void setColor(const Color &color)
    {
        glColor4f(color.r, color.g, color.b, color.a);
    }

    inline Vec3f computeNormal(
        const Vec3f &a,
        const Vec3f &b,
        const Vec3f &c)
    {
        auto u = b - a;
        auto v = c - a;
        auto n = u.cross(v).normalized();
        return Vec3f({n[0], n[1], n[2]});
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Frame
// ─────────────────────────────────────────────────────────────────────────────
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

    GLfloat lightPosition[] = {20.0f, 20.0f, 20.0f, 1.0f};
    GLfloat ambient[]       = { 0.20f,  0.20f,  0.20f, 1.0f};
    GLfloat diffuse[]       = { 1.0f,   1.0f,   1.0f,  1.0f};
    GLfloat specular[]      = { 1.0f,   1.0f,   1.0f,  1.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    GLfloat materialSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat shininess[]        = {64.0f};

    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  materialSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void DrawerOpengl::frameEnd()
{
    glFlush();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Desenho imediato
// ─────────────────────────────────────────────────────────────────────────────
void DrawerOpengl::drawVertices(
    const Vec3f *points,
    std::size_t count,
    const Color &color,
    float size)
{
    if (!points || count == 0 || color.a <= 0.0f)
        return;

    glDisable(GL_LIGHTING);
    glPointSize(size);
    setColor(color);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Vec3f), points);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(count));
    glDisableClientState(GL_VERTEX_ARRAY);

    glEnable(GL_LIGHTING);
}

void DrawerOpengl::drawLines(
    const Vec3f *points,
    std::size_t count,
    const Color &color,
    float width)
{
    if (!points || count == 0 || color.a <= 0.0f)
        return;

    glDisable(GL_LIGHTING);
    glLineWidth(width);
    setColor(color);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Vec3f), points);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(count * 2));
    glDisableClientState(GL_VERTEX_ARRAY);

    glEnable(GL_LIGHTING);
}

void DrawerOpengl::drawFaces(
    const Vec3f *points,
    std::size_t count,
    const Color &color)
{
    if (!points || count == 0 || color.a <= 0.0f)
        return;

    // Como as normais são por triângulo (flat), precisamos montar um buffer
    // temporário contendo posição + normal intercaladas — não dá pra usar o
    // array de entrada direto por causa do NORMAL_ARRAY.
    std::vector<FaceVertex> verts;
    verts.reserve(count * 3);

    const std::size_t totalPoints = count * 3;

    for (std::size_t i = 0; i < totalPoints; i += 3)
    {
        const auto &a = points[i + 0];
        const auto &b = points[i + 1];
        const auto &c = points[i + 2];

        const Vec3f norm = computeNormal(a, b, c);

        verts.push_back({norm, a});
        verts.push_back({norm, b});
        verts.push_back({norm, c});
    }

    setColor(color);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glNormalPointer(
        GL_FLOAT,
        sizeof(FaceVertex),
        verts[0].normal.data_ptr());

    glVertexPointer(
        3,
        GL_FLOAT,
        sizeof(FaceVertex),
        verts[0].position.data_ptr());

    glDrawArrays(
        GL_TRIANGLES,
        0,
        static_cast<GLsizei>(verts.size()));

    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Meshes — reaproveita os três métodos acima
// ─────────────────────────────────────────────────────────────────────────────
void DrawerOpengl::drawMeshes(
    const Mesh3f *meshes,
    std::size_t count,
    const Color &faceColor,
    const Color &edgeColor,
    const Color &vertexColor)
{
    if (!meshes || count == 0)
        return;

    for (std::size_t m = 0; m < count; ++m)
    {
        const auto &mesh = meshes[m];

        const auto &vertices = mesh.vertices();
        const auto &edges    = mesh.edges();
        const auto &faces    = mesh.faces();

        if (vertices.empty())
            continue;

        // ── Faces ────────────────────────────────────────────────────────
        if (faceColor.a > 0.0f && !faces.empty())
        {
            std::vector<Vec3f> tris;
            tris.reserve(faces.size() * 3);

            for (const auto &face : faces)
            {
                tris.push_back(vertices[face.indices[0]]);
                tris.push_back(vertices[face.indices[1]]);
                tris.push_back(vertices[face.indices[2]]);
            }

            drawFaces(tris.data(), faces.size(), faceColor);
        }

        // ── Arestas ──────────────────────────────────────────────────────
        if (edgeColor.a > 0.0f && !edges.empty())
        {
            std::vector<Vec3f> lines;
            lines.reserve(edges.size() * 2);

            for (const auto &edge : edges)
            {
                lines.push_back(vertices[edge.v1]);
                lines.push_back(vertices[edge.v2]);
            }

            drawLines(lines.data(), edges.size(), edgeColor, 1.0f);
        }

        // ── Vértices ─────────────────────────────────────────────────────
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