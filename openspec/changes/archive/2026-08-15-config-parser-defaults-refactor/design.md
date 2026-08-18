# Design: ConfigParser Defaults Refactor

## Architecture

`HardwareConfig` struct fields in `common/Config.h` are default-initialized upon construction.
When `ConfigParser::parseHardwareConfig(docObj, config)` is invoked on a `HardwareConfig` instance:

- If a key exists in JSON, ArduinoJson evaluates the first or second operand.
- If a key is missing from JSON, ArduinoJson falls back to the third operand (`config.<field>`), which maintains its exact default from `common/Config.h`.

```
  common/Config.h                                    common/ConfigParser.h
┌───────────────────────────┐                       ┌─────────────────────────────────────────────────────────────┐
│ struct Power {            │                       │ config.power.bootLatchS = pwrObj["boot_latch_s"]            │
│   float bootLatchS = 1.0f;│──────────────────────►│                         | pwrObj["BOOT_LATCH_S"]            │
│ }                         │                       │                         | config.power.bootLatchS;          │
└───────────────────────────┘                       └─────────────────────────────────────────────────────────────┘
```
