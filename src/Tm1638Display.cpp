#include "Tm1638Display.h"

#include <Arduino.h>
#include <SPI.h>

namespace {
constexpr uint32_t kSpiClockHz = 500000;
constexpr uint8_t kDataCommandAutoIncrement = 0x40;
constexpr uint8_t kAddressCommand = 0xC0;
constexpr uint8_t kDisplayControlCommand = 0x80;
constexpr uint8_t kDisplayOnBit = 0x08;

const SPISettings kSpiSettings(kSpiClockHz, LSBFIRST, SPI_MODE0);
} // namespace

Tm1638Display::Tm1638Display(uint8_t brightness)
    : brightness_(brightness > 7 ? 7 : brightness) {}

void Tm1638Display::begin() {
    SPI.setRX(kReceivePin);
    SPI.setCS(kStrobePin);
    SPI.setSCK(kClockPin);
    SPI.setTX(kDataPin);
    SPI.begin();

    pinMode(kStrobePin, OUTPUT);
    digitalWrite(kStrobePin, HIGH);
    initialized_ = true;

    clear();
}

void Tm1638Display::clear() {
    for (uint8_t i = 0; i < kRamBytes; i++) {
        frame_[i] = 0;
    }
    if (initialized_) {
        writeFrame();
    }
}

void Tm1638Display::render(const DisplayState& state) {
    renderFrame(Tm1638DisplayMapping::map(state));
}

void Tm1638Display::renderBrightness() {
    renderFrame(Tm1638DisplayMapping::mapBrightness(brightness_));
}

void Tm1638Display::renderTestPoint(uint8_t point) {
    renderFrame(Tm1638DisplayMapping::mapTestPoint(point));
}

uint8_t Tm1638Display::brightness() const {
    return brightness_;
}

void Tm1638Display::renderFrame(const Tm1638MappedFrame& mapped) {
    for (uint8_t i = 0; i < kRamBytes; i++) {
        frame_[i] = mapped.ramBytes[i];
    }
    if (initialized_) {
        writeFrame();
    }
}

void Tm1638Display::setBrightness(uint8_t brightness) {
    brightness_ = brightness > 7 ? 7 : brightness;
    if (initialized_) {
        writeDisplayControl();
    }
}

void Tm1638Display::setEnabled(bool enabled) {
    enabled_ = enabled;
    if (initialized_) {
        writeDisplayControl();
    }
}

void Tm1638Display::writeCommand(uint8_t command) {
    SPI.beginTransaction(kSpiSettings);
    digitalWrite(kStrobePin, LOW);
    SPI.transfer(command);
    digitalWrite(kStrobePin, HIGH);
    SPI.endTransaction();
}

void Tm1638Display::writeFrame() {
    writeCommand(kDataCommandAutoIncrement);

    SPI.beginTransaction(kSpiSettings);
    digitalWrite(kStrobePin, LOW);
    SPI.transfer(kAddressCommand);
    for (uint8_t i = 0; i < kRamBytes; i++) {
        SPI.transfer(frame_[i]);
    }
    digitalWrite(kStrobePin, HIGH);
    SPI.endTransaction();

    writeDisplayControl();
}

void Tm1638Display::writeDisplayControl() {
    const uint8_t command = kDisplayControlCommand |
        (enabled_ ? kDisplayOnBit : 0) | brightness_;
    writeCommand(command);
}