#include "raytrace_scene.hpp"
#include "camera.hpp"

#include <algorithm>
#include <cmath>
#include <thread>

using namespace geometry;

namespace {

constexpr float PI  = 3.14159265358979323846f;
constexpr float EPS = 1e-6f;
constexpr float INF = 1e30f;

// ─────────────────────────────────────────────────────────────────────
//  Helpers com Vec3f do usuário
// ─────────────────────────────────────────────────────────────────────

inline Vec3f vmin(const Vec3f& a, const Vec3f& b)
{
    return Vec3f{ std::min(a[0], b[0]),
                  std::min(a[1], b[1]),
                  std::min(a[2], b[2]) };
}

inline Vec3f vmax(const Vec3f& a, const Vec3f& b)
{
    return Vec3f{ std::max(a[0], b[0]),
                  std::max(a[1], b[1]),
                  std::max(a[2], b[2]) };
}

// Normal de triângulo — usa .cross().normalized() direto do AlgebricVector.
// Retorna false se degenerado (evita propagar NaN pra BVH/shading).
inline bool computeNormal(const Vec3f& a, const Vec3f& b, const Vec3f& c,
                          Vec3f& out)
{
    const Vec3f n = (b - a).cross(c - a);
    if (n.sqrNorm() < 1e-20f)
        return false;
    out = n.normalized();
    return true;
}

inline uint8_t toByte(float v)
{
    if (v <= 0.0f) return 0;
    if (v >= 1.0f) return 255;
    return static_cast<uint8_t>(v * 255.0f);
}

} // namespace

// ─────────────────────────────────────────────────────────────────────
//  Cena
// ─────────────────────────────────────────────────────────────────────
void RaytraceScene::clear()
{
    m_tris.clear();
    m_prim.clear();
    m_nodes.clear();
    m_root = -1;
}

void RaytraceScene::addTriangle(const Vec3f& a, const Vec3f& b, const Vec3f& c,
                                const Color& albedo)
{
    Vec3f n;
    if (!computeNormal(a, b, c, n))
        return;   // triângulo degenerado — ignora

    Triangle t;
    t.a      = a;
    t.b      = b;
    t.c      = c;
    t.normal = n;
    t.albedo = albedo;
    m_tris.push_back(t);
}

// ─────────────────────────────────────────────────────────────────────
//  BVH — split mediano no eixo mais longo
// ─────────────────────────────────────────────────────────────────────
int RaytraceScene::buildRecursive(int start, int end)
{
    const int nodeIdx = static_cast<int>(m_nodes.size());
    m_nodes.push_back(BVHNode{});

    Vec3f bmin{  INF,  INF,  INF };
    Vec3f bmax{ -INF, -INF, -INF };

    for (int i = start; i < end; ++i)
    {
        const Triangle& t = m_tris[m_prim[i]];
        bmin = vmin(bmin, vmin(t.a, vmin(t.b, t.c)));
        bmax = vmax(bmax, vmax(t.a, vmax(t.b, t.c)));
    }

    m_nodes[nodeIdx].bmin = bmin;
    m_nodes[nodeIdx].bmax = bmax;

    const int count = end - start;

    if (count <= 4)
    {
        m_nodes[nodeIdx].start = start;
        m_nodes[nodeIdx].count = count;
        return nodeIdx;
    }

    const Vec3f ext = bmax - bmin;
    int axis = 0;
    if (ext[1] > ext[axis]) axis = 1;
    if (ext[2] > ext[axis]) axis = 2;

    std::sort(m_prim.begin() + start, m_prim.begin() + end,
              [&](int i, int j) {
                  const Triangle& ti = m_tris[i];
                  const Triangle& tj = m_tris[j];
                  const float ci = (ti.a[axis] + ti.b[axis] + ti.c[axis]) / 3.0f;
                  const float cj = (tj.a[axis] + tj.b[axis] + tj.c[axis]) / 3.0f;
                  return ci < cj;
              });

    const int mid = (start + end) / 2;
    m_nodes[nodeIdx].left  = buildRecursive(start, mid);
    m_nodes[nodeIdx].right = buildRecursive(mid,   end);
    return nodeIdx;
}

void RaytraceScene::build()
{
    m_nodes.clear();
    m_prim.resize(m_tris.size());
    for (std::size_t i = 0; i < m_tris.size(); ++i)
        m_prim[i] = static_cast<int>(i);

    if (!m_prim.empty())
        m_root = buildRecursive(0, static_cast<int>(m_prim.size()));
    else
        m_root = -1;
}

// ─────────────────────────────────────────────────────────────────────
//  Interseção triângulo — Möller–Trumbore
// ─────────────────────────────────────────────────────────────────────
bool RaytraceScene::intersectTri(int idx,
                                 const Vec3f& o,
                                 const Vec3f& d,
                                 float& tHit,
                                 Vec3f& nHit) const
{
    const Triangle& tri = m_tris[idx];

    const Vec3f e1 = tri.b - tri.a;
    const Vec3f e2 = tri.c - tri.a;

    const Vec3f p   = d.cross(e2);
    const float det = e1.dot(p);
    if (std::fabs(det) < EPS) return false;

    const float invDet = 1.0f / det;
    const Vec3f tv = o - tri.a;

    const float u = tv.dot(p) * invDet;
    if (u < 0.0f || u > 1.0f) return false;

    const Vec3f q = tv.cross(e1);
    const float v = d.dot(q) * invDet;
    if (v < 0.0f || u + v > 1.0f) return false;

    const float t = e2.dot(q) * invDet;
    if (t < EPS) return false;

    tHit = t;
    nHit = tri.normal;
    return true;
}

