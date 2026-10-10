#include <unity.h>

#include "Tm1638DisplayMapping.h"

void setUp() {}
void tearDown() {}

void test_single_digit_is_right_aligned_and_transposed() {
    DisplayState state;
    state.digits[0] = 1;
    state.digitCount = 1;

    const Tm1638MappedFrame frame = Tm1638DisplayMapping::map(state);
    TEST_ASSERT_EQUAL_HEX16(1u << 9, frame.gridRows[1]);
    TEST_ASSERT_EQUAL_HEX16(1u << 9, frame.gridRows[2]);
    TEST_ASSERT_EQUAL_HEX16(0, frame.gridRows[0]);
    TEST_ASSERT_EQUAL_HEX16(0, frame.gridRows[7]);
    TEST_ASSERT_EQUAL_HEX8(2, frame.ramBytes[3]);
    TEST_ASSERT_EQUAL_HEX8(2, frame.ramBytes[5]);
}

void test_decimal_point_maps_to_previous_digit_h_row() {
    DisplayState state;
    state.digits[0] = 1;
    state.digits[1] = 2;
    state.digitCount = 2;
    state.hasDecimalPoint = true;
    state.decimalPointPosition = 1;

    const Tm1638MappedFrame frame = Tm1638DisplayMapping::map(state);
    TEST_ASSERT_EQUAL_HEX16(1u << 8, frame.gridRows[7]);
    TEST_ASSERT_EQUAL_HEX16((1u << 8) | (1u << 9), frame.gridRows[1]);
    TEST_ASSERT_EQUAL_HEX16(1u << 9, frame.gridRows[0]);
    TEST_ASSERT_EQUAL_HEX16(1u << 8, frame.gridRows[2]);
    TEST_ASSERT_EQUAL_HEX8(1, frame.ramBytes[15]);
}

void test_pending_decimal_shifts_digits_and_keeps_last_column_blank() {
    DisplayState state;
    state.digits[0] = 1;
    state.digits[1] = 2;
    state.digitCount = 2;
    state.decimalPointPosition = 2;
    state.hasDecimalPoint = true;
    state.entryInProgress = true;

    const Tm1638MappedFrame frame = Tm1638DisplayMapping::map(state);
    TEST_ASSERT_EQUAL_HEX16(1u << 8, frame.gridRows[7]);
    TEST_ASSERT_EQUAL_HEX16(1u << 8, frame.gridRows[0]);
    for (uint8_t row = 0; row < Tm1638MappedFrame::kGridCount; row++) {
        TEST_ASSERT_EQUAL_HEX16(0, frame.gridRows[row] & (1u << 9));
    }
}

void test_pending_initial_decimal_shows_zero_and_leaves_last_column_blank() {
    DisplayState state;
    state.digits[0] = 0;
    state.digitCount = 1;
    state.decimalPointPosition = 1;
    state.hasDecimalPoint = true;
    state.entryInProgress = true;

    const Tm1638MappedFrame frame = Tm1638DisplayMapping::map(state);
    TEST_ASSERT_EQUAL_HEX16(1u << 8, frame.gridRows[7]);
    for (uint8_t row = 0; row < Tm1638MappedFrame::kGridCount; row++) {
        TEST_ASSERT_EQUAL_HEX16(0, frame.gridRows[row] & (1u << 9));
    }
}

void test_fractional_trailing_zeroes_map_to_digit_glyphs() {
    DisplayState state;
    state.digits[0] = 1;
    state.digits[1] = 0;
    state.digits[2] = 0;
    state.digitCount = 3;
    state.decimalPointPosition = 1;
    state.fractionalDigitCount = 2;
    state.hasDecimalPoint = true;
    state.entryInProgress = true;

    const Tm1638MappedFrame frame = Tm1638DisplayMapping::map(state);
    TEST_ASSERT_EQUAL_HEX16(1u << 7, frame.gridRows[7]);
    TEST_ASSERT_BITS(1u << 9, 1u << 9, frame.gridRows[0]);
    TEST_ASSERT_BITS(1u << 9, 1u << 9, frame.gridRows[5]);
}

