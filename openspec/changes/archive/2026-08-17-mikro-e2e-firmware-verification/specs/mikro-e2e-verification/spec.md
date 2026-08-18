## ADDED Requirements

### Requirement: Real-Time Serial Debug Telemetry
The firmware SHALL stream structured, machine-parsable debug telemetry lines over USB CDC Serial at 2,000,000 baud when vehicle state changes or periodically every 250ms when active.

#### Scenario: Telemetry state reporting
- **WHEN** the vehicle controller runs its control loop
- **THEN** it outputs formatted `[STATUS]` lines containing Engine state, RPM, Throttle %, Motor %, Steering angle, Gear, Light bitmask, and Battery voltage.

### Requirement: Sound Engine Audio Verification
The audio system and test harness SHALL verify that sound rendering executes within real-time DSP budgets (< 2900 µs buffer computation time at 22,050 Hz) and generates valid PCM audio without mathematical NaN or clipping exceptions.

#### Scenario: Audio DSP performance assertion
- **WHEN** the sound engine is actively synthesizing engine, turbo, shifting, horn, and brake sounds
- **THEN** `[AUDIO_STATS]` lines report DSP computation time under 2900 µs and zero panic/NaN occurrences.

### Requirement: Automated Remote API & Hardware End-to-End Suite
The host verification suite SHALL orchestrate firmware building, flashing, LittleFS config bundle deployment (`hardware-MIKRO_V2-truck.json` + `ScaniaV8`), and full functional control over the Android RadioKit REST API (`127.0.0.1:17007`) with real-time serial verification over `/dev/ttyACM0`.

#### Scenario: Full functional verification
- **WHEN** the test script runs against physical hardware and companion app
- **THEN** it successfully starts the engine, shifts gears (D/P/R), ramps throttle and brake blends, sweeps steering with auto-indicator canceling, steps headlights (Off/Low/High), triggers horn and aux channels, and verifies serial ACK and telemetry responses.
