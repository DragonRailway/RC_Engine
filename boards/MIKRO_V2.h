
// PIN definitions for Mikro V2 board

struct PIN {
  static constexpr uint8_t L0 = 36;
  static constexpr uint8_t L1 = 38;
  static constexpr uint8_t L2 = 39;
  static constexpr uint8_t L3 = 40;
  static constexpr uint8_t L4 = 41;
  static constexpr uint8_t L5 = 42;
  static constexpr uint8_t L6 = 43;
  static constexpr uint8_t L7 = 1;
  static constexpr uint8_t L8 = 2;
  static constexpr uint8_t S1 = 5;
  static constexpr uint8_t S2 = 6;
  static constexpr uint8_t S3 = 7;
  static constexpr uint8_t S4 = 8;
};

struct DRIVER {
  struct A {
    static constexpr uint8_t PWM1 = 18;
    static constexpr uint8_t PWM2 = 21;
    static constexpr uint8_t EN = 17;
    static constexpr uint8_t BEMF = 9;
  };
  struct B {
      static constexpr uint8_t PWM1 = 12;
      static constexpr uint8_t PWM2 = 13;
      static constexpr uint8_t EN = 11;
      static constexpr uint8_t BEMF = 10;
  };
};

struct AUDIO {
  static constexpr uint8_t I2S_LRC = 48;
  static constexpr uint8_t I2S_BCLK = 47;
  static constexpr uint8_t I2S_DIN = 33;
  static constexpr uint8_t I2S_SD = 34;
};

struct POWER {
  static constexpr uint8_t POWER_ENABLE = 15; // set high to enable board
  static constexpr uint8_t POWER_BUTTON = 14; // Detect power button press
  static constexpr uint8_t VOLTAGE_SENS = 4;   // Voltage sense pin
  static constexpr uint8_t CHARGE_SENS = 35; // Charging status indicator
};