void test_short_negative_uses_center_segment_before_number() {
    DisplayState state;
    state.digits[0] = 1;
    state.digitCount = 1;
    state.hasDecimalPoint = true;
    state.decimalPointPosition = 1;
    state.negative = true;

    const Tm1638MappedFrame frame = Tm1638DisplayMapping::map(state);
    TEST_ASSERT_EQUAL_HEX16(1u << 8, frame.gridRows[6]);
    TEST_ASSERT_EQUAL_HEX16(0, frame.gridRows[7]);
    TEST_ASSERT_EQUAL_HEX8(1, frame.ramBytes[13]);
    TEST_ASSERT_EQUAL_HEX8(0, frame.ramBytes[15]);
}

void test_full_width_negative_uses_external_minus_segment() {
    DisplayState state;
    state.digitCount = Tm1638MappedFrame::kDigitCount;
    state.negative = true;
    for (uint8_t i = 0; i < state.digitCount; i++) {
        state.digits[i] = i;
    }

    const Tm1638MappedFrame frame = Tm1638DisplayMapping::map(state);
    TEST_ASSERT_EQUAL_HEX16(1u << 9, frame.gridRows[7]);
    TEST_ASSERT_EQUAL_HEX8(2, frame.ramBytes[15]);
}

void test_error_state_renders_err_on_rightmost_digits() {
    DisplayState state;
    state.error = true;

    const Tm1638MappedFrame frame = Tm1638DisplayMapping::map(state);
    TEST_ASSERT_EQUAL_HEX16((1u << 7) | (1u << 8) | (1u << 9), frame.gridRows[4]);
    TEST_ASSERT_EQUAL_HEX16((1u << 7) | (1u << 8) | (1u << 9), frame.gridRows[6]);
    TEST_ASSERT_EQUAL_HEX16(1u << 7, frame.gridRows[0]);
}

void test_brightness_feedback_shows_b_and_selected_level() {
    const Tm1638MappedFrame frame = Tm1638DisplayMapping::mapBrightness(3);
    TEST_ASSERT_EQUAL_HEX16(1u << 9, frame.gridRows[0]);
    TEST_ASSERT_EQUAL_HEX16(1u << 9, frame.gridRows[1]);
    TEST_ASSERT_EQUAL_HEX16((1u << 8) | (1u << 9), frame.gridRows[2]);
    TEST_ASSERT_EQUAL_HEX16((1u << 8) | (1u << 9), frame.gridRows[3]);
    TEST_ASSERT_EQUAL_HEX16(1u << 8, frame.gridRows[4]);
    TEST_ASSERT_EQUAL_HEX16(1u << 8, frame.gridRows[5]);
    TEST_ASSERT_EQUAL_HEX16((1u << 8) | (1u << 9), frame.gridRows[6]);
}

void test_test_point_selects_exactly_one_matrix_led() {
    for (uint8_t point = 0; point < Tm1638MappedFrame::kGridCount * Tm1638MappedFrame::kDigitCount; point++) {
        const Tm1638MappedFrame frame = Tm1638DisplayMapping::mapTestPoint(point);
        const uint8_t expectedColumn = point / Tm1638MappedFrame::kGridCount;
        const uint8_t expectedGrid = point % Tm1638MappedFrame::kGridCount;
        const uint16_t expectedColumnMask = static_cast<uint16_t>(1u << expectedColumn);
        for (uint8_t grid = 0; grid < Tm1638MappedFrame::kGridCount; grid++) {
            TEST_ASSERT_EQUAL_HEX16(grid == expectedGrid ? expectedColumnMask : 0, frame.gridRows[grid]);
        }
    }
    TEST_ASSERT_EQUAL_HEX16(0, Tm1638DisplayMapping::mapTestPoint(80).gridRows[0]);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_single_digit_is_right_aligned_and_transposed);
    RUN_TEST(test_decimal_point_maps_to_previous_digit_h_row);
    RUN_TEST(test_pending_decimal_shifts_digits_and_keeps_last_column_blank);
    RUN_TEST(test_pending_initial_decimal_shows_zero_and_leaves_last_column_blank);
    RUN_TEST(test_fractional_trailing_zeroes_map_to_digit_glyphs);
    RUN_TEST(test_short_negative_uses_center_segment_before_number);
    RUN_TEST(test_full_width_negative_uses_external_minus_segment);
    RUN_TEST(test_error_state_renders_err_on_rightmost_digits);
    RUN_TEST(test_brightness_feedback_shows_b_and_selected_level);
    RUN_TEST(test_test_point_selects_exactly_one_matrix_led);
    return UNITY_END();
}