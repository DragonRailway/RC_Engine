# RC Engine

RC Engine is an ESP32-S3 controller and dynamic sound synthesizer for RC scale models, trucks, crawlers, tracked vehicles, and locomotives.

It pairs with the **RadioKit** mobile app over Bluetooth Low Energy (BLE) to deliver physics-based engine simulation, multi-channel sound synthesis, lighting automation, and motor drive control.

---

## Features

### Vehicle Physics & Sound Engine
- **Real-Time Engine Simulation**: RPM curves, flywheel inertia, automatic transmission torque-converter slip, and start/stop state management.
- **Sound Effects**: Turbo spool, blow-off valves, diesel engine knock, Jake brake, air release, horns, sirens, and train bells.
- **Pre-Tuned Profiles**: Over 70 sound profiles covering highway trucks, off-road scalers, heavy construction machinery, tanks, and diesel locomotives.

### Drive & Motion Control
- **Drivetrain Modes**: Ackermann steering with servo endpoints or dual-motor differential skid-steer.
- **Drive Actuation**: Direct H-bridge DC motors and hobby RC ESC support with configurable PWM frequencies and direction mapping.
- **Auxiliary Machinery**: Control channels for dump truck tippers, cement mixers, winches, and excavator attachments.

### Lighting Automation
- **Scale Lighting Channels**: Directional headlights, high beams, 2-stage tail/brake lights, turn signals, and hazard flashers.
- **Auxiliary & Cabin Lights**: Dedicated cabin, step, ground, and dual alternating ditch lights.
- **Pattern Sequencer**: Configurable LED animations for beacons, strobes, and emergency lightbars.

### Wireless Control & Hot-Reload
- **RadioKit Mobile App**: Low-latency BLE control interface with tailored cockpit layouts for road trucks, machinery, and trains.
- **Live Telemetry**: Real-time battery voltage monitoring and vehicle speed telemetry.
- **LittleFS Config Bundles**: Hardware and vehicle parameters stored in JSON files that hot-reload without firmware re-flashing.

---

## Supported Boards

| Board | Target Vehicles | Key Peripherals |
| :--- | :--- | :--- |
| **TRACKLINK V3** | 1/14–1/16 trucks, crawlers, locomotives | Dual high-power motor channels, servo rails, expanded lighting outputs, I2S audio |
| **MIKRO V2** | 1/24–1/35 micro scale models | Compact footprint, dual motor driver, servo rail, integrated I2S amplifier, LED outputs |
| **GTRACK** | Tracked machinery, excavators | Dual track motor channels, aux motor drivers, lighting rails, I2S audio |

---

## Configuration Documentation

- **[Hardware Configuration Guide](GUIDE/HARDWARE_CONFIG.md)**: Pin assignments, motor driver types, servo endpoints, lighting channels, and battery monitoring.
- **[Vehicle Configuration Guide](GUIDE/VEHICLE_CONFIG.md)**: Engine RPM limits, inertia ramping, transmission dynamics, and sound volume mixing.

---

## Credits & License

- **RadioKit Ecosystem**: Built with [RadioKit](https://github.com/Radio-Kit/RadioKit) and [ESP32_EasyKit](https://github.com/DragonRailway/ESP32_EasyKit).
- **Sound Engine**: Engine synthesis concepts and baseline audio structures adapted from [Rc_Engine_Sound_ESP32](https://github.com/TheDIYGuy999/Rc_Engine_Sound_ESP32) by TheDIYGuy999.
