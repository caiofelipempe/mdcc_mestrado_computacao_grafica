#pragma once

#include <random>

#include "vector.hpp"
#include "result.hpp"

using Point2f = geometry::Vector<float, 2>;
using Segment2f = std::array<Point2f, 2>;
using Polygon2f = std::vector<Point2f>;
using Triangle2f =std::array<Point2f, 3>;
using ColorRGB = std::array<float, 3>;

namespace utils {

enum class Error {
    Unknown,
    SizeMismatch,
    DivisionByZero,
    ZeroNorm
};

template <typename T>
static constexpr T defaultEpsilon() noexcept {
    if constexpr (std::is_floating_point_v<T>)
        return static_cast<T>(1e-8);
    else
        return T{};
}

bool isZero(float v) {
    auto eps = defaultEpsilon<float>();
    return std::abs(v) <= eps;
}

Polygon2f generateRandomPolygon(size_t numPoints) {
    Polygon2f polygon;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0, 2 * M_PI);
    std::uniform_real_distribution<float> radiusDist(0.1, 1.0);
    
    // Método 1: Pontos em círculo com raios aleatórios (garante polígono simples)
    std::vector<std::pair<float, float>> polarPoints;
    std::vector<float> angles(numPoints);
    
    // Gerar ângulos aleatórios e ordená-los
    for (size_t i = 0; i < numPoints; ++i) {
        angles[i] = angleDist(gen);
    }
    std::sort(angles.begin(), angles.end());
    
    // Gerar raios aleatórios para cada ângulo
    for (size_t i = 0; i < numPoints; ++i) {
        float radius = radiusDist(gen);
        float x = radius * std::cos(angles[i]);
        float y = radius * std::sin(angles[i]);
        polygon.push_back({x, y});
    }
    
    return polygon;
}

