# Config Loader Implementation Plan

## Overview
Load hardware and vehicle configs from LittleFS JSON files, map pin names to GPIOs, and initialize all peripherals.

## Config Files

### Hardware Config (`/hardware-config.json`)
- `SOUND.VOLUME` - Master volume (0-100)
- `DRIVE_MOTOR` - H-Bridge or ESC config
  - `HARDWARE`: "HBRIDGE_A", "HBRIDGE_B", "S1" (ESC), "S2" (ESC)
  - `FREQUENCY`: PWM frequency (Hz)
  - `DIRECTION`: "FORWARD", "REVERSE", "UNI_FORWARD", "UNI_REVERSE"
  - `DUTY.MIN/MAX`: Duty cycle range (0-100)
- `STEERING_SERVO` - Servo config
  - `HARDWARE`: "S1", "S2"
  - `FREQUENCY`: 50Hz typical
  - `ENDPOINTS.LEFT/RIGHT/CENTER`: Pulse width (μs)
- `LIGHTS` - LED channels
  - `HEAD_LIGHT`, `TAIL_LIGHT`, `BRAKE_LIGHT`: Single LED
  - `TURN_LIGHT.LEFT/RIGHT`: Blinker LEDs
  - `HAZARD_LIGHT`, `REVERSING_LIGHT`: Virtual (reference other lights)

### Vehicle Config (`/vehicle-config.json`)
- `VEHICLE.NAME`, `VEHICLE.TYPE`
- `ENGINE.*` - Acceleration, deceleration, RPM settings
- `TRANSMISSION.TYPE`, `TRANSMISSION.NUMBER_OF_GEARS`
- `SOUND_VOLUME.*` - All 17 sound volume percentages

## Pin Name Mapping

| Name | GPIO | Function |
|------|------|----------|
| L0 | 42 | Built-in LED |
| L1 | 6 | LED 1 |
| L2 | 7 | LED 2 |
| L3 | 8 | LED 3 |
| L4 | 9 | LED 4 |
| L5 | 10 | LED 5 |
| L6 | 11 | LED 6 |
| S1 | 1 | Servo/ESC 1 |
| S2 | 2 | Servo/ESC 2 |
| HBRIDGE_A | - | {PWM1:13, PWM2:14, BEMF:4} |
| HBRIDGE_B | - | {DIR:15, PWM:16, BEMF:5} |

## Implementation Plan

### Phase 1: Config Structures

**File: `src/Config.h`**

```cpp
struct HardwareConfig {
    struct Sound { uint8_t volume; } sound;
    struct DriveMotor {
        enum Type { HBRIDGE, ESC } type;
        uint8_t hardwareId;  // HBRIDGE_A=0, HBRIDGE_B=1, S1=2, S2=3
        uint16_t frequency;
        enum Direction { FORWARD, REVERSE, UNI_FORWARD, UNI_REVERSE } direction;
        struct { uint8_t min; uint8_t max; } duty;
    } driveMotor;
    struct SteeringServo {
        uint8_t hardwareId;
        uint16_t frequency;
        struct { uint16_t left; uint16_t right; uint16_t center; } endpoints;
    } steeringServo;
    struct Lights {
        struct Light { uint8_t pin; uint8_t brightness; };
        Light headLight;
        Light tailLight;
        Light brakeLight;
        struct TurnLight { uint8_t leftPin; uint8_t rightPin; uint8_t brightness; uint16_t intervalOn; uint16_t intervalOff; } turnLight;
    } lights;
};

struct VehicleConfig {
    char name[32];
    char type[16];
    struct Engine { uint8_t acc; uint8_t dec; uint8_t idleRpm; uint16_t clutchRpm; uint16_t revSwitchPoint; uint16_t idleEndPoint; uint8_t knockInterval; uint8_t knockStartPoint; uint8_t jakeBrakeMinRpm; uint8_t fanStartPoint; } engine;
    struct Transmission { enum Type { AUTOMATIC, MANUAL, NONE } type; uint8_t numberOfGears; } transmission;
    struct SoundVolume { uint8_t start; uint8_t idle; uint8_t engineIdle; uint8_t fullThrottle; uint8_t rev; uint8_t engineRev; uint8_t turbo; uint8_t knock; uint8_t wastegate; uint8_t horn; uint8_t siren; uint8_t brake; uint8_t parkingBrake; uint8_t shifting; uint8_t reversing; uint8_t indicator; uint8_t coupling; uint8_t jakeBrake; uint8_t fan; } soundVolume;
};
```

### Phase 2: Pin Mapper

**File: `src/PinMapper.h`**

Maps hardware names to GPIO pins:

