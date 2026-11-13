# RC Brain for Rust (`rc-brain-rs`)

A high-performance, asynchronous `no_std` Rust vehicle controller and real-time audio sound synthesizer for RC scale models, trucks, crawlers, tracked machinery, and locomotives.

Powered by **`embassy-rs`**, **`esp-hal`**, and **`rk-rs`** on the **ESP32-S3**.

---

## Workspace Structure

- **[`rc-dsp`](crates/rc-dsp)**: Real-time 32-voice audio synthesizer and DSP mixer (`no_std`):
  - 4-point Catmull-Rom Hermite cubic spline interpolation (`hermite.rs`)
  - Rational soft-knee saturator / limiter (`limiter.rs`)
  - Procedural diesel engine knock pulse generators (`knock.rs` for V8, R6, V2 patterns)
  - Multi-channel audio mixer with volume envelopes and loop regions (`mixer.rs`)
- **`rc-core`** *(Phase 2)*: Vehicle physics (`EngineSim`), Ackermann / skid-steer motion control, and lighting sequencer.
- **`rc-boards`** *(Phase 3)*: Board pinouts and hardware profiles (`TRACKLINK_V3`, `MIKRO_V2`, `GTRACK`, `RK_ULTRA`).
- **`rc-brain`** *(Phase 4+)*: ESP32-S3 firmware with dual-core Embassy executor, I2S DMA streaming, and RadioKit BLE.

---

## Testing on Host

Run the unit tests and audio rendering test:
```bash
cargo test --workspace
```

Verify bare-metal `no_std` compilation for embedded targets:
```bash
cargo check --workspace --no-default-features --target riscv32imc-unknown-none-elf
```
