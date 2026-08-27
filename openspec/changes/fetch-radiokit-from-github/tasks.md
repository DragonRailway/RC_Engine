## 1. Pre-Build Fetch Script

- [x] 1.1 Create `scripts/fetch_radiokit.py` to clone/sparse-checkout `rk-arduino` from GitHub `main` branch into `lib/rk-arduino`

## 2. PlatformIO and Git Configuration

- [x] 2.1 Update `platformio.ini` to add `pre:scripts/fetch_radiokit.py` to `extra_scripts` and remove `symlink://../RadioKit/rk-arduino` from `lib_deps`
- [x] 2.2 Add `/lib/rk-arduino/` to `.gitignore`

## 3. Verification

- [x] 3.1 Test building `TRACKLINK_V3` and `MIKRO_V2` environments with dynamic fetch
- [x] 3.2 Verify clean build when `lib/rk-arduino` is removed
