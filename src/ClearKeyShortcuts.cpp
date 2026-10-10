#include "ClearKeyShortcuts.h"

ClearKeyAction ClearKeyShortcuts::keyPressed(KeyInput key) {
    if (mode_ == Mode::Normal) {
        if (key == KeyInput::Clear) {
            mode_ = Mode::AwaitingShortcut;
            return ClearKeyAction::Ignore;
        }
        return ClearKeyAction::ForwardKey;
    }

    if (mode_ == Mode::AwaitingShortcut) {
        if (key == KeyInput::Add) {
            mode_ = Mode::Brightness;
            return ClearKeyAction::IncreaseBrightness;
        }
        if (key == KeyInput::Subtract) {
            mode_ = Mode::Brightness;
            return ClearKeyAction::DecreaseBrightness;
        }
        if (key == KeyInput::Equals) {
            mode_ = Mode::DisplayTest;
            return ClearKeyAction::StartDisplayTest;
        }
        return ClearKeyAction::Ignore;
    }

    if (mode_ == Mode::Brightness) {
        if (key == KeyInput::Add) {
            return ClearKeyAction::IncreaseBrightness;
        }
        if (key == KeyInput::Subtract) {
            return ClearKeyAction::DecreaseBrightness;
        }
    }

    return ClearKeyAction::Ignore;
}

ClearKeyAction ClearKeyShortcuts::clearKeyReleased() {
    const Mode previousMode = mode_;
    mode_ = Mode::Normal;

    switch (previousMode) {
        case Mode::AwaitingShortcut:
            return ClearKeyAction::ClearCalculator;
        case Mode::Brightness:
            return ClearKeyAction::RestoreCalculatorDisplay;
        case Mode::DisplayTest:
            return ClearKeyAction::StopDisplayTest;
        case Mode::Normal:
            return ClearKeyAction::Ignore;
    }
    return ClearKeyAction::Ignore;
}