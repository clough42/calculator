#include <unity.h>

#include "CalculatorCore.h"

namespace {

void pressDigits(CalculatorCore& calc, const char* digits) {
    for (const char* p = digits; *p != '\0'; p++) {
        switch (*p) {
            case '0': calc.handleKey(KeyInput::Digit0); break;
            case '1': calc.handleKey(KeyInput::Digit1); break;
            case '2': calc.handleKey(KeyInput::Digit2); break;
            case '3': calc.handleKey(KeyInput::Digit3); break;
            case '4': calc.handleKey(KeyInput::Digit4); break;
            case '5': calc.handleKey(KeyInput::Digit5); break;
            case '6': calc.handleKey(KeyInput::Digit6); break;
            case '7': calc.handleKey(KeyInput::Digit7); break;
            case '8': calc.handleKey(KeyInput::Digit8); break;
            case '9': calc.handleKey(KeyInput::Digit9); break;
            case '.': calc.handleKey(KeyInput::Decimal); break;
            default: break;
        }
    }
}

// Asserts the display shows exactly `expected` (e.g. "-12.5") once rendered
// with a decimal point and optional leading '-', left-padded/trimmed per the
// configured digit count.
void assertDisplay(const DisplayState& state, const char* expected) {
    TEST_ASSERT_FALSE(state.error);

    bool expectNegative = expected[0] == '-';
    const char* numStart = expectNegative ? expected + 1 : expected;

    char rendered[DisplayState::kMaxDigits + 2] = {0};
    int pos = 0;
    if (state.negative) {
        rendered[pos++] = '-';
    }
    for (uint8_t i = 0; i < state.digitCount; i++) {
        if (state.hasDecimalPoint && i == state.decimalPointPosition) {
            rendered[pos++] = '.';
        }
        rendered[pos++] = static_cast<char>('0' + state.digits[i]);
    }
    if (state.hasDecimalPoint && state.decimalPointPosition == state.digitCount) {
        rendered[pos++] = '.';
    }
    rendered[pos] = '\0';

    // Compare numerically-equivalent strings by trimming leading zeros in the
    // integer part so callers can write natural expected values like "9".
    (void)numStart;
    TEST_ASSERT_EQUAL_STRING(expected, rendered);
}

} // namespace

void setUp() {}
void tearDown() {}

void test_basic_addition() {
    CalculatorCore calc(8);
    pressDigits(calc, "2");
    calc.handleKey(KeyInput::Add);
    pressDigits(calc, "3");
    calc.handleKey(KeyInput::Equals);
    assertDisplay(calc.getDisplay(), "5");
}

void test_chained_operations() {
    CalculatorCore calc(8);
    pressDigits(calc, "2");
    calc.handleKey(KeyInput::Add);
    pressDigits(calc, "3");
    calc.handleKey(KeyInput::Add); // applies 2+3=5 immediately, chains
    pressDigits(calc, "4");
    calc.handleKey(KeyInput::Equals); // 5+4=9
    assertDisplay(calc.getDisplay(), "9");
}

void test_repeated_equals_repeats_last_operation() {
    CalculatorCore calc(8);
    pressDigits(calc, "2");
    calc.handleKey(KeyInput::Add);
    pressDigits(calc, "3");
    calc.handleKey(KeyInput::Equals); // 5
    calc.handleKey(KeyInput::Equals); // 5+3=8
    calc.handleKey(KeyInput::Equals); // 8+3=11
    assertDisplay(calc.getDisplay(), "11");
}

void test_decimal_entry() {
    CalculatorCore calc(8);
    pressDigits(calc, "1.5");
    calc.handleKey(KeyInput::Add);
    pressDigits(calc, "2.25");
    calc.handleKey(KeyInput::Equals);
    assertDisplay(calc.getDisplay(), "3.75");
}

void test_pending_decimal_is_included_in_display_state() {
    CalculatorCore calc(10);
    pressDigits(calc, "1.");

    const DisplayState state = calc.getDisplay();
    TEST_ASSERT_TRUE(state.entryInProgress);
    TEST_ASSERT_TRUE(state.hasDecimalPoint);
    TEST_ASSERT_EQUAL_UINT8(0, state.fractionalDigitCount);
    assertDisplay(state, "1.");
}

