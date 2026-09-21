// CalculatorCore: hardware-independent 4-function calculator engine with memory.
//
// Portability requirement: this header/implementation must compile unmodified
// under both a native desktop toolchain (for unit tests) and the Pico's
// earlephilhower Arduino core. To guarantee that, only <cstdint>, <cstddef>,
// and <cmath> are used - no STL containers, no exceptions, no dynamic
// allocation, no iostream.
#pragma once

#include <cstdint>

// Logical keys of the 4-wide x 5-tall keypad. Values map 1:1 to the physical
// layout documented in README.md; do not renumber without updating main.cpp's
// row/col lookup table.
enum class KeyInput : uint8_t {
    MemRecallClear, // MRC: tap-tap toggle - recall memory, then clear on repeat tap
    MemAdd,         // M+
    MemSubtract,    // M-
    Clear,          // C: full reset (does not clear memory)
    Digit7,
    Digit8,
    Digit9,
    Divide,
    Digit4,
    Digit5,
    Digit6,
    Multiply,
    Digit1,
    Digit2,
    Digit3,
    Subtract,
    Digit0,
    Decimal,
    Equals,
    Add,
};

// Snapshot of what should currently be shown on the display.
struct DisplayState {
    static constexpr uint8_t kMaxDigits = 12;

    // Only the first `digitCount` entries are meaningful, most-significant first.
    uint8_t digits[kMaxDigits] = {0};
    uint8_t digitCount = 0;

    // Number of digits (from the left) that appear before the decimal point.
    // Only meaningful when hasDecimalPoint is true.
    uint8_t decimalPointPosition = 0;
    bool hasDecimalPoint = false;

    bool negative = false;
    bool error = false;
};

class CalculatorCore {
public:
    // numDigits configures the display width; clamped to
    // [1, DisplayState::kMaxDigits].
    explicit CalculatorCore(uint8_t numDigits = 8);

    // Call once per key press (i.e. on the transition into the pressed state).
    void handleKey(KeyInput key);

    // Returns the current contents that should be shown on the display.
    DisplayState getDisplay() const;

private:
    enum class Operator : uint8_t {
        None,
        Add,
        Subtract,
        Multiply,
        Divide,
    };

    uint8_t numDigits_;

    double displayValue_ = 0.0;   // value currently entered or shown
    double accumulator_ = 0.0;    // running total for chained operations
    Operator pendingOperator_ = Operator::None;

    double lastOperand_ = 0.0;    // for repeated '=' presses
    Operator lastOperator_ = Operator::None;
    bool hasLastOperation_ = false;

    double memoryValue_ = 0.0;

    bool enteringNewNumber_ = true; // next digit/decimal starts a fresh number
    bool decimalEntered_ = false;
    uint8_t enteredDigitCount_ = 0; // digits typed so far for the current entry
    uint8_t fractionDigitCount_ = 0;

    bool mrcArmed_ = false; // true right after a recall, awaiting a possible clear tap
    bool errorState_ = false;

    void handleDigit(uint8_t digit);
    void handleDecimal();
    void handleOperator(Operator op);
    void handleEquals();
    void handleClear();
    void handleMemAdd();
    void handleMemSubtract();
    void handleMemRecallClear();

    // Applies lhs OP rhs; returns false (and leaves outResult untouched) on
    // divide-by-zero or overflow beyond the configured digit width.
    bool applyOperator(double lhs, Operator op, double rhs, double& outResult) const;
    bool fitsInDisplay(double value) const;
};
