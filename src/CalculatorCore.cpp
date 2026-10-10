#include "CalculatorCore.h"

#include <cmath>

namespace {
constexpr uint8_t kDigitKeyValue(KeyInput key) {
    switch (key) {
        case KeyInput::Digit0: return 0;
        case KeyInput::Digit1: return 1;
        case KeyInput::Digit2: return 2;
        case KeyInput::Digit3: return 3;
        case KeyInput::Digit4: return 4;
        case KeyInput::Digit5: return 5;
        case KeyInput::Digit6: return 6;
        case KeyInput::Digit7: return 7;
        case KeyInput::Digit8: return 8;
        case KeyInput::Digit9: return 9;
        default: return 255; // not a digit key
    }
}
} // namespace

CalculatorCore::CalculatorCore(uint8_t numDigits) {
    if (numDigits < 1) {
        numDigits = 1;
    } else if (numDigits > DisplayState::kMaxDigits) {
        numDigits = DisplayState::kMaxDigits;
    }
    numDigits_ = numDigits;
}

void CalculatorCore::handleKey(KeyInput key) {
    // MRC's tap-tap toggle only survives if the very next key is MRC again.
    if (key != KeyInput::MemRecallClear) {
        mrcArmed_ = false;
    }

    if (key == KeyInput::Clear) {
        handleClear();
        return;
    }

    // Every other key is ignored while in the error state.
    if (errorState_) {
        return;
    }

    const uint8_t digit = kDigitKeyValue(key);
    if (digit <= 9) {
        handleDigit(digit);
        return;
    }

    switch (key) {
        case KeyInput::Decimal:
            handleDecimal();
            break;
        case KeyInput::Add:
            handleOperator(Operator::Add);
            break;
        case KeyInput::Subtract:
            handleOperator(Operator::Subtract);
            break;
        case KeyInput::Multiply:
            handleOperator(Operator::Multiply);
            break;
        case KeyInput::Divide:
            handleOperator(Operator::Divide);
            break;
        case KeyInput::Equals:
            handleEquals();
            break;
        case KeyInput::MemAdd:
            handleMemAdd();
            break;
        case KeyInput::MemSubtract:
            handleMemSubtract();
            break;
        case KeyInput::MemRecallClear:
            handleMemRecallClear();
            break;
        default:
            break; // Clear and digit keys are handled earlier
    }
}

void CalculatorCore::handleDigit(uint8_t digit) {
    if (enteringNewNumber_) {
        displayValue_ = 0.0;
        enteredDigitCount_ = 0;
        fractionDigitCount_ = 0;
        decimalEntered_ = false;
        enteringNewNumber_ = false;
        entryInProgress_ = true;
    }

    if (enteredDigitCount_ >= numDigits_) {
        return; // display full: silently ignore further digits
    }

    if (decimalEntered_) {
        uint8_t intDigits = 1;
        if (displayValue_ >= 1.0) {
            intDigits = static_cast<uint8_t>(std::floor(std::log10(displayValue_))) + 1;
        }
        if (intDigits + fractionDigitCount_ >= numDigits_) {
            return; // preserve one display position for the implicit integer zero
        }
        fractionDigitCount_++;
        displayValue_ += digit / std::pow(10.0, fractionDigitCount_);
    } else {
        displayValue_ = displayValue_ * 10.0 + digit;
    }
    enteredDigitCount_++;
}

void CalculatorCore::handleDecimal() {
    if (enteringNewNumber_) {
        displayValue_ = 0.0;
        enteredDigitCount_ = 0;
        fractionDigitCount_ = 0;
        enteringNewNumber_ = false;
        decimalEntered_ = false;
    }

    if (decimalEntered_) {
        return; // only one decimal point per number
    }
    if (enteredDigitCount_ >= numDigits_) {
        return; // no room left for a fractional part
    }
    decimalEntered_ = true;
    entryInProgress_ = true;
}

void CalculatorCore::handleOperator(Operator op) {
    if (pendingOperator_ != Operator::None) {
        double result = 0.0;
        if (!applyOperator(accumulator_, pendingOperator_, displayValue_, result)) {
            errorState_ = true;
            return;
        }
        accumulator_ = result;
        displayValue_ = result;
    } else {
        accumulator_ = displayValue_;
    }

    pendingOperator_ = op;
    hasLastOperation_ = false;
    enteringNewNumber_ = true;
    entryInProgress_ = false;
}

void CalculatorCore::handleEquals() {
    double result = 0.0;

    if (pendingOperator_ != Operator::None) {
        if (!applyOperator(accumulator_, pendingOperator_, displayValue_, result)) {
            errorState_ = true;
            return;
        }
        lastOperator_ = pendingOperator_;
        lastOperand_ = displayValue_;
        hasLastOperation_ = true;
    } else if (hasLastOperation_) {
        if (!applyOperator(displayValue_, lastOperator_, lastOperand_, result)) {
            errorState_ = true;
            return;
        }
    } else {
        return; // nothing to compute
    }

    accumulator_ = result;
    displayValue_ = result;
    pendingOperator_ = Operator::None;
    enteringNewNumber_ = true;
    entryInProgress_ = false;
}

