#pragma once

struct InputState {
    bool keys[512]{};
    bool mouseButtons[8]{};

    double mouseX{0.0};
    double mouseY{0.0};

    double scrollOffset{0.0};

    void resetFrameData() {
        scrollOffset = 0.0;
    }
};