//__RadioKit_Generated_Code__
//__Might_Be_Overwritten_

#ifndef RADIOKIT_GENERATED_H
#define RADIOKIT_GENERATED_H

#define RK_ENABLE_OTA
#define RK_ENABLE_FS
#define RK_ENABLE_BLE

#include <RadioKitLib.h>

#define RK_NUM_PAGES 2
static const char* rk_pageNames[] = {
  "Truck",
  "Loco",
};

static const uint8_t rk_pageOrientations[] = {
  0,  // Truck
  1,  // Loco
};

// ─── Page 0: Truck ───
RK_Knob steering_wheel(163, 54, 64, 0, 0);  // knob: pos=(163,54) size=?x64 label="steering_wheel"

RK_GasPedal gas_pedal(49, 55, 56, 20, 0);  // slider: pos=(49,55) size=20x56 label="gas_pedal"

RK_GasPedal brake_pedal(21, 65, 33, 32, 0);  // slider: pos=(21,65) size=32x33 label="brake_pedal"

RK_MultipleSelect truck_light(98, 87, 27, 100, 0);  // multiple: pos=(98,87) size=100x27 label="truck_light"

RK_ToggleButton start_button(98, 56, 20, 0, 0);  // button: pos=(98,56) size=?x20 label="start_button"

RK_ToggleButton left_indicator(136, 16, 13, 0, 0);  // button: pos=(136,16) size=?x13 label="left_indicator"

RK_ToggleButton right_indicator(190, 16, 13, 0, 0);  // button: pos=(190,16) size=?x13 label="right_indicator"

RK_Slider aux_slider(121, 55, 40, 12, 0);  // slider: pos=(121,55) size=12x40 label="aux_slider"

RK_PushButton horn_button(163, 15, 12, 0, 0);  // button: pos=(163,15) size=?x12 label="horn_button"

RK_MultipleButton gear_switch(75, 56, 40, 18, 0);  // multiple: pos=(75,56) size=18x40 label="gear_switch"

RK_SerialMonitor serial_monitor_1(97, 15, 30, 58);  // serialMonitor: pos=(97,15) size=58x30 label="serial_monitor_1"

// ─── Page 1: Loco ───
RK_Slider throttle_slider(67, 105, 114, 20, 0);  // slider: pos=(67,105) size=20x114 label="throttle_slider"

RK_SlideSwitch dir_switch(67, 173, 18, 32, 0);  // switch: pos=(67,173) size=32x18 label="dir_switch"

RK_MultipleSelect loco_light(31, 97, 130, 24, 0);  // multiple: pos=(31,97) size=24x130 label="loco_light"

RK_PushButton bell_button(31, 175, 20, 0, 0);  // button: pos=(31,175) size=?x20 label="bell_button"

RK_ToggleButton engine_button(67, 41, 20, 0, 0);  // button: pos=(67,41) size=?x20 label="engine_button"

RK_SerialMonitor serial_monitor_2(49, 16, 25, 57);  // serialMonitor: pos=(49,16) size=57x25 label="serial_monitor_2"

// ─── Telemetry Widgets ───
RK_Telemetry telemetry_Battery("Battery");
RK_Telemetry telemetry_Speed("Speed");

