## Why

When the BLE or Wi-Fi control connection is interrupted (`!RadioKit.isConnected()`), the vehicle currently retains the last received throttle, steering, aux actuator, and lighting inputs in RAM. If the connection drops while the vehicle is driving, it continues running at speed until the board power-off timer expires, presenting a runaway hazard and risk of gear damage.

This change introduces a deterministic multi-stage failsafe system upon disconnection and a safe throttle-to-neutral interlock on reconnection.

## What Changes

- **50% Gentle Braking Stop**: When connection is lost, drivetrain applies moderate 50% braking torque rather than an instant hard lock or unbraked runaway, bringing the vehicle to a controlled stop while protecting drivetrain gears.
- **Actuator & Pump Depowering**: Immediately detaches/depowers all steering servos (zero stall current/force) and switches off all auxiliary motors and the high-side pump output (`PUMP_ENABLE = LOW`).
- **Sound & Hazard Lighting**: Immediately drops simulated engine RPM to idle, automatically flashes 4-way Hazard lights, and shuts the sound engine OFF after 30 seconds of sustained disconnect.
- **Reconnect Throttle-to-Neutral Interlock**: Upon controller reconnection, reattaches servos and restores lighting, but latches drive torque locked until the user's gas pedal / throttle slider returns to 0 (neutral), preventing sudden vehicle lurching.

## Capabilities

### Modified Capabilities
- `vehicle-control-loop`: Update disconnect state machine to execute 50% braking stop, actuator depowering, idle sound drop, 30s engine shutdown, and throttle-to-neutral reconnection interlock.
- `board-power-management`: Synchronize disconnect failsafe phases with warning intervals and final power-latch shutdown.

## Impact

- `common/VehicleController.cpp` and `common/VehicleController.h`: Connection state tracking, failsafe actuator control, and reconnect interlock logic.
- `test/host_vc/host_vc_driver.cpp`: Host test cases verifying disconnect failsafe braking, servo detach, hazard triggering, 30s engine stop, and throttle zero-latch on reconnect.
