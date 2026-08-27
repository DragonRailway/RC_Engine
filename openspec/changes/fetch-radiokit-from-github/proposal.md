## Why

Currently, `RC_Engine` relies on a local filesystem symlink (`symlink://../RadioKit/rk-arduino`) to include the RadioKit embedded library. This breaks clean clones on systems or CI pipelines where the sibling `../RadioKit` repository is not present at the expected path. We need an automated pre-build fetch mechanism that pulls `rk-arduino` directly from GitHub (`https://github.com/Radio-Kit/RadioKit/tree/main/rk-arduino`) so builds are completely self-contained.

## What Changes

- Add a pre-build Python hook `scripts/fetch_radiokit.py` that shallow-clones (`--depth 1 --sparse`) the `rk-arduino` subdirectory from the GitHub repository `Radio-Kit/RadioKit` (branch `main`) into `lib/rk-arduino/`.
- Update `platformio.ini` to register `pre:scripts/fetch_radiokit.py` in `extra_scripts` and remove the local `symlink://../RadioKit/rk-arduino` dependency from `lib_deps`.
- Update `.gitignore` to ignore `/lib/rk-arduino/` so dynamically fetched library files are not checked into the repository.
- Update `AGENTS.md` and related docs to reflect the remote library fetch workflow.

## Capabilities

### New Capabilities
<!-- None -->

### Modified Capabilities
- `radiokit-ble-control`: Updates the RadioKit library resolution requirement from a local symlink to an automated pre-build fetch of `rk-arduino` from the GitHub repository (`https://github.com/Radio-Kit/RadioKit.git` branch `main`) into `lib/rk-arduino`.

## Impact

- **Build System**: `pio run` on any environment (`TRACKLINK_V3`, `MIKRO_V2`) will automatically fetch `lib/rk-arduino` if not present.
- **Dependencies**: Eliminates machine-local filesystem path coupling to `../RadioKit`.
- **Git Repo**: `lib/rk-arduino/` is gitignored.
