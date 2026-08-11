## ADDED Requirements

### Requirement: Hydraulic Flow Sound and Load Governor
The system SHALL detect auxiliary hydraulic control activity (> 10% magnitude), trigger hydraulic flow sound hiss, and increase engine target RPM by +20% of maximum RPM to simulate hydraulic pump load.

#### Scenario: Hydraulic movement detected
- **WHEN** auxiliary hydraulic control input magnitude exceeds 10%
- **THEN** hydraulic flow sound hiss triggers and engine target RPM is increased by 20% of max RPM

#### Scenario: Hydraulic movement idle
- **WHEN** auxiliary hydraulic control inputs return below 10%
- **THEN** hydraulic flow sound hiss deactivates and engine RPM returns to base throttle level

### Requirement: Track Pin Rattle Synthesis
The system SHALL evaluate vehicle movement speed when track rattle is enabled and trigger rhythmic metallic pin clanking sound effects with interval scaling based on speed.

#### Scenario: Tracked vehicle in motion
- **WHEN** vehicle speed is greater than zero and track rattle is enabled
- **THEN** track pin rattle sound triggers with clank intervals scaling from 500ms at low speed down to 90ms at top speed

### Requirement: Physical Auxiliary Servo Channel Control
The system SHALL map auxiliary hydraulic control values (provided through the public input surface) to physical ESP32 servo channels via `EasyServo`: Aux Servo 1 on the S2 pin and Aux Servo 2 on the S3 pin. Boards without an S3 pin (TRACKLINK_V3 has only S1/S2) SHALL provide Aux Servo 1 only; Aux Servo 2 requires a board with an S3 pin (MIKRO_V2).

#### Scenario: Auxiliary servo positioning
- **WHEN** auxiliary hydraulic control values change
- **THEN** available physical servo channels adjust pulse width proportionally between 1000us and 2000us

#### Scenario: Aux Servo 2 on a board without S3
- **WHEN** the active board has no S3 pin (TRACKLINK_V3)
- **THEN** Aux Servo 2 remains uninitialized and Aux Servo 1 continues to operate

### Requirement: Auxiliary hydraulic input surface
The firmware SHALL expose the work-machine hydraulic channel values, dump bed toggle, and bucket rattle trigger as writable public fields so external UI wiring (RadioKit widget bindings) can drive work-machine behavior without firmware-side input mapping. The UI bindings themselves are out of scope for the firmware.

#### Scenario: UI writes a hydraulic value
- **WHEN** external UI wiring writes a hydraulic channel value with magnitude above 10%
- **THEN** the control loop applies the corresponding servo output, activates hydraulic flow sound, and applies the engine load governor

#### Scenario: UI triggers dump bed
- **WHEN** external UI wiring sets the dump bed toggle
- **THEN** the dump bed sound effect activates and the hydraulic flow governor is applied
