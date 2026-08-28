## Context

The `RC_Engine` project relies on `RadioKit` for its Bluetooth LE control surface and telemetry transport. Currently, `platformio.ini` uses `symlink://../RadioKit/rk-arduino`, requiring the sibling repository `RadioKit` to exist locally on the same parent path. In the GitHub remote repository `Radio-Kit/RadioKit`, the Arduino library code is located at the subdirectory `rk-arduino/`.

## Goals / Non-Goals

**Goals:**
- Dynamically clone/fetch the `rk-arduino` subdirectory from `https://github.com/Radio-Kit/RadioKit.git` (branch `main`) into `lib/rk-arduino/` on-demand prior to building.
- Keep the build self-contained on any fresh environment without requiring a manual clone of `../RadioKit`.
- Remove the local symlink from `platformio.ini` while allowing PlatformIO to automatically compile `lib/rk-arduino/`.
- Ensure `lib/rk-arduino/` is ignored in Git.

**Non-Goals:**
- Creating a separate GitHub repository for `rk-arduino` (uses the existing `Radio-Kit/RadioKit` repo directly).
- Modifying RadioKit library source code.

## Decisions

### Decision 1: Pre-Build Hook Script (`scripts/fetch_radiokit.py`)
Run a Python script registered in `platformio.ini` under `extra_scripts` (`pre:scripts/fetch_radiokit.py`).
- **Rationale**: PlatformIO's build pipeline runs `extra_scripts` with `pre:` before dependency resolution and compilation. This guarantees the library files exist before compilation begins.
- **Alternatives considered**:
  - *Git Submodule*: Adds repository submodule state tracking and requires `git submodule update --init`.
  - *PlatformIO Git URL in `lib_deps`*: Fails because `rk-arduino` is in a subdirectory of the monorepo, not the repo root.

### Decision 2: Target Directory `lib/rk-arduino/`
Fetch the library contents directly into `lib/rk-arduino/` in the project root.
- **Rationale**: PlatformIO treats directories under `lib/` as project-level libraries, making headers available to `src/` and `common/` seamlessly without per-environment include path acrobatics.
- **Alternatives considered**:
  - *Fetching into `.pio/libdeps/<env>/rk-arduino`*: Requires cloning per-environment, duplicating storage and fetch time.

### Decision 3: Efficient Sparse Shallow Clone
The script uses `git clone --depth 1 --filter=blob:none --sparse https://github.com/Radio-Kit/RadioKit.git -b main` in a temporary directory, sets sparse checkout for `rk-arduino`, and moves `rk-arduino` to `lib/rk-arduino`.
- **Rationale**: Downloads only the small `rk-arduino` folder without downloading historical commits or large application assets in `radiokit-app`.

## Risks / Trade-offs

- **[Risk] Network required for initial build** → Mitigation: Once `lib/rk-arduino` is fetched, subsequent builds run offline without re-fetching unless `lib/rk-arduino` is removed.
- **[Risk] Subdirectory drift** → Mitigation: Script targets `main` branch explicitly and verifies `RadioKitLib.h` exists upon extraction.
