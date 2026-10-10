#include "Tm1638DisplayMapping.h"

namespace {
constexpr uint8_t kSegmentA = 1u << 0;
constexpr uint8_t kSegmentB = 1u << 1;
constexpr uint8_t kSegmentC = 1u << 2;
constexpr uint8_t kSegmentD = 1u << 3;
constexpr uint8_t kSegmentE = 1u << 4;
constexpr uint8_t kSegmentF = 1u << 5;
constexpr uint8_t kSegmentG = 1u << 6;

constexpr uint8_t kDigitGlyphs[] = {
    kSegmentA | kSegmentB | kSegmentC | kSegmentD | kSegmentE | kSegmentF,
    kSegmentB | kSegmentC,
    kSegmentA | kSegmentB | kSegmentD | kSegmentE | kSegmentG,
    kSegmentA | kSegmentB | kSegmentC | kSegmentD | kSegmentG,
    kSegmentB | kSegmentC | kSegmentF | kSegmentG,
    kSegmentA | kSegmentC | kSegmentD | kSegmentF | kSegmentG,
    kSegmentA | kSegmentC | kSegmentD | kSegmentE | kSegmentF | kSegmentG,
    kSegmentA | kSegmentB | kSegmentC,
    kSegmentA | kSegmentB | kSegmentC | kSegmentD | kSegmentE | kSegmentF | kSegmentG,
    kSegmentA | kSegmentB | kSegmentC | kSegmentD | kSegmentF | kSegmentG,
};

constexpr uint8_t kErrorGlyphs[] = {
    kSegmentA | kSegmentD | kSegmentE | kSegmentF | kSegmentG,
    kSegmentE | kSegmentG,
    kSegmentE | kSegmentG,
};

void setGlyph(Tm1638MappedFrame& frame, uint8_t column, uint8_t glyph) {
    for (uint8_t row = 0; row < Tm1638MappedFrame::kGridCount; row++) {
        if ((glyph & (1u << row)) != 0) {
            frame.gridRows[row] |= static_cast<uint16_t>(1u << column);
        }
    }
}

void mapError(Tm1638MappedFrame& frame) {
    constexpr uint8_t kErrorStart = Tm1638MappedFrame::kDigitCount - 3;
    for (uint8_t i = 0; i < 3; i++) {
        setGlyph(frame, kErrorStart + i, kErrorGlyphs[i]);
    }
}

void packFrame(Tm1638MappedFrame& frame) {
    for (uint8_t grid = 0; grid < Tm1638MappedFrame::kGridCount; grid++) {
        frame.ramBytes[grid * 2] = static_cast<uint8_t>(frame.gridRows[grid] & 0xFF);
        frame.ramBytes[grid * 2 + 1] = static_cast<uint8_t>((frame.gridRows[grid] >> 8) & 0x03);
    }
}
} // namespace

Tm1638MappedFrame Tm1638DisplayMapping::map(const DisplayState& state) {
    Tm1638MappedFrame frame;
    if (state.error || state.digitCount > Tm1638MappedFrame::kDigitCount) {
        mapError(frame);
        packFrame(frame);
        return frame;
    }

    const bool pendingDecimal = state.entryInProgress && state.hasDecimalPoint &&
        state.fractionalDigitCount == 0;
    uint8_t firstColumn = Tm1638MappedFrame::kDigitCount - state.digitCount;
    if (pendingDecimal) {
        if (firstColumn == 0) {
            mapError(frame);
            packFrame(frame);
            return frame;
        }
        firstColumn--;
    }

    for (uint8_t i = 0; i < state.digitCount; i++) {
        const uint8_t digit = state.digits[i];
        if (digit <= 9) {
            setGlyph(frame, firstColumn + i, kDigitGlyphs[digit]);
        }
    }

    if (state.hasDecimalPoint && state.decimalPointPosition > 0 &&
        state.decimalPointPosition <= state.digitCount) {
        const uint8_t decimalColumn = firstColumn + state.decimalPointPosition - 1;
        if (decimalColumn < Tm1638MappedFrame::kDigitCount - 1) {
            frame.gridRows[7] |= static_cast<uint16_t>(1u << decimalColumn);
        }
    }

    if (state.negative) {
        if (firstColumn > 0) {
            setGlyph(frame, firstColumn - 1, kSegmentG);
        } else {
            // The rightmost h channel is wired to the separate minus LED.
            frame.gridRows[7] |= static_cast<uint16_t>(1u << (Tm1638MappedFrame::kDigitCount - 1));
        }
    }

    packFrame(frame);
    return frame;
}