#include "drawer_opengl.hpp"

#include <GL/glew.h>

#include <vector>

using namespace geometry;

namespace
{

    inline void setColor(const Color &c)
    {
        glColor4f(c.r, c.g, c.b, c.a);
    }

    inline bool computeNormal(const Vec3f &a, const Vec3f &b, const Vec3f &c,
                              Vec3f &out)
    {
        const Vec3f n = (b - a).cross(c - a);
        if (n.sqrNorm() < 1e-20f)
            return false;
        out = n.normalized();
        return true;
    }

    struct FaceVertex
    {
        Vec3f normal;
        Vec3f position;
    };

    // Desenha triângulos já com normal por vértice (flat por face)
    void drawTriangles(const std::vector<FaceVertex> &verts, const Color &color)
    {
        if (verts.empty())
            return;

        setColor(color);

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);

        glNormalPointer(GL_FLOAT, sizeof(FaceVertex),
                        verts[0].normal.data_ptr());

        glVertexPointer(3, GL_FLOAT, sizeof(FaceVertex),
                        verts[0].position.data_ptr());

        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size()));

        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    }

} // namespace

// ─────────────────────────────────────────────────────────────────────
//  Raster
// ─────────────────────────────────────────────────────────────────────
void DrawerOpengl::rastVertices(const Vec3f *points, std::size_t count,
                                const Color &color, float size)
{
    if (!points || count == 0 || color.a <= 0.0f)
        return;

    glDisable(GL_LIGHTING);
    glPointSize(size);
    setColor(color);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Vec3f), points->data_ptr());
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(count));
    glDisableClientState(GL_VERTEX_ARRAY);

    glEnable(GL_LIGHTING);
}

void DrawerOpengl::rastLines(const Vec3f *points, std::size_t count,
                             const Color &color, float width)
{
    if (!points || count == 0 || color.a <= 0.0f)
        return;

    glDisable(GL_LIGHTING);
    glLineWidth(width);
    setColor(color);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Vec3f), points->data_ptr());
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(count * 2));
    glDisableClientState(GL_VERTEX_ARRAY);

    glEnable(GL_LIGHTING);
}

void DrawerOpengl::rastFaces(const Vec3f *points, std::size_t count,
                             const Color &color)
{
    if (!points || count == 0 || color.a <= 0.0f)
        return;

    std::vector<FaceVertex> verts;
    verts.reserve(count * 3);

    const std::size_t total = count * 3;
    for (std::size_t i = 0; i < total; i += 3)
    {
        const Vec3f &a = points[i + 0];
        const Vec3f &b = points[i + 1];
        const Vec3f &c = points[i + 2];

        Vec3f n;
        if (!computeNormal(a, b, c, n))
            continue;

        verts.push_back({n, a});
        verts.push_back({n, b});
        verts.push_back({n, c});
    }

    drawTriangles(verts, color);
}

void DrawerOpengl::rastMeshes(const Mesh3f *meshes, std::size_t count,
                              const Color &faceColor,
                              const Color &edgeColor,
                              const Color &vertexColor)
{
    if (!meshes || count == 0)
        return;

    for (std::size_t m = 0; m < count; ++m)
    {
        const auto &V = meshes[m].vertices();
        const auto &E = meshes[m].edges();
        const auto &F = meshes[m].faces();

        if (V.empty())
            continue;

        // ── Faces ────────────────────────────────────────────────────
        if (faceColor.a > 0.0f && !F.empty())
        {
            std::vector<FaceVertex> verts;
            verts.reserve(F.size() * 3);

            for (const auto &f : F)
            {
                const Vec3f &a = V[f.indices[0]];
                const Vec3f &b = V[f.indices[1]];
                const Vec3f &c = V[f.indices[2]];

                // Usa a lightNormal da face; se for zero, calcula pelo winding
                Vec3f n;
                if (f.lightNormal.sqrNorm() > 1e-20f)
                    n = f.lightNormal.normalized();
                else if (!computeNormal(a, b, c, n))
                    continue;

                verts.push_back({n, a});
                verts.push_back({n, b});
                verts.push_back({n, c});
            }

            drawTriangles(verts, faceColor);
        }

        // ── Arestas ──────────────────────────────────────────────────
        if (edgeColor.a > 0.0f && !E.empty())
        {
            std::vector<Vec3f> lines;
            lines.reserve(E.size() * 2);

            for (const auto &e : E)
            {
                lines.push_back(V[e.v1]);
                lines.push_back(V[e.v2]);
            }
            rastLines(lines.data(), E.size(), edgeColor, 1.0f);
        }

        // ── Vértices ─────────────────────────────────────────────────
        if (vertexColor.a > 0.0f)
        {
            rastVertices(V.data(), V.size(), vertexColor, 5.0f);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────
//  Lifecycle — só o renderer chama (via friend)
// ─────────────────────────────────────────────────────────────────────
void DrawerOpengl::frameBegin()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);

    GLfloat lightPos[] = {20.0f, 20.0f, 20.0f, 1.0f};
    GLfloat lightAmb[] = {0.20f, 0.20f, 0.20f, 1.0f};
    GLfloat lightDif[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat lightSpec[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat matSpec[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat shininess[] = {64.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void DrawerOpengl::frameEnd()
{
    glFlush();
}