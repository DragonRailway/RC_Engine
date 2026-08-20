#ifndef RADIOKIT_UI_H
#define RADIOKIT_UI_H

#include <FS.h>
#include <LittleFS.h>
#include <RadioKitLib.h>

// Simple: one slider, one text
RK_Slider testSlider(100, 70, 26, 147);
RK_Text   testText(139, 28, 16, 67);

static inline void initRadioKit() {
  RadioKit.config.name      = "IsoTest";
  RadioKit.config.theme     = "dragon";
  RadioKit.config.baudrate  = 1000000;

  testSlider.rk.label = "testSlider";
  testText.rk.label   = "testText";
  testText.rk.content = "ready";

  RadioKit.begin();
  RadioKit.startSerial(Serial);
  RadioKit.startBLE();
}

#endif
