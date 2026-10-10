#include <Arduino.h>

#include "CalculatorCore.h"
#include "ClearKeyShortcuts.h"
#include "Tm1638Display.h"

// Row outputs (driven low one at a time, idle high)
const uint8_t ROW_PINS[] = {2, 3, 4, 5, 6};
// Column inputs (externally pulled up, read low when a key is pressed)
const uint8_t COL_PINS[] = {7, 8, 9, 10};
const uint8_t NUM_ROWS = sizeof(ROW_PINS) / sizeof(ROW_PINS[0]);
const uint8_t NUM_COLS = sizeof(COL_PINS) / sizeof(COL_PINS[0]);

bool keyState[5][4];
constexpr uint8_t kClearRow = 0;
constexpr uint8_t kClearCol = 3;
constexpr uint8_t kDisplayTestStepCount = Tm1638MappedFrame::kTestStepCount;
constexpr uint32_t kDisplayTestStepMs = 20;

// Maps each physical [row][col] position to its logical key. See README.md
// for the keypad diagram this must stay in sync with.
const KeyInput KEY_MAP[5][4] = {
    {KeyInput::MemRecallClear, KeyInput::MemAdd, KeyInput::MemSubtract, KeyInput::Clear},
    {KeyInput::Digit7, KeyInput::Digit8, KeyInput::Digit9, KeyInput::Divide},
    {KeyInput::Digit4, KeyInput::Digit5, KeyInput::Digit6, KeyInput::Multiply},
    {KeyInput::Digit1, KeyInput::Digit2, KeyInput::Digit3, KeyInput::Subtract},
    {KeyInput::Digit0, KeyInput::Decimal, KeyInput::Equals, KeyInput::Add},
};

CalculatorCore calculator(10);
Tm1638Display display(3);
ClearKeyShortcuts clearKeyShortcuts;
bool displayTestActive = false;
uint8_t displayTestPoint = 0;
uint32_t displayTestLastStep = 0;

void printDisplay() {
    DisplayState state = calculator.getDisplay();
    display.render(state);
    if (state.error) {
        Serial.println("Error");
        return;
    }

    String out;
    if (state.negative) {
        out += '-';
    }
    for (uint8_t i = 0; i < state.digitCount; i++) {
        if (state.hasDecimalPoint && i == state.decimalPointPosition) {
            out += '.';
        }
        out += static_cast<char>('0' + state.digits[i]);
    }
    if (state.hasDecimalPoint && state.decimalPointPosition == state.digitCount) {
        out += '.';
    }
    Serial.println(out);
}

void handleClearKeyAction(ClearKeyAction action, KeyInput key) {
    switch (action) {
        case ClearKeyAction::ForwardKey:
            calculator.handleKey(key);
            printDisplay();
            break;
        case ClearKeyAction::ClearCalculator:
            calculator.handleKey(KeyInput::Clear);
            printDisplay();
            break;
        case ClearKeyAction::IncreaseBrightness: {
            uint8_t level = display.brightness();
            if (level < 7) {
                level++;
            }
            display.setBrightness(level);
            display.renderBrightness();
            break;
        }
        case ClearKeyAction::DecreaseBrightness: {
            uint8_t level = display.brightness();
            if (level > 0) {
                level--;
            }
            display.setBrightness(level);
            display.renderBrightness();
            break;
        }
        case ClearKeyAction::StartDisplayTest:
            displayTestActive = true;
            displayTestPoint = 0;
            displayTestLastStep = millis();
            display.renderTestStep(displayTestPoint);
            break;
        case ClearKeyAction::StopDisplayTest:
            displayTestActive = false;
            display.render(calculator.getDisplay());
            break;
        case ClearKeyAction::RestoreCalculatorDisplay:
            display.render(calculator.getDisplay());
            break;
        case ClearKeyAction::Ignore:
            break;
    }
}

void updateDisplayTest() {
    if (!displayTestActive || millis() - displayTestLastStep < kDisplayTestStepMs) {
        return;
    }

    displayTestLastStep = millis();
    displayTestPoint++;
    if (displayTestPoint >= kDisplayTestStepCount) {
        displayTestPoint = 0;
    }
    display.renderTestStep(displayTestPoint);
}

void setup() {
    // Initialize Serial communications
    Serial.begin(115200);

    display.begin();
    display.render(calculator.getDisplay());

    // Initialize onboard LED
    pinMode(LED_BUILTIN, OUTPUT);

    for (uint8_t r = 0; r < NUM_ROWS; r++) {
        pinMode(ROW_PINS[r], INPUT);
    }

    // External pull-ups are provided on the column lines.
    for (uint8_t c = 0; c < NUM_COLS; c++) {
        pinMode(COL_PINS[c], INPUT);
    }
}

void scanKeypad() {
    for (uint8_t r = 0; r < NUM_ROWS; r++) {
        pinMode(ROW_PINS[r], OUTPUT);
        digitalWrite(ROW_PINS[r], LOW);
        delayMicroseconds(10); // let the driven line settle before sampling

        for (uint8_t c = 0; c < NUM_COLS; c++) {
            bool pressed = (digitalRead(COL_PINS[c]) == LOW);
            if (pressed != keyState[r][c]) {
                keyState[r][c] = pressed;
                Serial.printf("Key [row %u, col %u] %s\n", r, c, pressed ? "pressed" : "released");
                if (pressed) {
                    const KeyInput key = KEY_MAP[r][c];
                    handleClearKeyAction(clearKeyShortcuts.keyPressed(key), key);
                } else if (r == kClearRow && c == kClearCol) {
                    handleClearKeyAction(clearKeyShortcuts.clearKeyReleased(), KeyInput::Clear);
                }
            }
        }

        // Briefly drive the row high to recharge the column lines (otherwise
        // C held in column 3 leaves the column low and rows scanned next read
        // false presses of -, which rolls brightness down). Every other row is
        // high-Z at this point, so this cannot fight a second closed key in the
        // same column.
        digitalWrite(ROW_PINS[r], HIGH);
        delayMicroseconds(10);
        pinMode(ROW_PINS[r], INPUT);
    }
}

void loop() {
    scanKeypad();
    updateDisplayTest();
    delay(10);
}
