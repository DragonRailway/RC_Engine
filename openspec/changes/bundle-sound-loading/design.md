## Context

On LittleFS, vehicle profiles and sounds are stored in self-contained bundles:
```
/vehicle_configs/<SoundSet>/
  ├── vehicle.json
  └── sounds/
      ├── <slot>.pcm
      ...
```
Or alternatively under `/sounds/vehicles/<SoundSet>/<slot>.pcm`.

Previously, `ConfigParser::loadSounds()` checked a series of deprecated flat paths (e.g. `/sounds/<soundSet>-<slot>.pcm`) and legacy hardcoded arrays (`genericNames`), but omitted `/vehicle_configs/<soundSet>/sounds/<slot>.pcm`. This caused sound loading to fail whenever vehicle bundles were uploaded or extracted in standard bundle directory format.

With the decision to drop legacy backward compatibility, sound resolution can be simplified and aligned with the clean bundle model.

## Goals / Non-Goals

**Goals:**
- Update `ConfigParser::loadSounds()` to search the following prioritized candidate paths for each sound slot:
  1. `/vehicle_configs/<soundSet>/sounds/<slot>.pcm`
  2. `/vehicle_config/<soundSet>/sounds/<slot>.pcm`
  3. `/sounds/vehicles/<soundSet>/<slot>.pcm`
  4. `/vehicle_configs/common/<type>/<slot>.pcm`
  5. `/sounds/common/<type>/<slot>.pcm`
  6. `/sounds/presets/<type>/<slot>.pcm`
- Clean up unused legacy lookup arrays (`genericNames`) and obsolete flat-file lookups.
- Validate that all required sounds (e.g., bell, horn, idle, start, rev) load cleanly on TRACKLINK_V3.

**Non-Goals:**
- Supporting deprecated flat filenames (`/sounds/ScaniaV8-idle.pcm`, `/sounds/bell-Dummy.pcm`).
- Modifying the binary PCM header format or audio output pipeline.

## Decisions

1. **Prioritize Bundle Directory Structure**:
   - Check `/vehicle_configs/<soundSet>/sounds/<slot>.pcm` first, ensuring that uploaded or staged vehicle bundles load without requiring path rewrites.
2. **Clean Fallback Order**:
   - Fall back to common vehicle-type presets (`common/<type>/<slot>.pcm`) when a vehicle bundle does not define a specific optional sound slot (such as generic brake or shifting sounds).
3. **Remove Dead Legacy Lookups**:
   - Strip out `genericNames` table and legacy root-level sound lookups from `ConfigParser`.

## Risks / Trade-offs

- **[Risk] Missing Sound Assets** → If a sound is omitted from both the bundle and the common preset, the slot remains empty (gracefully ignored by the sound engine mixer without crash).
