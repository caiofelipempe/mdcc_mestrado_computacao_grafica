#pragma once

#include "color.hpp"
#include "algebric_vector.hpp"
#include "mesh.hpp"

#include <cstddef>

class RendererGlfwOpengl;

class DrawerOpengl
{
    friend class RendererGlfwOpengl;

public:
    void rastFaces(const geometry::Vec3f *points,
                   std::size_t count,
                   const Color &color);

    void rastLines(const geometry::Vec3f *points,
                   std::size_t count,
                   const Color &color,
                   float width);

    void rastVertices(const geometry::Vec3f *points,
                      std::size_t count,
                      const Color &color,
                      float size);

    void rastMeshes(const geometry::Mesh3f *meshes,
                    std::size_t count,
                    const Color &faceColor,
                    const Color &edgeColor,
                    const Color &vertexColor);

private:
    // Lifecycle — só o renderer chama
    void frameBegin();
    void frameEnd();
};