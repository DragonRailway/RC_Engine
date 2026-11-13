## Context

RC Engine runs on ESP32-S3 boards (TRACKLINK_V2, TRACKLINK_V3, MIKRO_V2). These boards include a battery power latching circuit (`POWER_ENABLE`), a momentary power button (`POWER_BUTTON`), and a charge status input from the onboard LiPo charger IC (`CHARGE_SENS`, active LOW while charging).

When plugged into USB:
1. 5V VBUS powers the 3.3V LDO through a diode, keeping the ESP32 energized regardless of `POWER_ENABLE`.
2. The current firmware enters full operational mode at boot (advertising RadioKit BLE, initializing audio DSP, and running the 50Hz vehicle control task).
3. If no BLE companion app or transmitter is connected, `VehicleController` hits its disconnect timeout (~35–60s) and calls `HardwareInit::powerOff()`.
4. `HardwareInit::powerOff()` invokes `esp_deep_sleep(0)`. In ESP-IDF, `esp_deep_sleep(uint64_t time_in_us)` arms an RTC timer for `time_in_us` microseconds. Passing `0` sets an immediate timer wakeup, triggering a deep-sleep system reset and causing an infinite bootloop every 35–60 seconds.
5. In addition, `VehicleController` only checked `digitalRead(POWER::CHARGE_SENS) == LOW`. When the battery reaches full charge (or no battery is attached), `CHARGE_SENS` goes HIGH. The vehicle misidentifies this as an abandoned vehicle and shuts down.

## Goals / Non-Goals

**Goals:**
- **Minimal Charging Mode**: Ensure the vehicle enters a low-activity charging state when plugged into USB:
  - BLE modem advertising and RadioKit stack are OFF.
  - Audio task and I2S DAC output are OFF.
  - Motors and steering servos are safely parked/sleeping.
  - Disconnect auto-shutdown is completely inhibited.
- **Heartbeat & Solid 20% Indicators**:
  - During active charging (`CHARGE_SENS == LOW`): Double-pulse heartbeat blink at 100% of configured `brightness_max`.
  - When charge finishes (`CHARGE_SENS == HIGH` while on USB / docked): Steady solid glow at 20% of `brightness_max`.
- **Power Button "Switch On"**: Pressing the physical power button while in charging mode immediately launches RadioKit BLE, starts audio synthesis, and transitions to the active driving state (`ON`).
- **USB Unplug Clean Shutdown**: Unplugging USB while in charging mode (without having switched ON) powers off the board.
- **Fix `HardwareInit::powerOff()`**: Eliminate `esp_deep_sleep(0)` and prevent reset loops when USB remains connected.

**Non-Goals:**
- Modifying vehicle driving physics, motor control algorithms, or RadioKit remote protocol packets.
- Altering physical PCB circuitry.

## Decisions

### Decision 1: Quiesced Active State for Charging Animation
- **Choice**: Keep the ESP32 in active CPU mode with radios and DSP stopped, rather than Light-Sleep or Deep-Sleep.
- **Rationale**: The vehicle is receiving wall/USB 5V power (500mA–2000mA). The ESP32 with radios off consumes only ~20–30mA. Active mode allows the ESP32 LEDC hardware PWM peripheral and `EasyLED` to smoothly drive the configurable charging LED (especially for fading or 20% duty) without the complex clock-gating constraints of light sleep.
- **Alternatives Considered**: Light sleep with periodic 20ms wakeup. Rejected because APB clock gating interrupts LEDC PWM channels and introduces visual flickering.

### Decision 2: Deferred Subsystem Boot in `main.cpp`
- **Choice**: Split boot flow in `main.cpp`:
  - `HardwareInit::latchPower()` already detects USB boot (`digitalRead(POWER_BUTTON) == LOW`).
  - If booted via USB into charging mode, `ConfigParser` mounts LittleFS and loads hardware/vehicle configs, but `initRadioKit()` and `engine.begin()` are deferred.
  - Only `controlTask` runs, executing minimal charging LED animation and button polling.
  - When `HardwareInit::consumeButtonClicked()` detects a press, it executes the deferred `initRadioKit()` and `engine.begin()`, transitioning the board to `ON`.
- **Alternatives Considered**: Initialize everything and stop them later in `VehicleController`. Rejected because it briefly blares BLE advertising and pops audio amplifiers before shutting down.

### Decision 3: Heartbeat Animation Pattern
- **Choice**: Implement a deterministic 1000ms repeating cycle:
  - `t = millis() % 1000`:
    - `0 <= t < 120`: ON (100% `brightness_max`)
    - `120 <= t < 240`: OFF
    - `240 <= t < 360`: ON (100% `brightness_max`)
    - `360 <= t < 1000`: OFF
  - When charging completes: switch to solid `(brightness_max * 20) / 100`.

### Decision 4: Safe USB Power Off in `HardwareInit::powerOff()`
- **Choice**:
  - On battery, driving `POWER_ENABLE = LOW` drops power immediately.
  - On USB, driving `POWER_ENABLE = LOW` disables motor/external peripheral power rails.
  - Replace `esp_deep_sleep(0)` with `esp_deep_sleep_start()` with only the power button configured as an ext0/ext1 or deep-sleep GPIO wakeup source (or halt loop if waking via button is preferred), ensuring no 0µs timer wakeup resets the MCU.

## Risks / Trade-offs

- **[Risk]**: Connecting via USB for development / flashing might boot into charging mode without BLE.
  - **Mitigation**: Clicking the power button immediately boots into full mode with BLE enabled. Also, USB CDC serial communication remains fully active so `scripts/build_fs.py` and `pio run -t upload` are completely unaffected.
- **[Risk]**: Distinguishing USB disconnect on battery.
  - **Mitigation**: When unplugged on battery, `CHARGE_SENS` is HIGH and battery voltage is in the normal 1S range ($\le 4.20\text{V}$). The disconnect timer runs normally to protect the battery.
