# I2S Speaker Test (TrackLink V3)

Minimal I2S sine wave test to verify speaker hardware on TrackLink V3 board.

## Purpose

Proves whether the I2S hardware path works (peripheral, GPIO matrix, DAC amp, DMA, speaker). If this test produces sound but the main firmware doesn't, the issue is in the main firmware's audio pipeline.

## Usage

```bash
cd test_i2s_speaker
pio run -t upload
```

Then monitor serial at 2000000 baud to see timing diagnostics.

## Expected Output

```
=== I2S Speaker Test (TRACKLINK_V3) ===
Pins: LRC=17, BCLK=18, DIN=21, SD=47
[1] I2S_SD (amp enable) set HIGH
[2] I2S channel allocated OK
[3] I2S standard mode initialized OK
[4] I2S channel enabled OK
--- All I2S setup succeeded. Playing 440 Hz sine wave... ---
[PLAYING] pos=22016,  write_us=5,    bytes=256, err=0
[PLAYING] pos=44032,  write_us=6,    bytes=256, err=0
[PLAYING] pos=66048,  write_us=7313, bytes=256, err=0
```

## Key Findings

On TrackLink V3, this test proves:
- I2S peripheral initializes correctly
- GPIO matrix routes signals to pins 17/18/21
- DAC amp enable (pin 47) works
- DMA drains at expected rate
- Speaker produces 440 Hz tone

**This eliminates all hardware theories** — the issue is 100% in the main firmware's audio pipeline.

## Pin Mapping

| Function | TrackLink V3 | MIKRO_V2 |
|----------|-------------|----------|
| I2S_LRC  | GPIO 17     | GPIO 48  |
| I2S_BCLK | GPIO 18     | GPIO 47  |
| I2S_DIN  | GPIO 21     | GPIO 33  |
| I2S_SD   | GPIO 47     | GPIO 34  |
