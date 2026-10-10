# Calculator

A physical 4-function calculator built on a Raspberry Pi Pico (PlatformIO,
Arduino framework, earlephilhower core).

## Keypad layout

4 columns x 5 rows (20 keys):

| Col1 | Col2 | Col3 | Col4 |
|------|------|------|------|
| MRC  | M+   | M-   | C    |
| 7    | 8    | 9    | ÷    |
| 4    | 5    | 6    | ×    |
| 1    | 2    | 3    | −    |
| 0    | .    | =    | +    |

- **MRC**: tap-tap toggle. First press recalls the memory value onto the
  display. An immediate second press (no other key pressed in between) clears
  the memory to 0. Any other key press resets the toggle back to "recall" for
  the next MRC press.
- **M+ / M-**: adds/subtracts the current display value to/from memory.
- **C**: full reset (current entry, pending operation, error state). Does
  **not** clear memory.
- No CE, sign-change (+/-), or percent (%) keys in this version.

## CalculatorCore API

`CalculatorCore` (see [src/CalculatorCore.h](src/CalculatorCore.h)) is a
self-contained, hardware-independent calculator engine. `main.cpp` creates one
instance at startup and interacts with it through two methods only:

- `void handleKey(KeyInput key)` — call once per key press.
- `DisplayState getDisplay() const` — returns what should currently be shown.

Behavior notes:
- Display digit count is configurable at construction (`CalculatorCore(uint8_t
  numDigits = 8)`).
- The application configures ten digits to match the TM1638 display.
- Internal arithmetic uses `double`.
- Operators chain like a standard calculator (`2 + 3 + 4 =` computes `2+3=5`
  immediately, then `5+4=9`).
- Repeated `=` presses repeat the last operator/operand against the current
  value.
- During entry, the decimal point and typed fractional zeros remain visible;
  calculated results normalize trailing fractional zeros.
- Typing more digits than the configured width allows is silently ignored.
- Divide-by-zero or results too large to display set an error state; all keys
  except `C` are ignored until cleared.

## TM1638 Display

`Tm1638Display` drives the custom 10-digit, 8-segment matrix using SPI0:

- STB: GP17
- CLK: GP18
- DIO: GP19

The pure display mapper transposes digit glyphs into the eight GRID scan rows
and ten SEG output columns. Negative values use the center segment in the blank
position immediately before the number; when all ten digit positions are in
use, the rightmost `h` channel drives the separate minus segment instead. A
decimal point is not rendered on the rightmost digit. Brightness defaults to
level 3 and can be changed from 0 to 7.

Hold **C** and tap the arithmetic **+** or **−** key to change brightness one
level at a time; `b0` through `b7` remains on the display until **C** is
released, when the calculator display is restored. Brightness resets to its
default after power cycling. Hold **C** and tap **=** to sweep each of the 80
LED positions once, for 80 ms per position. It tests all eight GRID segments
on each digit from left to right before advancing to the next digit. Releasing
**C** stops the sweep immediately and restores the calculator display. Pressing
and releasing **C** alone clears the calculator on release.

## Portability

`CalculatorCore.h`/`.cpp` intentionally avoid any Arduino/hardware
dependencies and use only a conservative, portable C++ subset (no STL
containers, no exceptions, no dynamic allocation) so the same code compiles
and runs both on the Pico and natively on a development workstation.

## Testing

Calculator, shortcut-state, and display-mapping unit tests run natively on the
workstation (no Pico required):

```
pio test -e native
```

The `pico` environment remains the default build target for the actual
hardware:

```
pio run
```