```cpp
class PinMapper {
public:
    static uint8_t resolve(const char* name) {
        // LED pins
        if (strcmp(name, "L0") == 0) return PIN::L0;
        if (strcmp(name, "L1") == 0) return PIN::L1;
        // ... L2-L6
        
        // Servo/ESC pins
        if (strcmp(name, "S1") == 0) return PIN::S1;
        if (strcmp(name, "S2") == 0) return PIN::S2;
        
        // H-Bridge (returns common enable, specific pins via struct)
        if (strcmp(name, "HBRIDGE_A") == 0) return HBRIDGE::COMMON_EN;
        if (strcmp(name, "HBRIDGE_B") == 0) return HBRIDGE::COMMON_EN;
        
        return 0xFF; // Invalid
    }
    
    static HBRIDGE getHBridge(const char* name) {
        if (strcmp(name, "HBRIDGE_A") == 0) return {HBRIDGE::A::PWM1, HBRIDGE::A::PWM2, HBRIDGE::A::BEMF};
        if (strcmp(name, "HBRIDGE_B") == 0) return {HBRIDGE::B::DIR, HBRIDGE::B::PWM, HBRIDGE::B::BEMF};
        return {0, 0, 0};
    }
};
```

### Phase 3: Config Parser

**File: `src/ConfigParser.h`**

Extends `ConfigLoader` with structured parsing:

```cpp
class ConfigParser {
public:
    static bool loadHardwareConfig(const char* path, HardwareConfig& config);
    static bool loadVehicleConfig(const char* path, VehicleConfig& config);
    
private:
    static void parseDriveMotor(JsonObject& motor, HardwareConfig::DriveMotor& config);
    static void parseSteeringServo(JsonObject& servo, HardwareConfig::SteeringServo& config);
    static void parseLights(JsonObject& lights, HardwareConfig::Lights& config);
    static void parseEngine(JsonObject& engine, VehicleConfig::Engine& config);
    static void parseTransmission(JsonObject& trans, VehicleConfig::Transmission& config);
    static void parseSoundVolume(JsonObject& vol, VehicleConfig::SoundVolume& config);
};
```

### Phase 4: Hardware Initializer

**File: `src/HardwareInit.h`**

Initializes peripherals based on config:

```cpp
class HardwareInit {
public:
    static void init(const HardwareConfig& hw, const VehicleConfig& vehicle);
    
private:
    static void initDriveMotor(const HardwareConfig::DriveMotor& motor);
    static void initSteeringServo(const HardwareConfig::SteeringServo& servo);
    static void initLights(const HardwareConfig::Lights& lights);
    static void initSound(const HardwareConfig::Sound& sound, const VehicleConfig::SoundVolume& vol);
};
```

### Phase 5: Integration

**Update `src/main.cpp`:**

```cpp
void setup() {
    Serial.begin(2000000);
    
    // 1. Mount filesystem
    ConfigLoader::begin();
    
    // 2. Load configs
    HardwareConfig hwConfig;
    VehicleConfig vehicleConfig;
    
    ConfigParser::loadHardwareConfig("/hardware-config.json", hwConfig);
    ConfigParser::loadVehicleConfig("/vehicle-ScaniaV8.json", vehicleConfig);
    
    // 3. Initialize hardware
    HardwareInit::init(hwConfig, vehicleConfig);
    
    // 4. Load sounds (existing VehicleProfile)
    VehicleProfile profile;
    profile.load("/vehicle-ScaniaV8.json");
    
    // 5. Start audio
    RcEngineSound engine;
    // ... begin engine
}
```

## File Changes

### New Files
- `src/Config.h` - Config structures
- `src/PinMapper.h` - Pin name → GPIO mapping
- `src/ConfigParser.h` - JSON → config structures
- `src/HardwareInit.h` - Peripheral initialization

### Modified Files
- `src/main.cpp` - Use new config loader
- `src/ConfigLoader.h` - Add file upload support (optional)

## Testing Strategy

1. **Unit test**: Parse JSON → verify struct fields
2. **Pin test**: Print resolved GPIOs for each hardware name
3. **Motor test**: Init H-Bridge → verify PWM output
4. **Servo test**: Init servo → verify pulse output
5. **Light test**: Init LEDs → verify brightness/blinking
6. **Integration test**: Full boot → all peripherals working

## Questions for User

1. **H-Bridge direction**: Should we support both `HBRIDGE_A` and `HBRIDGE_B` independently, or just one at a time?
2. **ESC protocol**: Support PPM only, or also DSHOT?
3. **Light effects**: Implement blink/fade in software, or use LEDC hardware?
4. **Config hot-reload**: Allow changing config without reboot?
