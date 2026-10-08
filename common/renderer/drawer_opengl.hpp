#pragma once

#include "rasterizer.hpp"
#include "raytracer.hpp"
#include "raytrace_scene.hpp"

#include <cstdint>
#include <vector>

class Camera;
class RendererGlfwOpengl;

class DrawerOpengl : public Rasterizer, public Raytracer
{
    friend class RendererGlfwOpengl;

public:
    DrawerOpengl();
    ~DrawerOpengl() override;

    // ── Rasterizer ───────────────────────────────────────────────────
    void rastFaces(const geometry::Vec3f *, std::size_t, const Color &) override;
    void rastLines(const geometry::Vec3f *, std::size_t, const Color &, float) override;
    void rastVertices(const geometry::Vec3f *, std::size_t, const Color &, float) override;
    void rastMeshes(const geometry::Mesh3f *, std::size_t,
                    const Color &, const Color &, const Color &) override;

    // ── Raytracer ────────────────────────────────────────────────────
    void traceFaces(const geometry::Vec3f *, std::size_t, const Color &) override;
    void traceMeshes(const geometry::Mesh3f *, std::size_t, const Color &) override;
    void setRaytraceDivisor(int divisor) {
        m_rtDivisor = (divisor < 1) ? 1 : divisor;
    }
    int raytraceDivisor() const { return m_rtDivisor; }

    // ── Config exposta ao usuário ────────────────────────────────────
    bool raytraceEnabled() const { return m_rtEnabled; }
    void setRaytraceEnabled(bool v) { m_rtEnabled = v; }

private:
    // ── Lifecycle — só o renderer ────────────────────────────────────
    void frameBegin();
    void frameEnd();
    void setCamera(const Camera *camera) { m_camera = camera; }

    void composeAndBlit();

    RaytraceScene m_scene;
    bool m_rtEnabled = true;
    const Camera *m_camera = nullptr;
    int m_rtDivisor = 2;

    std::vector<float> m_rasterDepth;
    std::vector<unsigned char> m_compositeRGBA;
    unsigned int m_tex = 0;
};