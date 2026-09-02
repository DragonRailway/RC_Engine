## Why

When vehicles are deployed to LittleFS using the bundle directory structure (e.g. `/vehicle_configs/UnionPacific2002/sounds/*.pcm`), the firmware discovers `/vehicle_configs/UnionPacific2002/vehicle.json` but fails to load sound assets because `ConfigParser::loadSounds()` only checks legacy and staged sound paths (`/sounds/vehicles/...`, `/sounds/...`). Consequently, 0 sound assets are loaded and audio output is disabled at boot.

Since backward compatibility with legacy formats is not needed, sound resolution can be streamlined directly to vehicle bundles and standard fallback paths.

## What Changes

- **Streamlined Bundle Sound Resolution**: Update `ConfigParser::loadSounds()` to resolve `.pcm` sound assets directly from `/vehicle_configs/<soundSet>/sounds/<slot>.pcm` and `/sounds/vehicles/<soundSet>/<slot>.pcm`, followed by common type fallbacks (`/vehicle_configs/common/<type>/<slot>.pcm`, `/sounds/common/<type>/<slot>.pcm`).
- **Remove Obsolete Legacy Sound Fallbacks**: Eliminate deprecated flat prefixes (`/sounds/<soundSet>-<slot>.pcm`), legacy file lookup arrays (`genericNames`), and unorganized root sound lookups.

## Capabilities

### Modified Capabilities
- `config-filesystem-management`: Update sound asset resolution specification to resolve sounds from vehicle bundle directory structures (`/vehicle_configs/<sound_set>/sounds/<slot>.pcm`).

## Impact

- `common/ConfigParser.cpp`: Sound loading path resolution in `loadSounds()`.
- Unlocks immediate sound playback on TRACKLINK_V3 and other boards deployed with vehicle bundle folders.
