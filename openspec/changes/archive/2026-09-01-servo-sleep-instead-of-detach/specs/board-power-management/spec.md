## MODIFIED Requirements

### Requirement: Failsafe coordination with power timeout
The disconnect failsafe phases SHALL coordinate cleanly with the board's `disconnect_timeout_s` timer:
1. Short disconnects (< 30s) preserve engine idle and allow instant throttle recovery upon zero-crossing.
2. Medium disconnects (30s to `disconnect_timeout_s`) shut down engine audio and preserve low power state.
3. Sustained disconnects exceeding `disconnect_timeout_s` trigger complete board shutdown via `HardwareInit::powerOff()`.

#### Scenario: Full disconnect timeout progression
- **WHEN** signal is lost and not restored
- **THEN** vehicle brakes to stop, puts servos to sleep, drops sound to idle at t=0, stops engine sound at t=30s, and powers down board at t=disconnect_timeout_s
