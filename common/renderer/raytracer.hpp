#pragma once

#include "color.hpp"
#include "mesh.hpp"

#include <cstddef>

class Raytracer
{
public:
    virtual ~Raytracer() = default;

    virtual void traceFaces (const geometry::Vec3f* points,
                             std::size_t count,
                             const Color& color) = 0;

    virtual void traceMeshes(const geometry::Mesh3f* meshes,
                             std::size_t count,
                             const Color& faceColor) = 0;
};