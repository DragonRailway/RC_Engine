# references/ — PWM Library Reference

Reference copies of ESP32 PWM-related libraries, kept for study. **None of these are linked into the build** — the project's PWM layer is `ESP32_EasyKit`, vendored at `lib/ESP32_EasyKit`.

The project builds on the **pioarduino platform (Arduino-ESP32 3.x / ESP-IDF 5.x)**. This matters because ESP-IDF 5.x **removed the legacy MCPWM driver** from the standard include path (`driver/mcpwm.h` is now `driver/deprecated/driver/mcpwm.h`), while the new API lives at `driver/mcpwm_prelude.h`.

## Buildability on this toolchain

| Library | MCPWM API used | Builds on Arduino 3.x? | Notes |
|---|---|---|---|
| `Rc_Engine_Sound_ESP32` | n/a (sound engine) | — | Upstream reference for the sound engine; not a PWM library |
| `ESP32Servo` | legacy `mcpwm_config_t` / `MCPWM_OPR_A` (`driver/mcpwm.h`) | ❌ | Requires migration to `mcpwm_prelude.h` or the deprecated include path to build |
| `ESP32MCServo` | legacy `driver/mcpwm.h` | ❌ | Self-declares "Arduino 3.x / IDF 5.x support pending" |
| `ESP32_MCPWM` | legacy `mcpwm_gpio_init` (`driver/mcpwm.h`) | ❌ | Requires migration to `mcpwm_prelude.h` |

`ESP32_PWM_Fusion` (pristine Dlloydev "ESP32 ESP32S2 AnalogWrite", LEDC-only) was removed from `references/` — the project's dependency by that name was actually the rewritten `ESP32_EasyKit` (MCPWM + LEDC), now vendored at `lib/ESP32_EasyKit`. See `openspec/changes/adopt-easykit-pwm/` for the full history and design.

## Why the project uses ESP32_EasyKit

- The only library in play written against `driver/mcpwm_prelude.h` (the current API)
- MCPWM for motor + servo (12 operator slots on S3), LEDC for lights (≤ 8 channels)
- Thread-safe allocators (LEDCManager, MCPWMManager) prevent channel exhaustion
- Vendored in-repo so fixes ship with the project
