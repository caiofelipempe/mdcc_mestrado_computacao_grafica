#include "drawer_opengl.hpp"

#include <GL/glew.h>

#include <algorithm>
#include <vector>

using namespace geometry;

namespace {

inline void setColor(const Color& c)
{
    glColor4f(c.r, c.g, c.b, c.a);
}

inline bool computeNormal(const Vec3f& a, const Vec3f& b, const Vec3f& c,
                          Vec3f& out)
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

} // namespace

// ─────────────────────────────────────────────────────────────────────
//  Raster
// ─────────────────────────────────────────────────────────────────────
void DrawerOpengl::rastVertices(const Vec3f* points, std::size_t count,
                                const Color& color, float size)
{
    if (!points || count == 0 || color.a <= 0.0f) return;

    glDisable(GL_LIGHTING);
    glPointSize(size);
    setColor(color);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Vec3f), points->data_ptr());
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(count));
    glDisableClientState(GL_VERTEX_ARRAY);

    glEnable(GL_LIGHTING);
}

void DrawerOpengl::rastLines(const Vec3f* points, std::size_t count,
                             const Color& color, float width)
{
    if (!points || count == 0 || color.a <= 0.0f) return;

    glDisable(GL_LIGHTING);
    glLineWidth(width);
    setColor(color);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Vec3f), points->data_ptr());
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(count * 2));
    glDisableClientState(GL_VERTEX_ARRAY);

    glEnable(GL_LIGHTING);
}

