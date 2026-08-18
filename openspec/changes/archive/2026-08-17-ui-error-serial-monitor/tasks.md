## 1. UI Surface & Design Update

- [x] 1.1 Update `src/RADIOKIT.h` with `serial_monitor_1` and `serial_monitor_2` configured as hidden by default.
- [x] 1.2 Update `docs/radiokit-rc-ui-design.json` from the latest app design.

## 2. UI Error Logger Bridge

- [x] 2.1 Create `common/UiLogger.h` with boot warning buffering, multi-monitor broadcast, dynamic unhide, and auto-hide/clear support.
- [x] 2.2 Route `ConfigParser.h` warnings and errors to `UiLogger` for display in the UI monitor widgets only.
- [x] 2.3 Route battery and runtime fault notifications to `UiLogger`.

## 3. Integration & Lifecycle Wiring

- [x] 3.1 Initialize `UiLogger` in `src/main.cpp` and flush boot-time warnings after `initRadioKit()`.
- [x] 3.2 Update `reloadConfigs()` in `src/main.cpp` to auto-clear/hide monitors on successful reload and log on reload failure.

## 4. Verification & Testing

- [x] 4.1 Build firmware for `TRACKLINK_V3` and `MIKRO_V2` to verify clean compilation.
- [x] 4.2 Run test suites / host tests to ensure zero regressions.
