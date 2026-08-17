//__RadioKit_Generated_Code__
//__Might_Be_Overwritten_

#ifndef RADIOKIT_GENERATED_H
#define RADIOKIT_GENERATED_H

#ifndef RK_ENABLE_OTA
#define RK_ENABLE_OTA
#endif
#ifndef RK_ENABLE_FS
#define RK_ENABLE_FS
#endif
#ifndef RK_ENABLE_BLE
#define RK_ENABLE_BLE
#endif

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

RK_GasPedal gas_pedal(50, 59, 56, 20, 0);  // slider: pos=(50,59) size=20x56 label="gas_pedal"

RK_GasPedal brake_pedal(21, 69, 33, 32, 0);  // slider: pos=(21,69) size=32x33 label="brake_pedal"

RK_MultipleSelect truck_light(97, 81, 21, 70, 0);  // multiple: pos=(97,81) size=70x21 label="truck_light"

RK_ToggleButton start_button(97, 51, 20, 0, 0);  // button: pos=(97,51) size=?x20 label="start_button"

RK_ToggleButton left_indicator(136, 16, 13, 0, 0);  // button: pos=(136,16) size=?x13 label="left_indicator"

RK_ToggleButton right_indicator(190, 16, 13, 0, 0);  // button: pos=(190,16) size=?x13 label="right_indicator"

RK_Slider aux_slider(121, 49, 40, 12, 0);  // slider: pos=(121,49) size=12x40 label="aux_slider"

RK_PushButton horn_button(163, 15, 12, 0, 0);  // button: pos=(163,15) size=?x12 label="horn_button"

RK_MultipleButton gear_switch(72, 50, 40, 18, 0);  // multiple: pos=(72,50) size=18x40 label="gear_switch"

RK_SerialMonitor serial_monitor_1(97, 15, 30, 58);  // serialMonitor: pos=(97,15) size=58x30 label="serial_monitor_1"

// ─── Page 1: Loco ───
RK_Slider throttle_slider(74, 98, 95, 22, 0);  // slider: pos=(74,98) size=22x95 label="throttle_slider"

RK_SlideSwitch dir_switch(73, 162, 18, 32, 0);  // switch: pos=(73,162) size=32x18 label="dir_switch"

RK_MultipleSelect loco_light(29, 98, 83, 29, 0);  // multiple: pos=(29,98) size=29x83 label="loco_light"

RK_PushButton bell_button(29, 161, 20, 0, 0);  // button: pos=(29,161) size=?x20 label="bell_button"

RK_ToggleButton engine_button(29, 38, 20, 0, 0);  // button: pos=(29,38) size=?x20 label="engine_button"

RK_SerialMonitor serial_monitor_2(71, 38, 25, 57);  // serialMonitor: pos=(71,38) size=57x25 label="serial_monitor_2"

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
  truck_light.rk.items[0].label = "Low";
  truck_light.rk.items[0].pos = 0;
  truck_light.rk.items[1].label = "High";
  truck_light.rk.items[1].pos = 1;
  truck_light.rk.items[2].label = "Fog";
  truck_light.rk.items[2].pos = 2;
  truck_light.rk.itemCount = 3;
  start_button.rk.label = "start_button";
  start_button.setLabelHidden(true);
  start_button.rk.onText = "START";
  start_button.rk.offText = "STOP";
  left_indicator.rk.label = "left_indicator";
  left_indicator.setLabelHidden(true);
  left_indicator.rk.icon = "arrow-left";
  right_indicator.rk.label = "right_indicator";
  right_indicator.setLabelHidden(true);
  right_indicator.rk.icon = "arrow-right";
  aux_slider.rk.label = "aux_slider";
  aux_slider.setLabelHidden(true);
  aux_slider.rk.centering = RK_SPRING_MIN;
  aux_slider.rk.detents = 5;
  horn_button.rk.label = "horn_button";
  horn_button.setLabelHidden(true);
  horn_button.rk.icon = "bell";
  gear_switch.rk.label = "gear_switch";
  gear_switch.setLabelHidden(true);
  gear_switch.rk.items[0].label = "D";
  gear_switch.rk.items[0].pos = 0;
  gear_switch.rk.items[1].label = "P";
  gear_switch.rk.items[1].pos = 1;
  gear_switch.rk.items[2].label = "R";
  gear_switch.rk.items[2].pos = 2;
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
  dir_switch.rk.onText = "ON";
  dir_switch.rk.offText = "OFF";
  loco_light.setPage(1);
  loco_light.rk.label = "loco_light";
  loco_light.setLabelHidden(true);
  loco_light.rk.items[0].label = "A";
  loco_light.rk.items[0].pos = 0;
  loco_light.rk.items[1].label = "B";
  loco_light.rk.items[1].pos = 1;
  loco_light.rk.items[2].label = "C";
  loco_light.rk.items[2].pos = 2;
  loco_light.rk.itemCount = 3;
  bell_button.setPage(1);
  bell_button.rk.label = "bell_button";
  bell_button.setLabelHidden(true);
  bell_button.rk.onText = "ON";
  bell_button.rk.offText = "OFF";
  engine_button.setPage(1);
  engine_button.rk.label = "engine_button";
  engine_button.setLabelHidden(true);
  engine_button.rk.onText = "ON";
  engine_button.rk.offText = "OFF";
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