// ─────────────────────────────────────────────────────────────────────
//  Traversal BVH
// ─────────────────────────────────────────────────────────────────────
bool RaytraceScene::traverse(int nodeIdx,
                             const Vec3f& o,
                             const Vec3f& d,
                             float tMax,
                             int& outTri,
                             float& outT,
                             Vec3f& outN) const
{
    if (nodeIdx < 0) return false;
    const BVHNode& node = m_nodes[nodeIdx];

    // Slab test
    float t0 = 0.0f;
    float t1 = tMax;
    for (int i = 0; i < 3; ++i)
    {
        const float invD = 1.0f / (d[i] != 0.0f ? d[i] : EPS);
        float ta = (node.bmin[i] - o[i]) * invD;
        float tb = (node.bmax[i] - o[i]) * invD;
        if (ta > tb) std::swap(ta, tb);
        if (ta > t0) t0 = ta;
        if (tb < t1) t1 = tb;
        if (t0 > t1) return false;
    }

    bool hit = false;

    if (node.count > 0)
    {
        for (int i = node.start; i < node.start + node.count; ++i)
        {
            const int ti = m_prim[i];
            float t; Vec3f n;
            if (intersectTri(ti, o, d, t, n) && t < tMax)
            {
                tMax   = t;
                outT   = t;
                outTri = ti;
                outN   = n;
                hit    = true;
            }
        }
    }
    else
    {
        if (traverse(node.left, o, d, tMax, outTri, outT, outN))
        {
            tMax = outT;
            hit  = true;
        }
        if (traverse(node.right, o, d, tMax, outTri, outT, outN))
            hit = true;
    }

    return hit;
}

bool RaytraceScene::occluded(const Vec3f& o, const Vec3f& d, float tMax) const
{
    int tri; float t; Vec3f n;
    return traverse(m_root, o, d, tMax, tri, t, n);
}

// ─────────────────────────────────────────────────────────────────────
//  Shading — ambiente + 1 luz direcional + sombra dura
// ─────────────────────────────────────────────────────────────────────
Color RaytraceScene::shade(const Vec3f& hit,
                           int triIdx,
                           const Vec3f& n) const
{
    const Triangle& tri = m_tris[triIdx];

    Color out{ tri.albedo.r * 0.15f,
               tri.albedo.g * 0.15f,
               tri.albedo.b * 0.15f,
               1.0f };

    const Vec3f lightDir     = Vec3f{ 0.5f, 1.0f, 0.3f }.normalized();
    const Vec3f shadowOrigin = hit + n * 1e-3f;

    if (!occluded(shadowOrigin, lightDir, INF))
    {
        const float ndl = std::max(0.0f, n.dot(lightDir));
        out.r += tri.albedo.r * ndl;
        out.g += tri.albedo.g * ndl;
        out.b += tri.albedo.b * ndl;
    }

    return out;
}

// ─────────────────────────────────────────────────────────────────────
//  Render
// ─────────────────────────────────────────────────────────────────────
void RaytraceScene::renderRange(int y0, int y1)
{
    for (int y = y0; y < y1; ++y)
    {
        for (int x = 0; x < m_w; ++x)
        {
            const float px = (2.0f * (x + 0.5f) / m_w - 1.0f) * m_tanHalfFov * m_aspect;
            const float py = (1.0f - 2.0f * (y + 0.5f) / m_h) * m_tanHalfFov;

            const Vec3f dir = (m_forward + m_right * px + m_up * py).normalized();

            int tri; float t; Vec3f n;
            const std::size_t idx = static_cast<std::size_t>(y) * m_w + x;

            if (traverse(m_root, m_eye, dir, INF, tri, t, n))
            {
                const Color c = shade(m_eye + dir * t, tri, n);
                m_rgb[idx * 3 + 0] = toByte(c.r);
                m_rgb[idx * 3 + 1] = toByte(c.g);
                m_rgb[idx * 3 + 2] = toByte(c.b);

                m_depth[idx] = t * dir.dot(m_forward);
            }
            else
            {
                const float k = std::max(0.0f, std::min(1.0f, dir[1] * 0.5f + 0.5f));
                m_rgb[idx * 3 + 0] = toByte(0.20f + 0.40f * k);
                m_rgb[idx * 3 + 1] = toByte(0.30f + 0.40f * k);
                m_rgb[idx * 3 + 2] = toByte(0.55f + 0.40f * k);
                m_depth[idx] = INF;
            }
        }
    }
}

void RaytraceScene::render(const Camera& camera, int width, int height)
{
    if (m_root < 0 || width <= 0 || height <= 0) return;

    m_w = width;
    m_h = height;
    m_rgb.assign(static_cast<std::size_t>(width) * height * 3, 0);
    m_depth.assign(static_cast<std::size_t>(width) * height, INF);

    m_eye     = camera.position();
    m_forward = (camera.target() - m_eye).normalized();
    m_right   = m_forward.cross(camera.up()).normalized();
    m_up      = m_right.cross(m_forward).normalized();

    m_aspect     = camera.aspect();
    m_tanHalfFov = std::tan(camera.fov() * PI / 180.0f * 0.5f);

    const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
    const int bands   = static_cast<int>(hw);

    if (bands <= 1)
    {
        renderRange(0, m_h);
        return;
    }

    std::vector<std::thread> threads;
    threads.reserve(bands);

    const int step = (m_h + bands - 1) / bands;
    for (int b = 0; b < bands; ++b)
    {
        const int y0 = b * step;
        const int y1 = std::min(m_h, y0 + step);
        if (y0 >= y1) break;
        threads.emplace_back([this, y0, y1] { renderRange(y0, y1); });
    }
    for (auto& th : threads) th.join();
}