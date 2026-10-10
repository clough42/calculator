#pragma once

#include "CalculatorCore.h"

enum class ClearKeyAction : uint8_t {
    Ignore,
    ForwardKey,
    ClearCalculator,
    IncreaseBrightness,
    DecreaseBrightness,
    StartDisplayTest,
    StopDisplayTest,
    RestoreCalculatorDisplay,
};

class ClearKeyShortcuts {
public:
    ClearKeyAction keyPressed(KeyInput key);
    ClearKeyAction clearKeyReleased();

private:
    enum class Mode : uint8_t {
        Normal,
        AwaitingShortcut,
        Brightness,
        DisplayTest,
    };

    Mode mode_ = Mode::Normal;
};