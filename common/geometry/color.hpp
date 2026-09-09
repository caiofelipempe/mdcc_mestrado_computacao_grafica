// color.hpp

#pragma once

namespace geometry {

struct Color {
    float r;
    float g;
    float b;
    float a;

    constexpr Color(
        float r = 1.0f,
        float g = 1.0f,
        float b = 1.0f,
        float a = 1.0f)
        : r(r)
        , g(g)
        , b(b)
        , a(a)
    {}

    static constexpr Color White() {
        return {1.f, 1.f, 1.f, 1.f};
    }

    static constexpr Color Black() {
        return {0.f, 0.f, 0.f, 1.f};
    }

    static constexpr Color Red() {
        return {1.f, 0.f, 0.f, 1.f};
    }

    static constexpr Color Green() {
        return {0.f, 1.f, 0.f, 1.f};
    }

    static constexpr Color Blue() {
        return {0.f, 0.f, 1.f, 1.f};
    }

    static constexpr Color Yellow() {
        return {1.f, 1.f, 0.f, 1.f};
    }
};

}
