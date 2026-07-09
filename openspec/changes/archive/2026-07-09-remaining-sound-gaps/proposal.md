# Proposal: Remaining Sound Gaps — Tire Squeal, Hydraulic, Track Rattle

## Summary

Close the final sound gaps between our implementation and the reference Rc_Engine_Sound_ESP32 library. The reference supports **5 additional sound channels** we currently lack: tire squeal, hydraulic pump, hydraulic flow, track rattle, and bucket rattle. Additionally, it has a **dump bed mode** that ties hydraulic sounds to bed movement.

These features unlock support for **excavators, cranes, dozers, dump trucks**, and **sports cars** — vehicle types our current engine can't fully represent.

## What We're Adding

| # | Feature | Voice Slots | Reference Vehicles |
|---|---------|-------------|-------------------|
| 1 | Tire Squeal | 1 | LaFerrari, MercedesActrosV6 |
| 2 | Hydraulic Pump | 1 | CAT323, VolvoL120H, D6Dozer |
| 3 | Hydraulic Flow | 1 | CAT323, CAT730, BenfordDumper |
| 4 | Track Rattle | 1 | All tracked vehicles |
| 5 | Bucket Rattle | 1 | All excavators |
| 6 | Dump Bed Mode | 0 (config flag) | CAT730, BenfordDumper |

**Total new voice slots: 5** (bringing VOICE_COUNT from 19 to 24)

## Why This Matters

- **Vehicle diversity**: The reference supports 30+ vehicle profiles. We can only do trucks/cars well today.
- **Excavator support**: Hydraulic sounds are the defining characteristic of excavator/crane audio.
- **Tracked vehicle support**: Track rattle is essential for dozer/tank/excavator realism.
- **Sports car support**: Tire squeal is the signature sound for high-power RWD vehicles.

## What We're NOT Changing

- Existing 19 voice slots remain unchanged
- No changes to the audio output pipeline (ISR, I2S, DAC offset)
- No changes to the vehicle profile loading system
- Backward compatible — all new volume configs default to 0 (disabled)

## Reference Behavior Summary

### Tire Squeal
- **Trigger**: Throttle > threshold AND speed < threshold
- **Volume**: Scales inversely with speed (louder at low speed)
- **Loop**: Continuous while conditions met
- **Sound file**: `squeal-Tire2.json` (114,708 samples)

### Hydraulic Pump
- **Trigger**: Engine running + hydraulic mode active
- **Volume**: RPM-dependent (scales with engine speed)
- **Hydrostatic mode**: When enabled, volume also scales with vehicle speed
- **Loop**: Continuous while hydraulic mode active

### Hydraulic Flow
- **Trigger**: Boom/bucket movement detected
- **Volume**: Fixed or proportional to movement speed
- **Loop**: Continuous while movement detected

### Track Rattle
- **Trigger**: Vehicle moving (speed > 0)
- **Volume**: Fixed per vehicle config
- **Interval**: Speed-dependent — faster speed = shorter interval between rattles
- **Dual rattle**: Optional second track rattle with delay for realism

### Bucket Rattle
- **Trigger**: Bucket movement detected
- **Volume**: Fixed per vehicle config
- **Loop**: One-shot per movement event

### Dump Bed Mode
- **Trigger**: Dump bed raise/lower button
- **Behavior**: Activates hydraulic pump + flow sounds
- **Config flag**: `DUMP_BED` equivalent in JSON
