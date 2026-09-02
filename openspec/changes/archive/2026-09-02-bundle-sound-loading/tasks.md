## 1. ConfigParser Sound Loader Updates

- [x] 1.1 Update `ConfigParser::loadSounds()` in `common/ConfigParser.cpp` to resolve `/vehicle_configs/<soundSet>/sounds/<slot>.pcm` and `/vehicle_config/<soundSet>/sounds/<slot>.pcm` as top-priority candidates
- [x] 1.2 Add common preset fallback paths (`/vehicle_configs/common/<type>/<slot>.pcm`, `/sounds/common/<type>/<slot>.pcm`)
- [x] 1.3 Remove dead legacy lookup arrays (`genericNames`) and obsolete flat-file lookups

## 2. Build & Verification

- [x] 2.1 Build firmware for `TRACKLINK_V3` (`pio run -e TRACKLINK_V3`)
- [x] 2.2 Upload firmware to connected TRACKLINK_V3 board (`pio run -e TRACKLINK_V3 -t upload`)
- [x] 2.3 Verify serial boot log to confirm sound assets are successfully loaded and AudioOutput starts
- [x] 2.4 Verify engine and horn/bell sounds trigger via RadioKit Remote API
