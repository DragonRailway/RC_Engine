#pragma once

// PIN definitions for G-Track Ultimate Control Board for RC Models
// Reference: MAX98357 I2S, TMI8260 H-bridge drivers, 6x Low-side LEDs, 8x Servo/PWM, 4x GPIO

struct PIN {
  // LED Outputs (Low-side MOSFETs)
  static constexpr uint8_t L1 = 47;
  static constexpr uint8_t L2 = 48;
  static constexpr uint8_t L3 = 33;
  static constexpr uint8_t L4 = 34;
  static constexpr uint8_t L5 = 37;
  static constexpr uint8_t L6 = 38;

  // Servo Outputs (5V, up to 4A)
  static constexpr uint8_t S1 = 8;
  static constexpr uint8_t S2 = 9;
  static constexpr uint8_t S3 = 10;
  static constexpr uint8_t S4 = 11;
  static constexpr uint8_t E1 = 12;
  static constexpr uint8_t E2 = 13;
  static constexpr uint8_t E3 = 14;
  static constexpr uint8_t E4 = 15;

  // Extra GPIO (3.3V)
  static constexpr uint8_t A1 = 1;
  static constexpr uint8_t A2 = 2;
  static constexpr uint8_t A3 = 3;
  static constexpr uint8_t A4 = 4;
};

struct DRIVER {
  struct A {
    static constexpr uint8_t PWM1 = 39;
    static constexpr uint8_t PWM2 = 40;
    static constexpr uint8_t BEMF = 5;
  };
  struct B {
    static constexpr uint8_t PWM1 = 41;
    static constexpr uint8_t PWM2 = 42;
    static constexpr uint8_t BEMF = 6;
  };
};

struct AUDIO {
  static constexpr uint8_t I2S_LRC  = 16; // LCLK
  static constexpr uint8_t I2S_BCLK = 17; // BCLK
  static constexpr uint8_t I2S_DIN  = 18; // DIN
  static constexpr uint8_t I2S_SD   = 21; // ENABLE (SD_MODE)
};

struct POWER {
  static constexpr uint8_t POWER_ENABLE = 35;   // PWR_EN: set HIGH to keep board ON
  static constexpr uint8_t POWER_BUTTON = 36;   // PWR_BTN: digital read from MCU
  static constexpr uint8_t VOLTAGE_SENS = 7;    // VSENS: voltage divider 20k/5.1k
  static constexpr uint8_t CHARGE_SENS  = 0xFF; // No dedicated charge sense pin
  static constexpr uint8_t BUCK_5V_EN   = 44;   // 5V_EN: pull LOW to disable 5V buck
  static constexpr uint8_t POWER_OUT    = 43;   // OUT: High-side MOSFET for external loads
};


