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

    struct FrameContext
    {
        int width = 0;
        int height = 0;
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
    };

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

    // ── Config exposta ao usuário ────────────────────────────────────
    bool raytraceEnabled() const { return m_rtEnabled; }
    void setRaytraceEnabled(bool v) { m_rtEnabled = v; }

private:
    // ── Lifecycle — só o renderer ────────────────────────────────────
    void frameBegin();
    void frameEnd(const FrameContext &ctx);
    void setCamera(const Camera *camera) { m_camera = camera; }

    void composeAndBlit(const FrameContext &ctx);

    RaytraceScene m_scene;
    bool m_rtEnabled = true;
    const Camera *m_camera = nullptr;

    std::vector<float> m_rasterDepth;
    std::vector<unsigned char> m_compositeRGBA;
    unsigned int m_tex = 0;
};