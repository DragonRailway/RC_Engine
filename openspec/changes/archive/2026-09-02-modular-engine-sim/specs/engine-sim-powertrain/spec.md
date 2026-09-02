# engine-sim-powertrain Specification

## Purpose
Simulate vehicle powertrain physics, engine state machine, RPM dynamics, transmission gearing, torque converter slip, and ESC motor speed ramping in a dedicated 50 Hz model.

## ADDED Requirements

### Requirement: Engine state machine
The `EngineSim` class SHALL maintain an engine state machine with states `OFF`, `STARTING`, `RUNNING`, `STOPPING`, and `PARKING_BRAKE`. The simulation SHALL only produce motor drive output and revving RPM when in the `RUNNING` state (or direct mode with `hasEngine = false`).

#### Scenario: Engine start sequence
- **WHEN** `startEngine()` is called while state is `OFF`
- **THEN** state transitions to `STARTING`, resets virtual speed to 0, and advances to `RUNNING` after startup duration completes

#### Scenario: Engine stop sequence
- **WHEN** `stopEngine()` is called while state is `RUNNING` and RPM is below stop threshold
- **THEN** state transitions to `STOPPING`, ramps down RPM over `stopDurationMs`, and settles to `OFF` (or `PARKING_BRAKE` if parking brake engaged)

### Requirement: Unified ESC motor speed ramping and inertia
The `EngineSim` class SHALL compute the ramped ESC motor speed output (`getMotorSpeed()`) using configured acceleration (`acc`), deceleration (`dec`), brake deceleration (`brakeDec`), and gear ramp times (`gearRampTimes` / `escRampTime`).

#### Scenario: Normal acceleration ramp
- **WHEN** positive throttle target exceeds current motor speed
- **THEN** `EngineSim` increments motor speed by step size scaled by `acc` and elapsed time `dtMs`

#### Scenario: Dynamic braking deceleration
- **WHEN** brake input is applied or vehicle decelerates towards zero
- **THEN** motor speed steps down according to `brakeDec` and brake percentage until stationary

#### Scenario: Direct mode bypass
- **WHEN** `hasEngine` is false or `inertia` is set to 0
- **THEN** `getMotorSpeed()` immediately follows target speed without inertia ramping

### Requirement: Transmission and torque converter slip simulation
The `EngineSim` class SHALL simulate automatic and manual transmissions:
1. For automatic transmission (`TRANS_AUTOMATIC`): computes gear index from `motorSpeed` and adds torque converter slip proportional to `engineLoad` and `torqueConverterSlip`. In 1st gear / launch, slip multiplier is doubled.
2. For manual transmission (`TRANS_MANUAL`): shifts gears across configured shift thresholds and flags gear change events.

#### Scenario: Automatic transmission torque converter stall
- **WHEN** full throttle is applied from standstill (1st gear)
- **THEN** engine RPM rises with torque converter slip before vehicle motor speed catches up

#### Scenario: Automatic gear upshift
- **WHEN** vehicle motor speed exceeds the current gear threshold
- **THEN** `EngineSim` increments `selectedGear` and recalculates gear ratio base RPM

### Requirement: Physics-based event detection
The `EngineSim` class SHALL evaluate and expose boolean flags for physics events:
- `isJakeBrakeActive()`: true when throttle is released at high RPM (`>60% maxRpm`).
- `isWastegateTriggered()`: true on sudden throttle drop (`>150` units) under boost.
- `isBrakeSquealTriggered()`: true when vehicle decelerates to a stop under active braking.
- `isReverseActive()`: true when engaged gear is Reverse.

#### Scenario: High RPM throttle release triggers jake brake
- **WHEN** throttle is dropped to 0 while current RPM exceeds 60% of `maxRpm`
- **THEN** `isJakeBrakeActive()` becomes true and engine RPM decelerates at `jakeBrakeDecelRate`

#### Scenario: Sudden throttle drop triggers wastegate
- **WHEN** throttle decreases rapidly from high boost
- **THEN** `isWastegateTriggered()` becomes true for the blow-off duration