void CalculatorCore::handleClear() {
    displayValue_ = 0.0;
    accumulator_ = 0.0;
    pendingOperator_ = Operator::None;
    lastOperand_ = 0.0;
    lastOperator_ = Operator::None;
    hasLastOperation_ = false;
    enteringNewNumber_ = true;
    entryInProgress_ = false;
    decimalEntered_ = false;
    enteredDigitCount_ = 0;
    fractionDigitCount_ = 0;
    mrcArmed_ = false;
    errorState_ = false;
    // memoryValue_ deliberately left untouched: C does not clear memory.
}

void CalculatorCore::handleMemAdd() {
    memoryValue_ += displayValue_;
    enteringNewNumber_ = true;
    entryInProgress_ = false;
}

void CalculatorCore::handleMemSubtract() {
    memoryValue_ -= displayValue_;
    enteringNewNumber_ = true;
    entryInProgress_ = false;
}

void CalculatorCore::handleMemRecallClear() {
    if (mrcArmed_) {
        memoryValue_ = 0.0;
        mrcArmed_ = false;
    } else {
        mrcArmed_ = true;
    }
    displayValue_ = memoryValue_;
    enteringNewNumber_ = true;
    entryInProgress_ = false;
    decimalEntered_ = false;
    enteredDigitCount_ = 0;
    fractionDigitCount_ = 0;
}

bool CalculatorCore::applyOperator(double lhs, Operator op, double rhs, double& outResult) const {
    double result = 0.0;
    switch (op) {
        case Operator::Add:
            result = lhs + rhs;
            break;
        case Operator::Subtract:
            result = lhs - rhs;
            break;
        case Operator::Multiply:
            result = lhs * rhs;
            break;
        case Operator::Divide:
            if (rhs == 0.0) {
                return false;
            }
            result = lhs / rhs;
            break;
        case Operator::None:
            result = rhs;
            break;
    }

    if (!fitsInDisplay(result)) {
        return false;
    }
    outResult = result;
    return true;
}

bool CalculatorCore::fitsInDisplay(double value) const {
    if (std::isnan(value) || std::isinf(value)) {
        return false;
    }
    const double limit = std::pow(10.0, numDigits_);
    return std::fabs(value) < limit;
}

DisplayState CalculatorCore::getDisplay() const {
    DisplayState state;

    if (errorState_) {
        state.error = true;
        return state;
    }

    double value = displayValue_;
    bool negative = value < 0.0;
    double magnitude = std::fabs(value);

    uint8_t intDigits = 1;
    if (magnitude >= 1.0) {
        intDigits = static_cast<uint8_t>(std::floor(std::log10(magnitude))) + 1;
    }
    if (intDigits > numDigits_) {
        state.error = true;
        return state;
    }

    uint8_t fracDigits = entryInProgress_ ? fractionDigitCount_ : numDigits_ - intDigits;
    double scale = std::pow(10.0, fracDigits);
    double scaledValue = std::round(magnitude * scale);

    // A rounding carry (e.g. 9.996 -> 10.00) can push the integer part one
    // digit wider than expected; absorb a single carry by borrowing a
    // fractional digit, or report overflow if there's no room left.
    if (scaledValue >= std::pow(10.0, numDigits_)) {
        if (fracDigits == 0) {
            state.error = true;
            return state;
        }
        intDigits++;
        fracDigits--;
        scale = std::pow(10.0, fracDigits);
        scaledValue = std::round(magnitude * scale);
    }

    auto scaledInt = static_cast<uint64_t>(scaledValue);
    if (scaledInt == 0) {
        negative = false; // avoid displaying "-0"
    }

    while (!entryInProgress_ && fracDigits > 0 && (scaledInt % 10) == 0) {
        scaledInt /= 10;
        fracDigits--;
    }

    state.digitCount = intDigits + fracDigits;
    for (uint8_t i = 0; i < state.digitCount; i++) {
        const uint8_t place = state.digitCount - 1 - i;
        uint64_t divisor = 1;
        for (uint8_t p = 0; p < place; p++) {
            divisor *= 10;
        }
        state.digits[i] = static_cast<uint8_t>((scaledInt / divisor) % 10);
    }

    state.hasDecimalPoint = entryInProgress_ ? decimalEntered_ : fracDigits > 0;
    state.decimalPointPosition = intDigits;
    state.entryInProgress = entryInProgress_;
    state.fractionalDigitCount = entryInProgress_ ? fractionDigitCount_ : 0;
    state.negative = negative;
    state.error = false;
    return state;
}
