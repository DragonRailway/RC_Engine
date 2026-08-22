## ADDED Requirements

### Requirement: Virtual Mass Inertia Drive Ramping
The system SHALL filter the raw commanded vehicle throttle through an acceleration and deceleration inertia ramp before driving physical motor outputs, matching authentic vehicle mass dynamics.

#### Scenario: Smooth acceleration under load
- **WHEN** the engine is in RUNNING state and throttle changes rapidly from 0% to 100%
- **THEN** the physical motor speed shall increment progressively by the configured acceleration rate rather than stepping immediately to 100%

#### Scenario: Coasting deceleration on throttle release
- **WHEN** the driver releases the gas pedal from 100% to 0% with brake inactive
- **THEN** the physical motor speed shall coast down gradually at the configured deceleration rate

#### Scenario: Active braking with variable deceleration
- **WHEN** the brake pedal is applied while the vehicle is in motion
- **THEN** the physical motor speed shall decelerate at an increased rate proportional to the brake pressure

### Requirement: Direct Mode Fallback
The system SHALL automatically bypass the inertia simulation filter and operate in 1:1 direct throttle mode when vehicle configuration or engine parameters are omitted.

#### Scenario: Missing vehicle configuration
- **WHEN** no `/vehicle-config.json` exists on LittleFS
- **THEN** motor output shall map directly 1:1 with throttle input without inertia delay

#### Scenario: Configuration without engine section
- **WHEN** `/vehicle-config.json` is loaded but lacks an `"engine"` block or has inertia explicitly disabled
- **THEN** motor output shall map directly 1:1 with throttle input without inertia delay

### Requirement: Drivetrain Type Consistency
The system SHALL apply virtual mass inertia uniformly across both single-motor Ackermann and dual-motor skid-steer drivetrains.

#### Scenario: Skid-steer track speed calculation
- **WHEN** differential steering is applied on a skid-steer vehicle in motion
- **THEN** the base linear forward speed shall follow the inertia ramp while differential steering offsets are applied cleanly without lag or deadlocks

#### Scenario: Park interlock zeroing
- **WHEN** gear is switched to Park (P=1) or engine is stopped (OFF)
- **THEN** both the inertia speed state and the physical motor outputs shall immediately reset to 0
