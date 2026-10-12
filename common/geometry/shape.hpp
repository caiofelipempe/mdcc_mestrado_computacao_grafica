#pragma once

#include "algebric_vector.hpp"

#include <cmath>
#include <numbers>

namespace geometry
{
    class Shape
    {
    public:
        virtual ~Shape() = default;

        [[nodiscard]] virtual float area() const = 0;
        [[nodiscard]] virtual float volume() const = 0;
        [[nodiscard]] virtual Vec3f boundSize() const = 0;
    };

    class Block : public Shape
    {
    public:
        Block(float width = 1.0f, float height = 1.0f, float depth = 1.0f)
            : m_width(width), m_height(height), m_depth(depth) {}

        [[nodiscard]] float area() const override
        {
            return 2.0f * (m_width * m_height + m_width * m_depth + m_height * m_depth);
        }

        [[nodiscard]] float volume() const override { return m_width * m_height * m_depth; }
        [[nodiscard]] Vec3f boundSize() const override { return {m_width, m_height, m_depth}; }

    private:
        float m_width, m_height, m_depth;
    };

    class Cube : public Block
    {
    public:
        explicit Cube(float size = 1.0f) : Block(size, size, size) {}

        [[nodiscard]] float size() const { return boundSize()[0]; }
    };

    class RegularTetrahedron : public Shape
    {
    public:
        explicit RegularTetrahedron(float edgeLength = 1.0f) : m_edgeLength(edgeLength) {}

        [[nodiscard]] float area() const override
        {
            return std::numbers::sqrt3_v<float> * m_edgeLength * m_edgeLength;
        }

        [[nodiscard]] float volume() const override
        {
            return m_edgeLength * m_edgeLength * m_edgeLength / (6.0f * std::numbers::sqrt2_v<float>);
        }

        // Inscrito no cubo de lado edgeLength / sqrt(2).
        [[nodiscard]] Vec3f boundSize() const override
        {
            const float d = m_edgeLength / std::numbers::sqrt2_v<float>;
            return {d, d, d};
        }

    private:
        float m_edgeLength;
    };

    class Sphere : public Shape
    {
    public:
        explicit Sphere(float radius = 1.0f) : m_radius(radius) {}

        float &radius() { return m_radius; }
        const float &radius() const { return m_radius; }

        [[nodiscard]] float area() const override
        {
            return 4.0f * std::numbers::pi_v<float> * m_radius * m_radius;
        }

        [[nodiscard]] float volume() const override
        {
            return 4.0f / 3.0f * std::numbers::pi_v<float> * m_radius * m_radius * m_radius;
        }

        [[nodiscard]] Vec3f boundSize() const override
        {
            const float d = 2.0f * m_radius;
            return {d, d, d};
        }

    private:
        float m_radius;
    };

    class Cylinder : public Shape
    {
    public:
        Cylinder(float radius = 1.0f, float height = 1.0f) : m_radius(radius), m_height(height) {}

        float &radius() { return m_radius; }
        const float &radius() const { return m_radius; }
        float &height() { return m_height; }
        const float &height() const { return m_height; }

        [[nodiscard]] float area() const override
        {
            return 2.0f * std::numbers::pi_v<float> * m_radius * (m_radius + m_height);
        }

        [[nodiscard]] float volume() const override
        {
            return std::numbers::pi_v<float> * m_radius * m_radius * m_height;
        }

        [[nodiscard]] Vec3f boundSize() const override
        {
            return {2.0f * m_radius, m_height, 2.0f * m_radius};
        }

    private:
        float m_radius, m_height;
    };

    class Cone : public Shape
    {
    public:
        Cone(float radius = 1.0f, float height = 1.0f) : m_radius(radius), m_height(height) {}

        float &radius() { return m_radius; }
        const float &radius() const { return m_radius; }
        float &height() { return m_height; }
        const float &height() const { return m_height; }

        [[nodiscard]] float area() const override
        {
            const float slant = std::sqrt(m_radius * m_radius + m_height * m_height);
            return std::numbers::pi_v<float> * m_radius * (slant + m_radius);
        }

        [[nodiscard]] float volume() const override
        {
            return std::numbers::pi_v<float> * m_radius * m_radius * m_height / 3.0f;
        }

        [[nodiscard]] Vec3f boundSize() const override
        {
            return {2.0f * m_radius, m_height, 2.0f * m_radius};
        }

    private:
        float m_radius, m_height;
    };

        class Capsule : public Shape
    {
    public:
        Capsule(float radius = 1.0f, float height = 1.0f) : m_radius(radius), m_height(height) {}

        float &radius() { return m_radius; }
        const float &radius() const { return m_radius; }
        float &height() { return m_height; }
        const float &height() const { return m_height; }

        // Cilindro lateral + uma esfera completa (as duas semiesferas).
        [[nodiscard]] float area() const override
        {
            return 2.0f * std::numbers::pi_v<float> * m_radius * (m_height + 2.0f * m_radius);
        }

        [[nodiscard]] float volume() const override
        {
            return std::numbers::pi_v<float> * m_radius * m_radius *
                   (m_height + 4.0f / 3.0f * m_radius);
        }

        [[nodiscard]] Vec3f boundSize() const override
        {
            return {2.0f * m_radius, m_height + 2.0f * m_radius, 2.0f * m_radius};
        }

    private:
        float m_radius, m_height;
    };

} // namespace geometry