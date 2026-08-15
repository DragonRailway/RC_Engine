#pragma once

// Source files: https://oshwlab.com/shreeramlive/tracklink-v3
// Store link: https://www.elecrow.com/tracklink-v3-dragon-railway.html

// PIN definitions for TrackLink V3 board

struct PIN {
  static constexpr uint8_t L0 = 42; // BUILTIN LED
  static constexpr uint8_t L1 = 6;
  static constexpr uint8_t L2 = 7;
  static constexpr uint8_t L3 = 8;
  static constexpr uint8_t L4 = 9;
  static constexpr uint8_t L5 = 10;
  static constexpr uint8_t L6 = 11;
  static constexpr uint8_t S1 = 1;
  static constexpr uint8_t S2 = 2;
};

struct DRIVER {
  struct A {
    static constexpr uint8_t PWM1 = 13;
    static constexpr uint8_t PWM2 = 14;
    static constexpr uint8_t BEMF = 4;
  };
  struct B {
    static constexpr uint8_t DIR = 15;
    static constexpr uint8_t PWM = 16;
    static constexpr uint8_t BEMF = 5;
  };
  static constexpr uint8_t COMMON_EN = 12;
};

struct AUDIO {
  static constexpr uint8_t I2S_LRC = 17;
  static constexpr uint8_t I2S_BCLK = 18;
  static constexpr uint8_t I2S_DIN = 21;
  static constexpr uint8_t I2S_SD = 47;
};

struct POWER {
  static constexpr uint8_t POWER_ENABLE = 33; // set high to enable board
  static constexpr uint8_t POWER_BUTTON = 48; // Detect power button press
  static constexpr uint8_t VOLTAGE_SENS = 4;   // Voltage sense pin
  static constexpr uint8_t CHARGE_SENS = 34; // Charging status indicator
};