template <typename T, std::size_t N>
Point2f generateRandomPoint(T intervalA, T intervalB) {
    T start = intervalA;
    T end = intervalB;
    if (intervalA > intervalB) {
        start = intervalB;
        end = intervalA;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<T> dist(start, end);

    Point2f rp;
    for(int i = 0; i < N; i++) {
        rp[i] = (T)dist(gen);
    }

    return rp;
}

Result<float, Error, true>
pseudoangleOctante(const Point2f& v) {
    auto eps = defaultEpsilon<float>();

    const float x = v[0];
    const float y = v[1];

    if (std::abs(x) <= eps && std::abs(y) <= eps)
        return result::err(Error::SizeMismatch);

    const float ax = std::abs(x);
    const float ay = std::abs(y);

    // ax >= ay: octantes 1, 8 (x > 0) ou 4, 5 (x < 0)
    if (ax >= ay) {
        const float t = ay / ax;           // t ∈ [0, 1]
        if (x >= 0.0f)
            return (y >=0.0f) ? t              // oct 1: [0, 1)
                              : 8.0f - t;      // oct 8: (7, 8]
        else
            return (y >= 0.0f) ? 4.f - t       // oct 4: (3, 4]
                              : 4.f + t;      // oct 5: [4, 5)
    }
    // ay > ax: octantes 2, 3 (y > 0) ou 6, 7 (y < 0)
    else {
        const float t = ax / ay;           // t ∈ [0, 1)
        if (y >= 0.f)
            return (x >= 0.f) ? 2.f - t       // oct 2: (1, 2]
                              : 2.f + t;      // oct 3: [2, 3)
        else
            return (x >= 0.f) ? 2.f + t       // oct 7: [6, 7)
                              : 2.f - t;      // oct 6: (5, 6]
    }
}

Result<float, Error>
pseudoangleQuadrant(const Point2f& v) {
    float eps =defaultEpsilon<float>();

    const float x = v[0];
    const float y = v[1];

    if (std::abs(x) <= eps && std::abs(y) <= eps)
        return result::err(Error::Unknown);

    const float ax = std::abs(x);
    const float ay = std::abs(y);
    const float s  = ax + ay;        // sempre > 0

    if (x >= 0.f) {
        if (y >= 0.f) return y / s;              // Q1: [0, 1)
        else          return 3.f + x / s;       // Q4: [3, 4)
    } else {
        if (y >= 0.f) return 1.f + ax / s;      // Q2: [1, 2)
        else          return 2.f + ay / s;      // Q3: [2, 3)
    }
}

float orientedArea2(const Triangle2f& tri) {
    const auto& a = tri[0];
    const auto& b = tri[1];
    const auto& c = tri[2];

    Point2f ab{ b[0] - a[0], b[1] - a[1] };
    Point2f ac{ c[0] - a[0], c[1] - a[1] };
    return ab.cross(ac);
}

bool onSegment(const Segment2f& s, const Point2f& p) {
    float eps = defaultEpsilon<float>();

    const auto& a = s[0];
    const auto& b = s[1];

    return
        p[0] >= std::min(a[0], b[0]) - eps &&
        p[0] <= std::max(a[0], b[0]) + eps &&
        p[1] >= std::min(a[1], b[1]) - eps &&
        p[1] <= std::max(a[1], b[1]) + eps;
}

bool segmentIntersectionExists(const Segment2f& s1, const Segment2f& s2) {
    float eps = defaultEpsilon<float>();

    float o1 = orientedArea2(Triangle2f{s1[0], s1[1], s2[0]});
    float o2 = orientedArea2(Triangle2f{s1[0], s1[1], s2[1]});
    float o3 = orientedArea2(Triangle2f{s2[0], s2[1], s1[0]});
    float o4 = orientedArea2(Triangle2f{s2[0], s2[1], s1[1]});

    // Interseção própria
    if ((o1 > eps && o2 < -eps || o1 < -eps && o2 > eps) &&
        (o3 > eps && o4 < -eps || o3 < -eps && o4 > eps))
        return true;

    // Casos colineares
    if (isZero(o1) && onSegment(s1, s2[0])) return true;
    if (isZero(o2) && onSegment(s1, s2[1])) return true;
    if (isZero(o3) && onSegment(s2, s1[0])) return true;
    if (isZero(o4) && onSegment(s2, s1[1])) return true;

    return false;
}

bool isPointInsidePolygonRaycast(const Point2f& point, const Polygon2f& polygon) {
    float eps = defaultEpsilon<float>();

    int n = polygon.size();
    bool isInside = false;

    for (int i = 0, j = n - 1; i < n; j = i++) {
        if (((polygon[i][1] > point[1]) != (polygon[j][1] > point[1])) &&
            (point[0] < (polygon[j][0] - polygon[i][0]) * (point[1] - polygon[i][1]) / 
            (polygon[j][1] - polygon[i][1]) + polygon[i][0])) {
            isInside = !isInside;
        }
    }

    return isInside;
}

bool isPointInsidePolygonWinding(const Point2f& p, const Polygon2f& polygon)
{
    int windingNumber = 0;
    size_t n = polygon.size();

    for (size_t i = 0; i < n; ++i) {
        const auto& v1 = polygon[i];
        const auto& v2 = polygon[(i + 1) % n];
        
        // Aresta cruza para cima
        if (v1[1] <= p[1]) {
            if (v2[1] > p[1]) { // cruzamento ascendente
                if (orientedArea2(Triangle2f{v1, v2, p}) > 0)
                    ++windingNumber;
            }
        }
        // Aresta cruza para baixo
        else {
            if (v2[1] <= p[1]) { // cruzamento descendente
                if (orientedArea2(Triangle2f{v1, v2, p}) < 0)
                    --windingNumber;
            }
        }
    }

    return windingNumber != 0;
}

std::optional<Point2f> segmentIntersectionPoint(const Segment2f& s1, const Segment2f& s2) {
    float eps = defaultEpsilon<float>();

    const auto& p = s1[0];
    const auto& p2 = s1[1];
    const auto& q = s2[0];
    const auto& q2 = s2[1];

    Point2f r{ p2[0] - p[0], p2[1] - p[1] };
    Point2f s{ q2[0] - q[0], q2[1] - q[1] };

    float rxs = r.cross(s);
    float q_pxs = Point2f{ q[0] - p[0], q[1] - p[1] }.cross(s);

    if (std::abs(rxs) <= eps)
        return std::nullopt;

    float t = q_pxs / rxs;

    if (t < -eps || t > 1.f + eps)
        return std::nullopt;

    return Point2f{
        p[0] + t * r[0],
        p[1] + t * r[1]
    };
}

std::vector<Point2f> segmentPolygonIntersectionPoints(const Segment2f& segment, const Polygon2f& polygon) {
    float eps = defaultEpsilon<float>();

    std::vector<Point2f> points;

    for (std::size_t i = 0; i < polygon.size(); ++i) {
        Segment2f edge{
            polygon[i],
            polygon[(i + 1) % polygon.size()]
        };

        if (!segmentIntersectionExists(segment, edge))
            continue;

        auto p = segmentIntersectionPoint(segment, edge);
        if (p)
            points.push_back(*p);
    }

    return points;
}

}