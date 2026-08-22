## Context

The RC sound engine runs on an ESP32-S3 delivering 22,050 Hz audio to an I2S DAC / Class-D amplifier (MAX98357A). The previous implementation used sample-by-sample generation (`getNextSample()`) with an 8-bit unsigned output (+128 DC bias) and suffered from array index misalignment, unweighted voice summing, and per-sample critical section thrashing.

## Goals / Non-Goals

**Goals:**
- Eliminate voice misconfigurations (`JAKE_BRAKE`, `FAN`, `SIREN`, `BRAKE`, `COUPLING`, `INDICATOR`) using C++ designated initializers.
- Replicate reference sub-mix gain staging (0.8x engine/horn, 0.2x turbo/fan/SC, 0.2x group B effects).
- Upgrade the audio engine to native 16-bit signed PCM block rendering (`renderBlock()`).
- Reduce task synchronization overhead by 98% (from 22,050 lock pairs/sec to 344/sec).
- Maintain 100% backward compatibility with all existing JSON vehicle profiles and LittleFS schemas.

**Non-Goals:**
- Modifying sound sample file formats or re-encoding JSON PCM data.
- Changing vehicle physics, ESC control loops, or RadioKit UI communication protocols.

## Decisions

### 1. Block-Based Rendering Architecture (`renderBlock`)
- **Decision**: Replace per-sample `getNextSample()` calls with `RcEngineSound::renderBlock(int16_t* interleavedStereoBuffer, size_t frames)`.
- **Rationale**:
  - Takes a single `voiceMutex` snapshot per 64-sample buffer (every ~2.9ms) instead of per sample.
  - Loops only over active voices in tightly unrolled inner loops, enabling compiler vectorization/pipelining.
  - Directly generates 16-bit interleaved stereo samples for I2S DMA.
- **Alternatives Considered**:
  - Keep `getNextSample()` with 16-bit return: Fixes audio bit depth but retains 44,100 critical section transitions/sec.

### 2. Digital Sub-Mix Gain Staging
- **Decision**: In `renderBlock()`, scale voice contributions according to reference hardware grouping:
  ```cpp
  // Group A (Engine): 0.8x factor
  // Auxiliary Engine (Turbo, Fan, Supercharger): 0.2x factor
  // Group B (Knock, Wastegate, Air Brake, Parking Brake, Shifting, Reversing, Coupling, Uncoupling): 0.2x factor
  // Horn & Siren: 0.8x factor
  // Excavator (Hydraulic Flow, Track Rattle, Bucket Rattle) & Tire Squeal: 1.0x factor
  ```
- **Rationale**: Vehicle profile JSON files (e.g. Scania V8) configure raw gains like `dieselKnock = 400%` and `wastegate = 250%`, which expect the $0.2\times$ Group B multiplier.

### 3. Native 16-Bit Dynamic Range Headroom
- **Decision**: Accumulate voice samples in 32-bit integers (`int32_t`), scale by master volume and channel weights, shift to 16-bit range ($\ll 8$), and clamp to `[-32768, 32767]`.
- **Rationale**: Eliminates quantization noise floor, removes artificial 8-bit clipping, and avoids DC offset math.

### 4. Designated Initializer Array Mapping
- **Decision**: Map `voiceDefs` using explicit `[SoundID]` designated initializers:
  ```cpp
  static const VoiceDef voiceDefs[SOUND_COUNT] = {
      [IDLE]            = { .pitchShifted = true,  .loop = true,  .oneShot = false },
      [REV]             = { .pitchShifted = true,  .loop = true,  .oneShot = false },
      [START]           = { .pitchShifted = false, .loop = false, .oneShot = false },
      [KNOCK]           = { .pitchShifted = false, .loop = false, .oneShot = true  },
      [TURBO]           = { .pitchShifted = false, .loop = true,  .oneShot = false },
      [WASTEGATE]       = { .pitchShifted = false, .loop = false, .oneShot = true  },
      [HORN]            = { .pitchShifted = false, .loop = true,  .oneShot = false },
      [JAKE_BRAKE]      = { .pitchShifted = true,  .loop = true,  .oneShot = false },
      [FAN]             = { .pitchShifted = true,  .loop = true,  .oneShot = false },
      [SIREN]           = { .pitchShifted = false, .loop = true,  .oneShot = false },
      [BRAKE]           = { .pitchShifted = false, .loop = false, .oneShot = true  },
      [PARKING_BRAKE]   = { .pitchShifted = false, .loop = false, .oneShot = true  },
      [SHIFTING]        = { .pitchShifted = false, .loop = false, .oneShot = true  },
      [REVERSING]       = { .pitchShifted = false, .loop = true,  .oneShot = false },
      [INDICATOR]       = { .pitchShifted = false, .loop = true,  .oneShot = false },
      [COUPLING]        = { .pitchShifted = false, .loop = false, .oneShot = true  },
      [SUPERCHARGER]    = { .pitchShifted = true,  .loop = true,  .oneShot = false },
      [UNCOUPLING]      = { .pitchShifted = false, .loop = false, .oneShot = true  },
      [SOUND1]          = { .pitchShifted = false, .loop = true,  .oneShot = false },
      [TIRE_SQUEAL]     = { .pitchShifted = false, .loop = true,  .oneShot = false },
      [HYDRAULIC_PUMP]  = { .pitchShifted = true,  .loop = true,  .oneShot = false },
      [HYDRAULIC_FLOW]  = { .pitchShifted = false, .loop = true,  .oneShot = false },
      [TRACK_RATTLE]    = { .pitchShifted = false, .loop = false, .oneShot = true  },
      [BUCKET_RATTLE]   = { .pitchShifted = false, .loop = false, .oneShot = true  },
      [BELL]            = { .pitchShifted = false, .loop = true,  .oneShot = false },
      [DOOR]            = { .pitchShifted = false, .loop = false, .oneShot = true  },
      [SCANNER]         = { .pitchShifted = false, .loop = true,  .oneShot = false },
      [MUSIC]           = { .pitchShifted = false, .loop = true,  .oneShot = false },
      [WHISTLE]         = { .pitchShifted = false, .loop = true,  .oneShot = false },
      [GUN]             = { .pitchShifted = false, .loop = false, .oneShot = true  },
      [OUT_OF_FUEL]     = { .pitchShifted = false, .loop = true,  .oneShot = false },
      [OTHERS]          = { .pitchShifted = false, .loop = true,  .oneShot = false },
  };
  ```

## Risks / Trade-offs

- **[Risk]** DSP test scripts expecting `getNextSample()` might break.
  $\to$ **Mitigation**: Maintain `getNextSample()` as an inline adapter delegating to a single-frame render or returning signed sample for test harnesses.
- **[Risk]** Volume perceived as slightly quieter due to lack of 8-bit square-wave clipping.
  $\to$ **Mitigation**: Clean 16-bit dynamics preserve transient peaks without distortion; master volume scales up to full 16-bit DAC amplitude.
