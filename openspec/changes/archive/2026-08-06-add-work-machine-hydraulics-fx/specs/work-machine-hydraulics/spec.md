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
The system SHALL map auxiliary hydraulic control inputs to physical ESP32 servo channels (Servo 2 and Servo 3) via `EasyServo`.

#### Scenario: Auxiliary servo positioning
- **WHEN** auxiliary hydraulic control values change
- **THEN** physical Servo 2 and Servo 3 outputs adjust pulse width proportionally between 1000us and 2000us
