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

    static constexpr Color White()       { return {1.f, 1.f, 1.f, 1.f}; }
    static constexpr Color Black()       { return {0.f, 0.f, 0.f, 1.f}; }

    static constexpr Color Red()         { return {1.f, 0.f, 0.f, 1.f}; }
    static constexpr Color Green()       { return {0.f, 1.f, 0.f, 1.f}; }
    static constexpr Color Blue()        { return {0.f, 0.f, 1.f, 1.f}; }

    static constexpr Color Yellow()      { return {1.f, 1.f, 0.f, 1.f}; }
    static constexpr Color Cyan()        { return {0.f, 1.f, 1.f, 1.f}; }
    static constexpr Color Magenta()     { return {1.f, 0.f, 1.f, 1.f}; }

    static constexpr Color Orange()      { return {1.f, 0.5f, 0.f, 1.f}; }
    static constexpr Color Purple()      { return {0.5f, 0.f, 0.5f, 1.f}; }
    static constexpr Color Pink()        { return {1.f, 0.4f, 0.7f, 1.f}; }

    static constexpr Color Brown()       { return {0.6f, 0.3f, 0.1f, 1.f}; }

    static constexpr Color Lime()        { return {0.5f, 1.f, 0.f, 1.f}; }
    static constexpr Color Olive()       { return {0.5f, 0.5f, 0.f, 1.f}; }

    static constexpr Color Navy()        { return {0.f, 0.f, 0.5f, 1.f}; }
    static constexpr Color Teal()        { return {0.f, 0.5f, 0.5f, 1.f}; }

    static constexpr Color Maroon()      { return {0.5f, 0.f, 0.f, 1.f}; }

    static constexpr Color Silver()      { return {0.75f, 0.75f, 0.75f, 1.f}; }
    static constexpr Color Gray()        { return {0.5f, 0.5f, 0.5f, 1.f}; }

    static constexpr Color DarkGray()    { return {0.25f, 0.25f, 0.25f, 1.f}; }
    static constexpr Color LightGray()   { return {0.8f, 0.8f, 0.8f, 1.f}; }

    static constexpr Color Gold()        { return {1.f, 0.84f, 0.f, 1.f}; }
    static constexpr Color Coral()       { return {1.f, 0.5f, 0.31f, 1.f}; }
    static constexpr Color Violet()      { return {0.93f, 0.51f, 0.93f, 1.f}; }

    static constexpr Color SkyBlue()     { return {0.53f, 0.81f, 0.92f, 1.f}; }
    static constexpr Color RoyalBlue()   { return {0.25f, 0.41f, 0.88f, 1.f}; }

    static constexpr Color ForestGreen() { return {0.13f, 0.55f, 0.13f, 1.f}; }
    static constexpr Color DarkGreen()   { return {0.f, 0.39f, 0.f, 1.f}; }

    static constexpr Color Transparent() { return {0.f, 0.f, 0.f, 0.f}; }

    static constexpr Color AxisX()       { return Red();   }
    static constexpr Color AxisY()       { return Green(); }
    static constexpr Color AxisZ()       { return Blue();  }
};

} // namespace geometry