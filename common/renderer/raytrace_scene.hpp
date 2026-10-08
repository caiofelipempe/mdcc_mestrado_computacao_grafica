#pragma once

#include "color.hpp"
#include "algebric_vector.hpp"   // geometry::Vec3f, Mesh3f

#include <cstdint>
#include <vector>

class Camera;

class RaytraceScene
{
public:
    void clear();

    void addTriangle(const geometry::Vec3f& a,
                     const geometry::Vec3f& b,
                     const geometry::Vec3f& c,
                     const Color& albedo);

    bool empty() const { return m_tris.empty(); }

    void build();
    void render(const Camera& camera, int w, int h);

    const std::vector<uint8_t>& color() const { return m_rgb; }
    const std::vector<float>&   depth() const { return m_depth; }
    int width()  const { return m_w; }
    int height() const { return m_h; }

private:
    struct Triangle {
        geometry::Vec3f a, b, c;
        geometry::Vec3f normal;
        Color           albedo;
    };

    struct BVHNode {
        geometry::Vec3f bmin, bmax;
        int left  = -1;
        int right = -1;
        int start = 0;
        int count = 0;
    };

    int  buildRecursive(int start, int end);

    bool intersectTri(int idx,
                      const geometry::Vec3f& o,
                      const geometry::Vec3f& d,
                      float& t,
                      geometry::Vec3f& n) const;

    bool traverse(int node,
                  const geometry::Vec3f& o,
                  const geometry::Vec3f& d,
                  float tMax,
                  int& outTri,
                  float& outT,
                  geometry::Vec3f& outN) const;

    bool occluded(const geometry::Vec3f& o,
                  const geometry::Vec3f& d,
                  float tMax) const;

    Color shade(const geometry::Vec3f& hit,
                int triIdx,
                const geometry::Vec3f& n) const;

    void renderRange(int y0, int y1);

    std::vector<Triangle>        m_tris;
    std::vector<int>             m_prim;
    std::vector<BVHNode>         m_nodes;
    int                          m_root = -1;

    std::vector<uint8_t>         m_rgb;
    std::vector<float>           m_depth;
    int                          m_w = 0, m_h = 0;

    geometry::Vec3f              m_eye, m_forward, m_right, m_up;
    float                        m_tanHalfFov = 1.0f;
    float                        m_aspect     = 1.0f;
};