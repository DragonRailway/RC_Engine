## Why

During testing and code exploration of the sound engine simulation, three critical DSP and state-machine issues were identified in `RcEngineSound.cpp`:
1. The diesel knock trigger sample comparison breaks permanently on idle loop circular wrap-around once `lastKnockTriggerSample + knockIntervalSamples > sampleCount`, silencing knock audio.
2. The idle/rev cross-fade calculation inverted the target proportion in `map()`, ramping idle volume up and rev volume down at high RPMs.
3. Audio generation in `getNextSample()` enters and exits critical spinlocks (`portENTER_CRITICAL(&voiceMutex)`) up to 32 times per sample (2048 times per 2.9ms buffer), creating cross-core bus contention with the main control loop.

Fixing these ensures accurate engine audio synthesis and reduces FreeRTOS task contention.

## What Changes

- **Knock Trigger Wrap-around**: Account for circular idle sample buffer index wrap-around in `RcEngineSound::update()` so knock pulses fire continuously across idle loop boundaries.
- **Idle/Rev Cross-fade Correction**: Correct `RcEngineSound::update()` cross-fade mapping so `idleProportion` smoothly decays from 100% (at `idleEndPoint`) to 0% (at `revSwitchPoint`).
- **Critical Section Spinlock Reduction**: Refactor voice state synchronization in `getNextSample()` to perform a single batched critical section for write-backs rather than locking/unlocking per active voice per sample.

## Capabilities

### New Capabilities
None.

### Modified Capabilities
- `audio-debug-tooling`: Update DSP simulation behavioral requirements for continuous knock cadence across idle loop wrap-around and correct idle/rev proportional fading.

## Impact

- **Affected Code**: `lib/SoundEngine/src/RcEngineSound.cpp`, `lib/SoundEngine/src/RcEngineSound.h`.
- **API Impact**: None (internal DSP math and synchronization refactor; external API signatures unchanged).
- **Performance Impact**: Substantially lower cross-core spinlock overhead and uninterrupted knock/rev audio blending.
