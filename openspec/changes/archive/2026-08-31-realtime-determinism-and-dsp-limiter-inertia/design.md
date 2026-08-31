## Context

The ESP32-S3 runs FreeRTOS with dual Xtensa LX7 cores at 240 MHz. Currently:
- Core 0 runs the BLE stack and radio events.
- Core 1 runs the real-time audio synthesis task (`audioTask` at Priority 3) and the Arduino `loop()` (Priority 1).
- `VehicleController::update()`, `HardwareInit::update()`, and `RadioKit.update()` execute sequentially in `loop()`, subject to variable execution delays when BLE packets arrive.

## Goals / Non-Goals

**Goals:**
- Decouple vehicle control and animation updates from BLE serial/radio polling by placing them in a dedicated FreeRTOS task with exact 20.0 ms period (`vTaskDelayUntil()`).
- Replace periodic filesystem `stat()` calls with event-driven hot-reload triggering.
- Eliminate audio digital clipping distortion via a fast cubic soft-knee saturator in `renderBlock()`.
- Upgrade `RcEngineSound` flywheel inertia from integer step division to continuous float exponential smoothing.

**Non-Goals:**
- Changing RadioKit protocol framing or BLE GATT characteristic UUIDs.
- Modifying sound asset formats (.pcm layout remains unchanged).

## Decisions

### 1. FreeRTOS Task Scheduling & Prioritization on Core 1
- **Decision**: 
  - `audioTask` (Priority 3, Core 1): 22,050 Hz block rendering (consumes ~3-5% CPU).
  - `controlTask` (Priority 2, Core 1): Strict 50 Hz (20 ms) loop for `VehicleController::update()` and `HardwareInit::update()` (consumes ~0.5% CPU).
  - `loopTask` (Priority 1, Core 1): `RadioKit.update()`, BLE telemetry transport, and asynchronous config reload processing.
- **Rationale**: Ensures the control loop and physics integration never suffer jitter from BLE packets or serial RPC handlers.

### 2. Polynomial Soft-Knee Limiter Formulation
- **Decision**: Evaluate the saturator on the normalized sample $x = \text{mixAccum} / 32768.0f$:
  $$y(x) = \begin{cases}
  x & |x| \le \frac{2}{3} \\
  \text{sgn}(x) \cdot \left(\frac{3 - (2 - 3|x|)^2}{3}\right) & \frac{2}{3} < |x| < 1.0 \\
  \text{sgn}(x) \cdot 1.0 & |x| \ge 1.0
  \end{cases}$$
- **Rationale**: Requires only 2 single-cycle FPU instructions when engaged, preserves bit-exact linearity below $-3.5\text{ dBFS}$ ($|x| \le 0.667$), and prevents harsh square-wave clipping on volume bursts.

### 3. Continuous Exponential RPM Inertia Differential Model
- **Decision**: Model virtual flywheel RPM as:
  $$\text{RPM}(t) = \text{RPM}(t - \Delta t) + (\text{Target} - \text{RPM}(t - \Delta t)) \cdot (1.0f - e^{-\Delta t / \tau})$$
  where $\tau = \text{inertia} \times 0.005\text{s} + \text{accelTime}$.
- **Rationale**: Provides smooth mathematical convergence at any loop rate, eliminating integer division truncation notches at low idle speeds.

## Risks / Trade-offs

- [Risk] Multi-threaded access between `controlTask` (reading vehicle state) and `loopTask` (receiving RadioKit control packets).
  - *Mitigation*: RadioKit widget state writes are atomic 32-bit/float memory operations on ESP32-S3 architecture; existing critical sections protect sound engine voice snapshots.