void DrawerOpengl::rastFaces(const Vec3f* points, std::size_t count,
                             const Color& color)
{
    if (!points || count == 0 || color.a <= 0.0f) return;

    std::vector<FaceVertex> verts;
    verts.reserve(count * 3);

    const std::size_t total = count * 3;
    for (std::size_t i = 0; i < total; i += 3)
    {
        const Vec3f& a = points[i + 0];
        const Vec3f& b = points[i + 1];
        const Vec3f& c = points[i + 2];

        Vec3f n;
        if (!computeNormal(a, b, c, n))
            continue;

        verts.push_back({n, a});
        verts.push_back({n, b});
        verts.push_back({n, c});
    }

    if (verts.empty()) return;

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

void DrawerOpengl::rastMeshes(const Mesh3f* meshes, std::size_t count,
                              const Color& faceColor,
                              const Color& edgeColor,
                              const Color& vertexColor)
{
    if (!meshes || count == 0) return;

    for (std::size_t m = 0; m < count; ++m)
    {
        const auto& V = meshes[m].vertices();
        const auto& E = meshes[m].edges();
        const auto& F = meshes[m].faces();

        if (V.empty()) continue;

        // ── Faces ────────────────────────────────────────────────────
        if (faceColor.a > 0.0f && !F.empty())
        {
            std::vector<Vec3f> tris;
            tris.reserve(F.size() * 3);

            for (const auto& f : F)
            {
                tris.push_back(V[f.indices[0]]);
                tris.push_back(V[f.indices[1]]);
                tris.push_back(V[f.indices[2]]);
            }
            rastFaces(tris.data(), F.size(), faceColor);
        }

        // ── Arestas ──────────────────────────────────────────────────
        if (edgeColor.a > 0.0f && !E.empty())
        {
            std::vector<Vec3f> lines;
            lines.reserve(E.size() * 2);

            for (const auto& e : E)
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
//  Raytracer — alimenta a cena
// ─────────────────────────────────────────────────────────────────────
void DrawerOpengl::traceFaces(const Vec3f* points, std::size_t count,
                              const Color& color)
{
    if (!points || count == 0 || color.a <= 0.0f) return;

    const std::size_t total = count * 3;
    for (std::size_t i = 0; i < total; i += 3)
        m_scene.addTriangle(points[i + 0], points[i + 1], points[i + 2], color);
}

void DrawerOpengl::traceMeshes(const Mesh3f* meshes, std::size_t count,
                               const Color& faceColor)
{
    if (!meshes || count == 0 || faceColor.a <= 0.0f) return;

    for (std::size_t m = 0; m < count; ++m)
    {
        const auto& V = meshes[m].vertices();
        const auto& F = meshes[m].faces();
        for (const auto& f : F)
            m_scene.addTriangle(V[f.indices[0]], V[f.indices[1]],
                                V[f.indices[2]], faceColor);
    }
}

// ─────────────────────────────────────────────────────────────────────
//  Construtor / Destrutor
// ─────────────────────────────────────────────────────────────────────
DrawerOpengl::DrawerOpengl() = default;

DrawerOpengl::~DrawerOpengl()
{
    if (m_tex)
    {
        glDeleteTextures(1, &m_tex);
        m_tex = 0;
    }
}

// ─────────────────────────────────────────────────────────────────────
//  Lifecycle — só o renderer chama (via friend)
// ─────────────────────────────────────────────────────────────────────
void DrawerOpengl::frameBegin()
{
    m_scene.clear();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);

    GLfloat lightPos[]  = {20.0f, 20.0f, 20.0f, 1.0f};
    GLfloat lightAmb[]  = { 0.20f, 0.20f, 0.20f, 1.0f};
    GLfloat lightDif[]  = { 1.0f,  1.0f,  1.0f,  1.0f};
    GLfloat lightSpec[] = { 1.0f,  1.0f,  1.0f,  1.0f};
    GLfloat matSpec[]   = { 1.0f,  1.0f,  1.0f,  1.0f};
    GLfloat shininess[] = {64.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  lightDif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  matSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void DrawerOpengl::frameEnd(const FrameContext& ctx)
{
    glFlush();

    if (!m_rtEnabled || m_scene.empty() || !m_camera)
        return;

    if (ctx.width <= 0 || ctx.height <= 0)
        return;

    composeAndBlit(ctx);
}

// ─────────────────────────────────────────────────────────────────────
//  Composição por profundidade
// ─────────────────────────────────────────────────────────────────────
void DrawerOpengl::composeAndBlit(const FrameContext& ctx)
{
    const int w = ctx.width;
    const int h = ctx.height;

    m_rasterDepth.resize(static_cast<std::size_t>(w) * h);
    glReadPixels(0, 0, w, h, GL_DEPTH_COMPONENT, GL_FLOAT, m_rasterDepth.data());

    m_scene.build();
    m_scene.render(*m_camera, w, h);

    const auto& rtRGB   = m_scene.color();
    const auto& rtDepth = m_scene.depth();

    m_compositeRGBA.assign(static_cast<std::size_t>(w) * h * 4, 0);

    const float n = ctx.nearPlane;
    const float f = ctx.farPlane;

    for (int y = 0; y < h; ++y)
    {
        const int rtY = h - 1 - y;

        for (int x = 0; x < w; ++x)
        {
            const std::size_t fbIdx = static_cast<std::size_t>(y) * w + x;
            const std::size_t rtIdx = static_cast<std::size_t>(rtY) * w + x;
            const std::size_t dst   = fbIdx * 4;

            const float z = m_rasterDepth[fbIdx];
            const float rasterDist = (z >= 1.0f)
                ? 1e30f
                : (n * f) / (f - z * (f - n));

            const float td = rtDepth[rtIdx];

            if (td < rasterDist && td < 1e29f)
            {
                m_compositeRGBA[dst + 0] = rtRGB[rtIdx * 3 + 0];
                m_compositeRGBA[dst + 1] = rtRGB[rtIdx * 3 + 1];
                m_compositeRGBA[dst + 2] = rtRGB[rtIdx * 3 + 2];
                m_compositeRGBA[dst + 3] = 255;
            }
        }
    }

    if (m_tex == 0)
    {
        glGenTextures(1, &m_tex);
        glBindTexture(GL_TEXTURE_2D, m_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, m_tex);
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, m_compositeRGBA.data());

    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 1.0f);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}