void test_initial_decimal_displays_zero_and_point() {
    CalculatorCore calc(10);
    pressDigits(calc, ".");

    const DisplayState state = calc.getDisplay();
    TEST_ASSERT_TRUE(state.hasDecimalPoint);
    TEST_ASSERT_EQUAL_UINT8(1, state.digitCount);
    assertDisplay(state, "0.");
}

void test_fractional_trailing_zeroes_remain_visible_during_entry() {
    CalculatorCore calc(10);
    pressDigits(calc, "1.00");

    DisplayState state = calc.getDisplay();
    TEST_ASSERT_EQUAL_UINT8(2, state.fractionalDigitCount);
    assertDisplay(state, "1.00");

    calc.handleKey(KeyInput::Digit3);
    state = calc.getDisplay();
    TEST_ASSERT_EQUAL_UINT8(3, state.fractionalDigitCount);
    assertDisplay(state, "1.003");
}

void test_second_decimal_point_is_ignored_without_losing_entry() {
    CalculatorCore calc(10);
    pressDigits(calc, "1.0");
    calc.handleKey(KeyInput::Decimal);
    calc.handleKey(KeyInput::Digit0);

    const DisplayState state = calc.getDisplay();
    TEST_ASSERT_EQUAL_UINT8(2, state.fractionalDigitCount);
    assertDisplay(state, "1.00");
}

void test_fractional_trailing_zeroes_normalize_after_calculation() {
    CalculatorCore calc(10);
    pressDigits(calc, "1.00");
    calc.handleKey(KeyInput::Add);
    pressDigits(calc, "0");
    calc.handleKey(KeyInput::Equals);

    const DisplayState state = calc.getDisplay();
    TEST_ASSERT_FALSE(state.entryInProgress);
    assertDisplay(state, "1");
}

void test_pending_decimal_respects_ten_digit_capacity() {
    CalculatorCore full(10);
    pressDigits(full, "1234567890.");
    TEST_ASSERT_FALSE(full.getDisplay().hasDecimalPoint);
    assertDisplay(full.getDisplay(), "1234567890");

    CalculatorCore roomForFraction(10);
    pressDigits(roomForFraction, "123456789.");
    assertDisplay(roomForFraction.getDisplay(), "123456789.");
    roomForFraction.handleKey(KeyInput::Digit0);
    assertDisplay(roomForFraction.getDisplay(), "123456789.0");
}

void test_negative_result() {
    CalculatorCore calc(8);
    pressDigits(calc, "3");
    calc.handleKey(KeyInput::Subtract);
    pressDigits(calc, "5");
    calc.handleKey(KeyInput::Equals);
    assertDisplay(calc.getDisplay(), "-2");
}

void test_divide_by_zero_sets_error() {
    CalculatorCore calc(8);
    pressDigits(calc, "5");
    calc.handleKey(KeyInput::Divide);
    pressDigits(calc, "0");
    calc.handleKey(KeyInput::Equals);
    DisplayState state = calc.getDisplay();
    TEST_ASSERT_TRUE(state.error);
}

void test_clear_recovers_from_error_but_keeps_memory() {
    CalculatorCore calc(8);
    pressDigits(calc, "7");
    calc.handleKey(KeyInput::MemAdd); // memory = 7
    pressDigits(calc, "5");
    calc.handleKey(KeyInput::Divide);
    pressDigits(calc, "0");
    calc.handleKey(KeyInput::Equals); // error
    TEST_ASSERT_TRUE(calc.getDisplay().error);

    calc.handleKey(KeyInput::Clear);
    TEST_ASSERT_FALSE(calc.getDisplay().error);
    assertDisplay(calc.getDisplay(), "0");

    calc.handleKey(KeyInput::MemRecallClear); // recall memory: should still be 7
    assertDisplay(calc.getDisplay(), "7");
}

void test_keys_ignored_during_error_except_clear() {
    CalculatorCore calc(8);
    pressDigits(calc, "5");
    calc.handleKey(KeyInput::Divide);
    pressDigits(calc, "0");
    calc.handleKey(KeyInput::Equals); // error

    pressDigits(calc, "9"); // should be ignored
    calc.handleKey(KeyInput::Add); // should be ignored
    calc.handleKey(KeyInput::MemAdd); // should be ignored
    TEST_ASSERT_TRUE(calc.getDisplay().error);
}

