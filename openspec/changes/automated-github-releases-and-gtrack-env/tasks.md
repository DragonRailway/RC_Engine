## 1. G-Track Board Environment Setup

- [x] 1.1 Add `[env:GTRACK]` to `platformio.ini` with compiler flags and include paths
- [x] 1.2 Update `src/main.cpp` to resolve `/hardware-GTRACK.json` and default name `"GTRACK"`
- [x] 1.3 Create `configs/hardware_configs/hardware-GTRACK.json` for G-Track board

## 2. Release Packaging Utility

- [x] 2.1 Create `scripts/package_release.py` to parse `platformio.ini`, compile all envs, package `factory.bin` / `ota.bin`, and zip `configs/`

## 3. GitHub Actions Release Workflow

- [x] 3.1 Create `.github/workflows/release.yml` with tag (`v*`) and `workflow_dispatch` triggers, artifact building, and GitHub release publishing

## 4. Verification

- [x] 4.1 Verify local release packaging runs and generates all `.bin` files and `RC_Engine-configs.zip`
- [x] 4.2 Validate `hardware-GTRACK.json` schema and compile `GTRACK` board target
