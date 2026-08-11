## MODIFIED Requirements

### Requirement: Engine Start and Stop State Machine
The system SHALL boot in an engine OFF state and require a dedicated latched Engine Power toggle widget (`engine_start`, `RK_ToggleButton`) to start the engine: the engine transitions OFF → STARTING (cranking sound) when the toggle switches ON and RUNNING → OFF when the toggle switches OFF. Throttle input SHALL NOT start the engine; while the engine is OFF or STARTING, motor drive commands are disabled regardless of throttle widget position.

#### Scenario: Boot state is OFF
- **WHEN** system boots up and the Engine Power toggle is OFF
- **THEN** engine state is OFF, idle sound is silent, and motor drive commands are disabled

#### Scenario: Engine Start triggered
- **WHEN** the Engine Power toggle switches ON
- **THEN** engine state becomes STARTING, cranking audio sample plays, and upon completion transitions state to RUNNING

#### Scenario: Engine stop
- **WHEN** the Engine Power toggle switches OFF while the engine is RUNNING
- **THEN** engine state transitions to OFF and drive commands are disabled

#### Scenario: Start cancelled mid-crank
- **WHEN** the Engine Power toggle switches OFF while the engine is STARTING
- **THEN** the cranking start is cancelled and engine state returns to OFF

#### Scenario: Throttle does not auto-start
- **WHEN** throttle input is applied while the engine is OFF and the Engine Power toggle is OFF
- **THEN** the engine remains OFF and the drive motor stays disabled

#### Scenario: Reversing light independent of engine start
- **WHEN** the reversing light control bit (Item E / bit 4) is asserted
- **THEN** the reversing light turns on and the engine power state is unaffected
