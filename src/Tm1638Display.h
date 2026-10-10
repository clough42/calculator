#pragma once

#include <cstdint>

#include "Tm1638DisplayMapping.h"

class Tm1638Display {
public:
    static constexpr uint8_t kDefaultBrightness = 3;

    explicit Tm1638Display(uint8_t brightness = kDefaultBrightness);

    void begin();
    void clear();
    void render(const DisplayState& state);
    void renderBrightness();
    void renderTestStep(uint8_t step);
    void setBrightness(uint8_t brightness);
    void setEnabled(bool enabled);
    uint8_t brightness() const;

private:
    static constexpr uint8_t kStrobePin = 17;
    static constexpr uint8_t kClockPin = 18;
    static constexpr uint8_t kDataPin = 19;
    static constexpr uint8_t kReceivePin = 16;
    static constexpr uint8_t kRamBytes = 16;

    uint8_t brightness_;
    bool enabled_ = true;
    bool initialized_ = false;
    uint8_t frame_[kRamBytes] = {0};

    void writeCommand(uint8_t command);
    void writeFrame();
    void writeDisplayControl();
    void renderFrame(const Tm1638MappedFrame& mapped);
};