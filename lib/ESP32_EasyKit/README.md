# ESP32_PWM_Fusion: Advanced Servo, Motor, and LED Library

[![arduino-library-badge](https://www.ardu-badge.com/badge/ESP32_EasyKit.svg?)](https://www.ardu-badge.com/ESP32_EasyKit)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**ESP32_PWM_Fusion** is a high-performance C++17 library for the ESP32 series that leverages native silicon features (MCPWM and LEDC) to provide rock-solid, hardware-locked control for robots, lighting, and RC applications.

---

## Why use this library?

*   **⚡ Hardware-Locked Stability**: Uses dedicated MCPWM units for sub-microsecond pulse precision with zero CPU jitter.
*   **🤖 Organic Motion**: Advanced **Asymmetric Sigmoid** easing and **Momentum Hand-off** ensure professional, jerk-free movements for servos.
*   **🏎️ Low Latency**: Direct-drive motor control with hardware-enforced dead-time for high-performance control loops.
*   **💎 FPU Optimized**: Designed from the ground up for the ESP32 hardware Floating Point Unit.

---

## Quick Example

```cpp
#include <ESP32_EasyKit.h>

Servo myServo;
EasyMotor myMotor;
EasyLED myLED(6);

void setup() {
  myServo.attach(18);       
  myMotor.begin(7, 8);      
  
  // Asymmetric move: Go to 90°, rapid start, soft finish (0.2 in, 0.8 out)
  myServo.write(90.0f, 60.0f, 0.2f, 0.8f); 
  
  // Instant motor control (Direct-Drive)
  myMotor.write(100.0f);
}

void loop() {
  myServo.update(); // Process servo animation
  myLED.update();   // Process LED fading/blinking
}
```

---

## Documentation

For a deep dive into the library architecture and performance tuning, start with the **[Introduction](docs/INTRODUCTION.md)**.

### API Reference
- 📑 [**Servo Functions**](docs/SERVO_Functions.md): Precise motion with asymmetric easing.
- 📑 [**DC Motor Functions**](docs/DCMOTOR_Functions.md): Low-latency direct-drive control.
- 📑 [**LED & PWM Functions**](docs/LED_Functions.md): Flicker-free transitions and patterns.

---

## Performance Tip

To maximize performance, always use the `f` suffix for decimal constants:
```cpp
myServo.write(90.0f, 60.0f, 0.5f, 0.5f); // Hardware FPU (Fast)
myServo.write(90.0, 60.0, 0.5, 0.5);     // Software Emulation (Slow)
```

For more details on forcing FPU safety with compiler flags, see the **[Optimization Guide](docs/INTRODUCTION.md#high-performance-fpu-optimization)**.

---

Licensed under the **MIT License**.
