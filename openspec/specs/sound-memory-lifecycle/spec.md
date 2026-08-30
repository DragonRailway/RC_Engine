# Sound Memory Lifecycle Specification

## Purpose
Defines requirements for PSRAM lifecycle management of dynamically allocated sound sample buffers, ensuring proper allocation tracking, deallocation on reassignment/destruction, and safe engine reinitialization without memory leaks.

## Requirements

### Requirement: SoundData PSRAM Lifecycle Management
`SoundData` SHALL track whether its sample buffers were dynamically allocated (`isDynamic`) and SHALL automatically free PSRAM buffers before overwriting or destroying the struct.

#### Scenario: Sound samples allocated with ps_malloc
- **WHEN** `ConfigParser::loadSoundSlot()` successfully allocates a sample buffer via `ps_malloc()`
- **THEN** the resulting `SoundSlot` is stored in a `SoundData` with `isDynamic == true`

#### Scenario: SoundData reassigned with new data
- **WHEN** a `SoundData` with `isDynamic == true` is overwritten via assignment (`soundData = newData`)
- **THEN** the old PSRAM buffers for all slots with non-null `samples` pointers are freed before the new data is copied

#### Scenario: SoundData destroyed
- **WHEN** a `SoundData` with `isDynamic == true` goes out of scope or is destroyed
- **THEN** the PSRAM buffers for all slots with non-null `samples` pointers are freed

#### Scenario: Hot-reload of vehicle sound assets
- **WHEN** `reloadConfigs()` triggers a vehicle name change and `loadSounds()` is called
- **THEN** the old `profile.sounds` PSRAM buffers are freed before the new sound data is loaded, preventing memory leaks across repeated hot-reloads

#### Scenario: SoundData default-constructed (not allocated)
- **WHEN** a `SoundData` is default-constructed or assigned from a default-constructed source (`isDynamic == false`)
- **THEN** no `free()` calls are made on its slot buffers

### Requirement: RcEngineSound Cleanup on begin()
`RcEngineSound::begin()` SHALL free any previously loaded dynamic sound data before copying new `SoundData`, ensuring the engine can safely reinitialize with a new vehicle profile without leaking the old sample buffers.

#### Scenario: Engine reinitialized with new sound data
- **WHEN** `RcEngineSound::begin(newSoundData, newConfig)` is called while the engine already holds dynamically allocated sounds
- **THEN** the old `sounds` buffers are freed before `sounds = newSoundData` executes

#### Scenario: Engine reinitialized with same sound data
- **WHEN** `RcEngineSound::begin()` is called with the same `SoundData` reference (e.g., during config reload with unchanged vehicle name)
- **THEN** no double-free occurs (assignment operator handles self-assignment safely)
