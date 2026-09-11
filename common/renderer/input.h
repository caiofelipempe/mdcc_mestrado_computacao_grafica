#pragma once

#include <array>
#include <cctype>
#include <cstdint>

enum class Key : std::uint16_t
{
    Escape = 256,

    Enter,
    Tab,
    Backspace,

    Insert,
    Delete,

    Right,
    Left,
    Down,
    Up,

    PageUp,
    PageDown,

    Home,
    End,

    CapsLock,
    ScrollLock,
    NumLock,

    PrintScreen,
    Pause,

    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

    LeftShift,
    LeftControl,
    LeftAlt,

    RightShift,
    RightControl,
    RightAlt
};

enum class MouseButton : std::uint8_t
{
    Left   = 0,
    Right  = 1,
    Middle = 2
};

enum class GamepadButton : std::uint8_t
{
    South = 0,
    East,
    West,
    North,

    LeftBumper,
    RightBumper,

    Back,
    Start,
    Guide,

    LeftThumb,
    RightThumb,

    DPadUp,
    DPadRight,
    DPadDown,
    DPadLeft
};

enum class GamepadAxis : std::uint8_t
{
    LeftX = 0,
    LeftY,

    RightX,
    RightY,

    LeftTrigger,
    RightTrigger
};

struct InputState
{
private:

    std::array<bool, 512> m_keys{};
    std::array<bool, 8>   m_mouseButtons{};

    std::array<bool, 16>  m_gamepadButtons{};
    std::array<float, 6>  m_gamepadAxes{};

    bool m_gamepadConnected{false};

public:

    double mouseX{0.0};
    double mouseY{0.0};

    double scrollOffset{0.0};

public:

    [[nodiscard]]
    bool pressed(char key) const
    {
        const auto code =
            static_cast<unsigned char>(
                std::toupper(key)
            );

        return m_keys[code];
    }

    [[nodiscard]]
    bool pressed(Key key) const
    {
        return m_keys[
            static_cast<std::size_t>(key)
        ];
    }

    [[nodiscard]]
    bool mouse(MouseButton button) const
    {
        return m_mouseButtons[
            static_cast<std::size_t>(button)
        ];
    }

    [[nodiscard]]
    bool leftMouse() const
    {
        return mouse(MouseButton::Left);
    }

    [[nodiscard]]
    bool rightMouse() const
    {
        return mouse(MouseButton::Right);
    }

    [[nodiscard]]
    bool middleMouse() const
    {
        return mouse(MouseButton::Middle);
    }

    [[nodiscard]]
    bool button(GamepadButton button) const
    {
        return m_gamepadButtons[
            static_cast<std::size_t>(button)
        ];
    }

    [[nodiscard]]
    float axis(GamepadAxis axis) const
    {
        return m_gamepadAxes[
            static_cast<std::size_t>(axis)
        ];
    }

    [[nodiscard]]
    bool gamepadConnected() const
    {
        return m_gamepadConnected;
    }

    void resetFrameData()
    {
        scrollOffset = 0.0;
    }

private:

    friend class RendererGlfwOpengl;
};