## ADDED Requirements

### Requirement: Engine RPM Acceleration and Deceleration Inertia
The system SHALL simulate virtual flywheel mass by ramping engine RPM over time according to acceleration and deceleration parameters.

#### Scenario: Throttle acceleration ramp
- **WHEN** gas pedal is applied
- **THEN** current RPM SHALL ramp smoothly from idle toward target RPM bounded by acceleration parameter `acc` and flywheel `inertia`

#### Scenario: Throttle release deceleration
- **WHEN** gas pedal is released to zero
- **THEN** current RPM SHALL decay smoothly back to idle RPM bounded by deceleration parameter `dec`

### Requirement: Jake Brake Physics Drag
The system SHALL apply additional engine braking drag when Jake brake is engaged during deceleration.

#### Scenario: Jake brake activation during high-RPM deceleration
- **WHEN** Jake brake is activated while engine RPM is above minimum threshold
- **THEN** engine deceleration rate SHALL increase and Jake brake audio voice SHALL play

### Requirement: Hydraulic Governor RPM Boost
The system SHALL apply an engine governor RPM boost when auxiliary hydraulics are operated.

#### Scenario: Auxiliary hydraulic slider movement
- **WHEN** auxiliary hydraulic control slider is moved
- **THEN** engine idle RPM SHALL receive a +20% governor boost to simulate hydraulic pump load