void test_digit_overflow_is_silently_ignored() {
    CalculatorCore calc(4);
    pressDigits(calc, "123456"); // only first 4 digits ("1234") should register
    assertDisplay(calc.getDisplay(), "1234");
}

void test_memory_add_and_subtract() {
    CalculatorCore calc(8);
    pressDigits(calc, "10");
    calc.handleKey(KeyInput::MemAdd); // memory = 10
    pressDigits(calc, "4");
    calc.handleKey(KeyInput::MemSubtract); // memory = 6

    calc.handleKey(KeyInput::MemRecallClear); // recall -> display 6
    assertDisplay(calc.getDisplay(), "6");
}

void test_mrc_tap_tap_clears_memory() {
    CalculatorCore calc(8);
    pressDigits(calc, "42");
    calc.handleKey(KeyInput::MemAdd); // memory = 42

    calc.handleKey(KeyInput::MemRecallClear); // 1st tap: recall -> 42
    assertDisplay(calc.getDisplay(), "42");

    calc.handleKey(KeyInput::MemRecallClear); // 2nd tap: clear memory -> 0
    assertDisplay(calc.getDisplay(), "0");

    calc.handleKey(KeyInput::MemRecallClear); // acts as a fresh 1st tap again
    assertDisplay(calc.getDisplay(), "0");
}

void test_mrc_toggle_disarmed_by_other_key() {
    CalculatorCore calc(8);
    pressDigits(calc, "42");
    calc.handleKey(KeyInput::MemAdd); // memory = 42

    calc.handleKey(KeyInput::MemRecallClear); // recall -> 42
    pressDigits(calc, "1"); // any other key disarms the toggle
    calc.handleKey(KeyInput::MemRecallClear); // this is a fresh 1st tap: recall again
    assertDisplay(calc.getDisplay(), "42");
}

void test_configurable_digit_count() {
    CalculatorCore calc(3);
    pressDigits(calc, "7");
    calc.handleKey(KeyInput::Add);
    pressDigits(calc, "1");
    calc.handleKey(KeyInput::Equals);
    assertDisplay(calc.getDisplay(), "8");
}

void test_ten_digit_display_capacity() {
    CalculatorCore calc(10);
    pressDigits(calc, "1234567890");

    const DisplayState state = calc.getDisplay();
    TEST_ASSERT_EQUAL_UINT8(10, state.digitCount);
    assertDisplay(state, "1234567890");
}

void test_overflow_beyond_digit_capacity_sets_error() {
    CalculatorCore calc(3); // max representable magnitude is < 1000
    pressDigits(calc, "999");
    calc.handleKey(KeyInput::Add);
    pressDigits(calc, "5");
    calc.handleKey(KeyInput::Equals);
    TEST_ASSERT_TRUE(calc.getDisplay().error);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_basic_addition);
    RUN_TEST(test_chained_operations);
    RUN_TEST(test_repeated_equals_repeats_last_operation);
    RUN_TEST(test_decimal_entry);
    RUN_TEST(test_pending_decimal_is_included_in_display_state);
    RUN_TEST(test_initial_decimal_displays_zero_and_point);
    RUN_TEST(test_fractional_trailing_zeroes_remain_visible_during_entry);
    RUN_TEST(test_second_decimal_point_is_ignored_without_losing_entry);
    RUN_TEST(test_fractional_trailing_zeroes_normalize_after_calculation);
    RUN_TEST(test_pending_decimal_respects_ten_digit_capacity);
    RUN_TEST(test_negative_result);
    RUN_TEST(test_divide_by_zero_sets_error);
    RUN_TEST(test_clear_recovers_from_error_but_keeps_memory);
    RUN_TEST(test_keys_ignored_during_error_except_clear);
    RUN_TEST(test_digit_overflow_is_silently_ignored);
    RUN_TEST(test_memory_add_and_subtract);
    RUN_TEST(test_mrc_tap_tap_clears_memory);
    RUN_TEST(test_mrc_toggle_disarmed_by_other_key);
    RUN_TEST(test_configurable_digit_count);
    RUN_TEST(test_ten_digit_display_capacity);
    RUN_TEST(test_overflow_beyond_digit_capacity_sets_error);
    return UNITY_END();
}
