#pragma once

#include "color.hpp"
#include "algebric_vector.hpp"
#include "mesh.hpp"

#include <cstddef>

class Rasterizer
{
public:
    virtual ~Rasterizer() = default;

    virtual void rastFaces   (const geometry::Vec3f* points,
                              std::size_t count,
                              const Color& color) = 0;

    virtual void rastLines   (const geometry::Vec3f* points,
                              std::size_t count,
                              const Color& color,
                              float width) = 0;

    virtual void rastVertices(const geometry::Vec3f* points,
                              std::size_t count,
                              const Color& color,
                              float size) = 0;

    virtual void rastMeshes  (const geometry::Mesh3f* meshes,
                              std::size_t count,
                              const Color& faceColor,
                              const Color& edgeColor,
                              const Color& vertexColor) = 0;
};