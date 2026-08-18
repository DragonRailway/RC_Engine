# Design: Decoupled RadioKit Lifecycle & Dynamic BLE Advertising

## Architecture Overview

```
                      ┌──────────────────────────────────────────────┐
                      │ 1. initRadioKit() [RADIOKIT.h]               │
                      │    Defines widget labels, icons, pages, etc. │
                      └──────────────────────┬───────────────────────┘
                                             │
                                             ▼
                      ┌──────────────────────────────────────────────┐
                      │ 2. applyDeviceMetadata() [main.cpp]          │
                      │    Resolves Priority Cascade:                │
                      │    • Hardware name -> Vehicle -> Fallback    │
                      │    • Hardware desc -> Vehicle -> Fallback    │
                      │    • Type -> "Truck" vs "Locomotive"         │
                      └──────────────────────┬───────────────────────┘
                                             │
                                             ▼
                      ┌──────────────────────────────────────────────┐
                      │ 3. RadioKit.begin() [main.cpp]               │
                      │    Commits resolved dynamic configuration    │
                      └──────────────────────┬───────────────────────┘
                                             │
                                             ▼
                      ┌──────────────────────────────────────────────┐
                      │ 4. RadioKit.startBLE() & enableFS()          │
                      │    Broadcasts BLE GAP advertisement using    │
                      │    resolved name (e.g., "Scania V8")         │
                      └──────────────────────────────────────────────┘
```

## Detailed Design

### 1. `src/RADIOKIT.h` Changes
- `initRadioKit()` will continue to set widget labels, spring behavior, item masks, and page orientations.
- The default configuration assignments (`RadioKit.config.name = "RC_UI"`, etc.) are left as baseline defaults or omitted if set dynamically.
- `RadioKit.begin()`, `RadioKit.startSerial(Serial)`, `RadioKit.startBLE()`, and `RadioKit.enableFS()` are removed from `initRadioKit()`.

### 2. `src/main.cpp` Lifecycle Changes
Inside `setup()`:
```cpp
    Serial.println("\n── Starting RadioKit ──");
    initRadioKit();
    applyDeviceMetadata(hwConfig, profile.config);

    RadioKit.begin();
    RadioKit.startSerial(Serial);
    RadioKit.startBLE();
    RadioKit.enableFS();

    UiLogger::onRadioKitStarted();
    applyAuxSliderProfile();
```

### 3. Dynamic Runtime BLE Name Updates
Inside `applyDeviceMetadata()` during boot and hot-reload (`reloadConfigs()`):
```cpp
static void applyDeviceMetadata(const HardwareConfig& hw, const RcEngineSound::Config& vc) {
    if (hw.name[0] != '\0') {
        RadioKit.config.name = hw.name;
    } else if (vc.name[0] != '\0' && strcmp(vc.name, "Unknown") != 0) {
        RadioKit.config.name = vc.name;
    } else {
        RadioKit.config.name = "RC_UI";
    }

    if (hw.description[0] != '\0') {
        RadioKit.config.description = hw.description;
    } else if (vc.description[0] != '\0') {
        RadioKit.config.description = vc.description;
    }

    if (vc.type == RcEngineSound::VEHICLE_LOCOMOTIVE) {
        RadioKit.config.type = "Locomotive";
        RadioKit.setActivePage(1);
    } else {
        RadioKit.config.type = "Truck";
        RadioKit.setActivePage(0);
    }

#if defined(RK_ENABLE_BLE)
    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    if (pAdv) {
        pAdv->setName(RadioKit.config.name.c_str());
    }
#endif

    Serial.printf("[Device] Name: '%s', Description: '%s', Type: '%s'\n",
                  RadioKit.config.name, RadioKit.config.description, RadioKit.config.type);
}
```

### 4. Hardware Configs
Ensure hardware configs without a custom board name (like `hardware-MIKRO_V2-truck.json`) have `""` or omit `name` so the vehicle bundle's name (`Scania V8`, `Caterpillar 323`, etc.) is dynamically chosen by default.
