## Context

Board headers in `boards/` define physical hardware mappings for various ESP32 boards (TRACKLINK_V2, TRACKLINK_V3, MIKRO_V2, GTRACK). Different circuits use inverted logic levels:
- Status LEDs connected to VCC (active LOW).
- Voltage regulator enables (e.g. buck converter `SERVO_ENABLE` on GTRACK) where pulling LOW disables and HIGH enables, or vice versa.
- Motor driver enable lines that are active-low.
- Charge sense status flags that indicate charging when LOW or HIGH depending on the battery charger IC.

Currently, `BoardTypes.h` defines pins as `uint8_t` and `NO_PIN = 0xFF`. Inversion cannot be expressed in the board definition itself, forcing peripheral setup to assume active-high polarity everywhere.

## Goals / Non-Goals

**Goals:**
- Allow any board pin to be declared as negative (e.g., `-42`) to indicate inverted/active-low hardware polarity.
- Use `int8_t` for pin types, keeping a compact 1-byte storage footprint.
- Redefine `NO_PIN = 127` (`0x7F`) so that negative values (`-1` .. `-48`) do not collide with `NO_PIN`.
- Provide `Board::isPinInverted()`, `Board::toGpio()`, and `Board::isValid()` helpers.
- Propagate inversion into `HardwareInit` for `EasyLED`, direct digital writes, and input reads (`CHARGE_SENS`).
- Keep code clean, backwards-compatible, and zero runtime overhead.

**Non-Goals:**
- Dynamic inversion configuration via JSON at runtime (inversion is a hardware property declared in board definitions).
- Inverting GPIO 0 (GPIO 0 is a strapping pin and not used as an inverted output on our boards).

## Decisions

### 1. Data Type: `int8_t` with `NO_PIN = 127`
- **Choice**: `using Pin_t = int8_t; static constexpr int8_t NO_PIN = 127;`
- **Rationale**:
  - Valid ESP32-S3 GPIOs are $0..48$.
  - Inverted pins are $-1..-48$.
  - `NO_PIN = 127` is positive, never collides with any negative GPIO or valid positive GPIO.
  - Checking inversion is a single sign comparison: `pin < 0`.
- **Alternatives Considered**:
  - `NO_PIN = -128`: Requires an extra check (`pin < 0 && pin != NO_PIN`) and negating `-128` overflows signed 8-bit integers.
  - `int16_t`: Doubles pin memory usage in tables with no practical benefit for ESP32 GPIO ranges.

### 2. Resolution & Conversion Helpers on `Board`
- **Helpers**:
  ```cpp
  static inline constexpr bool isPinInverted(int8_t pin) { return pin < 0; }
  static inline constexpr bool isValid(int8_t pin) { return pin != NO_PIN; }
  static inline constexpr uint8_t toGpio(int8_t pin) {
      if (pin == NO_PIN) return 0xFF; // returns 0xFF for unsigned legacy checks
      return static_cast<uint8_t>(pin < 0 ? -pin : pin);
  }
  ```
- **Rationale**: Consumers (like `HardwareInit` and `ConfigParser`) can easily extract physical GPIO numbers for SDK APIs (`pinMode`, `digitalWrite`, `EasyLED::begin`) while cleanly checking polarity.

### 3. Peripheral Integration
- **`EasyLED`**: Pass `Board::isPinInverted(pin)` to `EasyKit::LEDConfig::invertOutput` during `initGroup` and ditch/turn light setup.
- **Digital outputs (`POWER_ENABLE`, `SERVO_ENABLE`, `PUMP_ENABLE`)**: Use `digitalWrite(gpio, (active ? HIGH : LOW) ^ isPinInverted(pin))`.
- **Digital inputs (`CHARGE_SENS`)**: Detect active state via `(digitalRead(gpio) == LOW) ^ isPinInverted(pin)`.

### 4. Updating Hardcoded `0xFF` Checks
- Replace legacy comparisons against literal `0xFF` with `NO_PIN` or `Board::isValid(pin)` to avoid type mismatch warnings.

## Risks / Trade-offs

- **[Risk] Existing comparisons against `0xFF` failing with `int8_t`**:
  - `(int8_t)0xFF` is `-1`. If code compares an `int8_t` pin against `0xFF` directly without casting, `127 != 0xFF`.
  - **Mitigation**: Update all board-pin checks to compare against `NO_PIN` (or use `Board::isValid()`), and ensure `Board::toGpio(NO_PIN)` returns `0xFF` when an unsigned `uint8_t` return is requested.
- **[Risk] Pin 0 inversion limitation**:
  - In integer math, `-0 == 0`.
  - **Mitigation**: As verified, GPIO 0 is strapping/BOOT and never an inverted output on our boards.
