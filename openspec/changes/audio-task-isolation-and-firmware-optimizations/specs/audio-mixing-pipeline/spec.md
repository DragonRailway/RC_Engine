## ADDED Requirements

### Requirement: Non-blocking real-time audio task execution
The `audioTask` in `AudioOutput.h` SHALL execute without blocking I/O or direct serial logging calls inside the real-time sample generation and I2S DMA write loop:
1. The task SHALL NOT invoke `Serial.printf` or any blocking UART/USB CDC logging functions inside its main loop.
2. Block generation statistics SHALL be maintained in atomic counters accessible asynchronously for diagnostics.

#### Scenario: Audio rendering during disconnected or stalled USB serial
- **WHEN** USB serial TX buffer is full or disconnected
- **THEN** `audioTask` continues uninterrupted without DMA underruns or buffer starvation

### Requirement: Precomputed reciprocal invariants in sound synthesis
The sound engine `RcEngineSound` SHALL precalculate and cache reciprocal floating-point invariants during `setConfig()`:
1. Invariant terms including `invRpmRange = 1.0f / (maxRpm - idleEndPoint)` and `invMaxRpm = 1.0f / maxRpm` SHALL be computed at configuration time.
2. Pitch scaling and throttle percentage calculations in `update()` SHALL use precalculated reciprocals with floating-point multiplication instead of runtime division.

#### Scenario: Real-time pitch calculation
- **WHEN** engine RPM changes during throttle transitions
- **THEN** `pitchFactor` is computed using single-cycle floating-point multiplication with `invRpmRange`
