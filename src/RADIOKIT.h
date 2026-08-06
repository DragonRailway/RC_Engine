//__RadioKit_Generated_Code__
//__Might_Be_Overwritten_

#ifndef RC_BRAIN_RADIOKIT_H
#define RC_BRAIN_RADIOKIT_H

#define RK_ENABLE_OTA
#define RK_ENABLE_FS
#define RK_ENABLE_BLE

#include <RadioKitLib.h>

#define RK_NUM_PAGES 2
static const char* rk_pageNames[] = {
  "Truck",
  "Loco",
};

// ─── Page 0: Truck ───
RK_Knob steering_wheel(164, 52, 64, 0, 0);  // knob: pos=(164,52) size=?x64 label="steering_wheel"

RK_GasPedal gas_pedal(21, 54, 58, 21, 0);  // slider: pos=(21,54) size=21x58 label="gas_pedal"

RK_GasPedal brake_pedal(48, 62, 41, 20, 0);  // slider: pos=(48,62) size=20x41 label="brake_pedal"

RK_MultipleSelect led_select(95, 58, 21, 0, 0);  // multiple: pos=(95,58) size=?x21 label="led_select"

// ─── Page 1: Loco ───
RK_Slider slider(74, 98, 95, 22, 0);  // slider: pos=(74,98) size=22x95 label="slider"

RK_SlideSwitch dir_switch(73, 162, 18, 32, 0);  // switch: pos=(73,162) size=32x18 label="dir_switch"

RK_MultipleSelect lights_toggle(26, 95, 26, 0, 0);  // multiple: pos=(26,95) size=?x26 label="lights_toggle"

RK_PushButton horn(26, 159, 20, 0, 0);  // button: pos=(26,159) size=?x20 label="horn"

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
  steering_wheel.rk.centering = RK_SPRING_CENTER;
  steering_wheel.rk.startAngle = -135.0;
  steering_wheel.rk.endAngle = 135.0;
  gas_pedal.rk.label = "gas_pedal";
  gas_pedal.setLabelHidden(true);
  gas_pedal.rk.centering = RK_SPRING_CENTER;
  brake_pedal.rk.label = "brake_pedal";
  brake_pedal.setLabelHidden(true);
  brake_pedal.rk.centering = RK_SPRING_CENTER;
  led_select.rk.label = "led_select";
  led_select.setLabelHidden(true);
  led_select.rk.items[0].label = "A";
  led_select.rk.items[0].pos = 0;
  led_select.rk.items[1].label = "B";
  led_select.rk.items[1].pos = 1;
  led_select.rk.items[2].label = "C";
  led_select.rk.items[2].pos = 2;
  led_select.rk.items[3].label = "D";
  led_select.rk.items[3].pos = 3;
  led_select.rk.items[4].label = "E";
  led_select.rk.items[4].pos = 4;
  led_select.rk.itemCount = 5;
  slider.rk.label = "slider";
  slider.setLabelHidden(true);
  slider.rk.centering = RK_SPRING_NONE;
  dir_switch.setPage(1);
  dir_switch.rk.label = "dir_switch";
  dir_switch.setLabelHidden(true);
  lights_toggle.rk.label = "lights_toggle";
  lights_toggle.setLabelHidden(true);
  lights_toggle.rk.items[0].label = "A";
  lights_toggle.rk.items[0].pos = 0;
  lights_toggle.rk.items[1].label = "B";
  lights_toggle.rk.items[1].pos = 1;
  lights_toggle.rk.items[2].label = "C";
  lights_toggle.rk.items[2].pos = 2;
  lights_toggle.rk.items[3].label = "D";
  lights_toggle.rk.items[3].pos = 3;
  lights_toggle.rk.items[4].label = "E";
  lights_toggle.rk.items[4].pos = 4;
  lights_toggle.rk.itemCount = 5;
  // RK_PushButton is momentary by construction; no mode switch needed
  horn.rk.label = "horn";
  horn.setPage(1);
  horn.rk.label = "horn";
  horn.setLabelHidden(true);
  telemetry_Battery.rk.icon = "battery";
  telemetry_Battery.rk.unit = "%";
  telemetry_Battery.rk.content = "--";
  telemetry_Speed.rk.icon = "gauge";
  telemetry_Speed.rk.content = "--";

  RadioKit.setNumPages(RK_NUM_PAGES);
  RadioKit.setPageNames(rk_pageNames);

  RadioKit.begin();

  RadioKit.startSerial(Serial);
  RadioKit.startBLE();

  RadioKit.enableFS();
}

#endif // RC_BRAIN_RADIOKIT_H
