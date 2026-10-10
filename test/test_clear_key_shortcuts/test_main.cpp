#include <unity.h>

#include "ClearKeyShortcuts.h"

void setUp() {}
void tearDown() {}

void test_plain_clear_is_deferred_until_release() {
    ClearKeyShortcuts shortcuts;
    TEST_ASSERT_EQUAL(ClearKeyAction::Ignore, shortcuts.keyPressed(KeyInput::Clear));
    TEST_ASSERT_EQUAL(ClearKeyAction::ClearCalculator, shortcuts.clearKeyReleased());
}

void test_clear_plus_increases_brightness_and_does_not_clear() {
    ClearKeyShortcuts shortcuts;
    TEST_ASSERT_EQUAL(ClearKeyAction::Ignore, shortcuts.keyPressed(KeyInput::Clear));
    TEST_ASSERT_EQUAL(ClearKeyAction::IncreaseBrightness, shortcuts.keyPressed(KeyInput::Add));
    TEST_ASSERT_EQUAL(ClearKeyAction::IncreaseBrightness, shortcuts.keyPressed(KeyInput::Add));
    TEST_ASSERT_EQUAL(ClearKeyAction::RestoreCalculatorDisplay, shortcuts.clearKeyReleased());
}

void test_clear_minus_decreases_brightness() {
    ClearKeyShortcuts shortcuts;
    shortcuts.keyPressed(KeyInput::Clear);
    TEST_ASSERT_EQUAL(ClearKeyAction::DecreaseBrightness, shortcuts.keyPressed(KeyInput::Subtract));
    TEST_ASSERT_EQUAL(ClearKeyAction::RestoreCalculatorDisplay, shortcuts.clearKeyReleased());
}

void test_clear_equals_starts_test_and_release_stops_it() {
    ClearKeyShortcuts shortcuts;
    shortcuts.keyPressed(KeyInput::Clear);
    TEST_ASSERT_EQUAL(ClearKeyAction::StartDisplayTest, shortcuts.keyPressed(KeyInput::Equals));
    TEST_ASSERT_EQUAL(ClearKeyAction::Ignore, shortcuts.keyPressed(KeyInput::Digit5));
    TEST_ASSERT_EQUAL(ClearKeyAction::StopDisplayTest, shortcuts.clearKeyReleased());
}

void test_normal_keys_forward_and_other_keys_while_clear_held_are_ignored() {
    ClearKeyShortcuts shortcuts;
    TEST_ASSERT_EQUAL(ClearKeyAction::ForwardKey, shortcuts.keyPressed(KeyInput::Digit7));
    shortcuts.keyPressed(KeyInput::Clear);
    TEST_ASSERT_EQUAL(ClearKeyAction::Ignore, shortcuts.keyPressed(KeyInput::Digit7));
    TEST_ASSERT_EQUAL(ClearKeyAction::ClearCalculator, shortcuts.clearKeyReleased());
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_plain_clear_is_deferred_until_release);
    RUN_TEST(test_clear_plus_increases_brightness_and_does_not_clear);
    RUN_TEST(test_clear_minus_decreases_brightness);
    RUN_TEST(test_clear_equals_starts_test_and_release_stops_it);
    RUN_TEST(test_normal_keys_forward_and_other_keys_while_clear_held_are_ignored);
    return UNITY_END();
}