#pragma once

#include <cstdint>

#include "CalculatorCore.h"

struct Tm1638MappedFrame {
    static constexpr uint8_t kGridCount = 8;
    static constexpr uint8_t kDigitCount = 10;

    // Each bit represents one SEG output; row 0..7 maps to a..g and h (dp).
    uint16_t gridRows[kGridCount] = {0};
    uint8_t ramBytes[kGridCount * 2] = {0};
};

class Tm1638DisplayMapping {
public:
    static Tm1638MappedFrame map(const DisplayState& state);
    static Tm1638MappedFrame mapBrightness(uint8_t level);
    static Tm1638MappedFrame mapTestPoint(uint8_t point);
};