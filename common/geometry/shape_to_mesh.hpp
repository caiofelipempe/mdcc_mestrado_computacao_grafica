#pragma once

#include "mesh.hpp"
#include "shape.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <utility>
#include <vector>

namespace geometry
{
    inline constexpr int DefaultSegments = 32;

    namespace detail
    {
        // Perfil (raio, y) de baixo para cima. O primeiro e o último ponto são
        // os polos: só o y deles é usado (raio tratado como zero).
        using Profile = std::vector<std::pair<float, float>>;

        // Sólido de revolução em torno do eixo Y.
        inline Mesh3f revolve(const Profile &profile, int segments)
        {
            Mesh3f mesh;

            const auto bottom = mesh.addVertex({0.0f, profile.front().second, 0.0f});

            std::vector<std::vector<std::size_t>> rings;
            for (std::size_t i = 1; i + 1 < profile.size(); ++i)
            {
                auto &ring = rings.emplace_back();
                for (int s = 0; s < segments; ++s)
                {
                    const float a = 2.0f * std::numbers::pi_v<float> * static_cast<float>(s) /
                                    static_cast<float>(segments);
                    ring.push_back(mesh.addVertex({profile[i].first * std::cos(a),
                                                   profile[i].second,
                                                   profile[i].first * std::sin(a)}));
                }
            }

            const auto top = mesh.addVertex({0.0f, profile.back().second, 0.0f});

            for (std::size_t s = 0; s < static_cast<std::size_t>(segments); ++s)
            {
                const std::size_t n = (s + 1) % static_cast<std::size_t>(segments);

                mesh.addFace(bottom, rings.front()[s], rings.front()[n]);
                mesh.addFace(top, rings.back()[n], rings.back()[s]);

                for (std::size_t r = 0; r + 1 < rings.size(); ++r)
                {
                    mesh.addFace(rings[r][s], rings[r + 1][s], rings[r + 1][n]);
                    mesh.addFace(rings[r][s], rings[r + 1][n], rings[r][n]);
                }
            }

            mesh.buildEdgesFromFaces();
            return mesh;
        }
    } // namespace detail

    // Block (e Cube, que herda de Block): centrado na origem.
    [[nodiscard]] inline Mesh3f toMesh(const Block &block)
    {
        Mesh3f mesh;
        const Vec3f h = block.boundSize() * 0.5f;

        for (const float z : {-h[2], h[2]})
            for (const auto [x, y] : {std::pair{-h[0], -h[1]}, {h[0], -h[1]}, {h[0], h[1]}, {-h[0], h[1]}})
                (void)mesh.addVertex({x, y, z});

        static constexpr std::array<std::array<std::size_t, 3>, 12> faces{{
            {0, 3, 2}, {0, 2, 1}, {4, 5, 6}, {4, 6, 7}, {0, 1, 5}, {0, 5, 4},
            {3, 7, 6}, {3, 6, 2}, {1, 2, 6}, {1, 6, 5}, {0, 4, 7}, {0, 7, 3},
        }};
        for (const auto &[a, b, c] : faces)
            mesh.addFace(a, b, c);

        // Só as 12 arestas do cubo, sem as diagonais da triangulação.
        for (std::size_t i = 0; i < 4; ++i)
        {
            mesh.addEdge(i, (i + 1) % 4);
            mesh.addEdge(i + 4, (i + 1) % 4 + 4);
            mesh.addEdge(i, i + 4);
        }

        return mesh;
    }

    [[nodiscard]] inline Mesh3f toMesh(const RegularTetrahedron &tetra)
    {
        Mesh3f mesh;
        const float s = tetra.boundSize()[0] * 0.5f;

        (void)mesh.addVertex({s, s, s});
        (void)mesh.addVertex({-s, -s, s});
        (void)mesh.addVertex({-s, s, -s});
        (void)mesh.addVertex({s, -s, -s});

        mesh.addFace(0, 1, 2);
        mesh.addFace(0, 3, 1);
        mesh.addFace(0, 2, 3);
        mesh.addFace(1, 3, 2);

        mesh.buildEdgesFromFaces();
        return mesh;
    }

    [[nodiscard]] inline Mesh3f toMesh(const Sphere &sphere, int segments = DefaultSegments)
    {
        const float r = sphere.radius();
        const int rings = std::max(2, segments / 2);

        detail::Profile profile{{0.0f, -r}};
        for (int i = 1; i < rings; ++i)
        {
            const float phi = std::numbers::pi_v<float> * (1.0f - static_cast<float>(i) / static_cast<float>(rings));
            profile.push_back({r * std::sin(phi), r * std::cos(phi)});
        }
        profile.push_back({0.0f, r});

        return detail::revolve(profile, segments);
    }

    [[nodiscard]] inline Mesh3f toMesh(const Cylinder &cylinder, int segments = DefaultSegments)
    {
        const float r = cylinder.radius();
        const float h = cylinder.height() * 0.5f;

        return detail::revolve({{0.0f, -h}, {r, -h}, {r, h}, {0.0f, h}}, segments);
    }

    [[nodiscard]] inline Mesh3f toMesh(const Cone &cone, int segments = DefaultSegments)
    {
        const float r = cone.radius();
        const float h = cone.height() * 0.5f;

        return detail::revolve({{0.0f, -h}, {r, -h}, {0.0f, h}}, segments);
    }

    // Duas semiesferas ligadas por um trecho cilíndrico de altura height().
    [[nodiscard]] inline Mesh3f toMesh(const Capsule &capsule, int segments = DefaultSegments)
    {
        const float r = capsule.radius();
        const float h = capsule.height() * 0.5f;
        const int hemi = std::max(1, segments / 4);
        constexpr float halfPi = std::numbers::pi_v<float> * 0.5f;

        detail::Profile profile;

        // Semiesfera inferior: do polo (phi = pi) ao equador (phi = pi/2).
        for (int i = 0; i <= hemi; ++i)
        {
            const float phi = std::numbers::pi_v<float> - halfPi * static_cast<float>(i) / static_cast<float>(hemi);
            profile.push_back({r * std::sin(phi), -h + r * std::cos(phi)});
        }

        // Semiesfera superior: do equador (phi = pi/2) ao polo (phi = 0).
        for (int i = 0; i <= hemi; ++i)
        {
            const float phi = halfPi * (1.0f - static_cast<float>(i) / static_cast<float>(hemi));
            profile.push_back({r * std::sin(phi), h + r * std::cos(phi)});
        }

        return detail::revolve(profile, segments);
    }

} // namespace geometry