// ─── Config Init ───
static inline void initRadioKit() {
  RadioKit.config.name        = "RC_UI";
  RadioKit.config.type        = "Locomotive";
  RadioKit.config.theme       = "dragon";
  RadioKit.config.baudrate    = 1000000;

  steering_wheel.rk.label = "steering_wheel";
  steering_wheel.setLabelHidden(true);
  steering_wheel.rk.variant = 1;     // steeringWheel
  steering_wheel.rk.centering = RK_SPRING_NONE;
  steering_wheel.rk.startAngle = -200;
  steering_wheel.rk.endAngle = 200;
  steering_wheel.rk.centerIcon = "renault";
  gas_pedal.rk.label = "gas_pedal";
  gas_pedal.setLabelHidden(true);
  gas_pedal.rk.centering = RK_SPRING_MIN;
  brake_pedal.rk.label = "brake_pedal";
  brake_pedal.setLabelHidden(true);
  brake_pedal.rk.centering = RK_SPRING_MIN;
  truck_light.rk.label = "truck_light";
  truck_light.setLabelHidden(true);
  truck_light.rk.items[0] = {"", "lightbulb", 0};
  truck_light.rk.items[1] = {"", "headlights", 1};
  truck_light.rk.items[2] = {"", "snowflake", 2};
  truck_light.rk.items[3] = {"", "warning", 3};
  truck_light.rk.items[4] = {"", "siren", 4};
  truck_light.rk.items[5] = {"", "car-simple", 5};
  truck_light.rk.items[6] = {"", "wrench", 6};
  truck_light.rk.items[7] = {"", "x", 7};
  truck_light.rk.itemCount = 8;
  start_button.rk.label = "start_button";
  start_button.setLabelHidden(true);
  start_button.rk.onText = "START";
  start_button.rk.offText = "STOP";
  start_button.rk.icon = "refresh-ccw";
  left_indicator.rk.label = "left_indicator";
  left_indicator.setLabelHidden(true);
  left_indicator.rk.icon = "arrow-fat-left";
  right_indicator.rk.label = "right_indicator";
  right_indicator.setLabelHidden(true);
  right_indicator.rk.icon = "arrow-fat-right";
  aux_slider.rk.label = "aux_slider";
  aux_slider.setLabelHidden(true);
  aux_slider.rk.centering = RK_SPRING_MIN;
  aux_slider.rk.detents = 5;
  horn_button.rk.label = "horn_button";
  horn_button.setLabelHidden(true);
  horn_button.rk.icon = "bell-ringing";
  gear_switch.rk.label = "gear_switch";
  gear_switch.setLabelHidden(true);
  gear_switch.rk.items[0] = {"D", nullptr, 0};
  gear_switch.rk.items[1] = {"P", nullptr, 1};
  gear_switch.rk.items[2] = {"R", nullptr, 2};
  gear_switch.rk.itemCount = 3;
  serial_monitor_1.rk.label = "serial_monitor_1";
  serial_monitor_1.setLabelHidden(true);
  throttle_slider.setPage(1);
  throttle_slider.rk.label = "throttle_slider";
  throttle_slider.setLabelHidden(true);
  throttle_slider.rk.centering = RK_SPRING_NONE;
  dir_switch.setPage(1);
  dir_switch.rk.label = "dir_switch";
  dir_switch.setLabelHidden(true);
  loco_light.setPage(1);
  loco_light.rk.label = "loco_light";
  loco_light.setLabelHidden(true);
  loco_light.rk.items[0] = {"", "arrow-up", 0};
  loco_light.rk.items[1] = {"", "arrow-down", 1};
  loco_light.rk.items[2] = {"", "snowflake", 2};
  loco_light.rk.items[3] = {"", "crosshair", 3};
  loco_light.rk.items[4] = {"", "siren", 4};
  loco_light.rk.items[5] = {"", "cable-car", 5};
  loco_light.rk.items[6] = {"", "tablet", 6};
  loco_light.rk.items[7] = {"", "x", 7};
  loco_light.rk.itemCount = 8;
  bell_button.setPage(1);
  bell_button.rk.label = "bell_button";
  bell_button.setLabelHidden(true);
  bell_button.rk.icon = "bell-ringing";
  engine_button.setPage(1);
  engine_button.rk.label = "engine_button";
  engine_button.setLabelHidden(true);
  engine_button.rk.onText = "START";
  engine_button.rk.icon = "power";
  serial_monitor_2.setPage(1);
  serial_monitor_2.rk.label = "serial_monitor_2";
  serial_monitor_2.setLabelHidden(true);
  telemetry_Battery.rk.icon = "battery";
  telemetry_Battery.rk.unit = "%";
  telemetry_Battery.rk.content = "--";
  telemetry_Speed.rk.icon = "gauge";
  telemetry_Speed.rk.content = "--";

  RadioKit.setNumPages(RK_NUM_PAGES);
  RadioKit.setPageNames(rk_pageNames);
  RadioKit.setPageOrientations(rk_pageOrientations);

  RadioKit.begin();

  RadioKit.startSerial(Serial);
  RadioKit.startBLE();

  RadioKit.enableFS();
}

#endif // RADIOKIT_GENERATED_H

