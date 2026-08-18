## Context

The RadioKit companion app now features dedicated serial monitor widgets on both the Truck control page (`serial_monitor_1`) and the Loco control page (`serial_monitor_2`). In normal operation, these widgets should remain hidden. When configuration warnings or runtime errors arise, the widgets should be revealed and populated with diagnostic error messages.

## Goals / Non-Goals

**Goals:**
- Update `src/RADIOKIT.h` to declare both `serial_monitor_1` and `serial_monitor_2` as hidden by default.
- Create a lightweight `UiLogger` in `common/UiLogger.h` to coordinate error messaging and widget visibility across pages.
- Buffer boot-time warnings prior to RadioKit initialization and flush them once RadioKit starts.
- Direct error messages to the UI serial monitor widgets only.
- Implement automatic widget hiding when a clean configuration is hot-reloaded.

**Non-Goals:**
- Replace USB CDC Serial debug output used for flashing / host dev tooling.
- Implement complex scrolling history or interactive terminal input over the serial monitor widgets.

## Decisions

### 1. Dedicated `UiLogger` Header in `common/UiLogger.h`
- **Decision**: Encapsulate UI error handling in a static/inline class or namespace `UiLogger`.
- **Rationale**: Keeps `ConfigParser.h`, `main.cpp`, and `VehicleController.h` decoupled from widget internals while providing a clean API: `UiLogger::init()`, `UiLogger::logWarn(...)`, `UiLogger::logError(...)`, `UiLogger::clear()`, and `UiLogger::flushBootQueue()`.
- **Alternatives Considered**: Inlining widget logic directly inside `ConfigParser.h` (rejected due to circular header dependencies between `ConfigParser.h` and `RADIOKIT.h`).

### 2. Multi-Page Synchronization
- **Decision**: Whenever `UiLogger` unhides or prints to the monitor, it addresses both `serial_monitor_1` (Page 0) and `serial_monitor_2` (Page 1) so the error is visible regardless of active vehicle page.

### 3. Boot-time Warning Buffer
- **Decision**: Provide a fixed static circular/linear buffer (e.g. 512 bytes) that captures warnings before `initRadioKit()` runs.
- **Rationale**: `setup()` loads configs before BLE and RadioKit are started. Buffering guarantees that boot warnings are not lost.

### 4. Auto-Hide on Hot-Reload Recovery
- **Decision**: `reloadConfigs()` in `main.cpp` calls `UiLogger::clear()` upon successful reload of both hardware and vehicle configs with zero warnings.

## Risks / Trade-offs

- **[Risk] Widget text length limit** → `RADIOKIT_TEXT_LEN` in `Text.h` is 32 bytes per chunk. `UiLogger` streams messages cleanly so multiple lines / chunks are received intact by the Flutter app.
- **[Risk] Multiple reloads with errors** → `UiLogger` re-asserts unhidden state and streams fresh error details on every failed attempt.
