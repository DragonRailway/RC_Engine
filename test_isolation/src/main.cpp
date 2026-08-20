#include <Arduino.h>
#include "RADIOKIT.h"

static char _textBuf[16];
static int  lastVal = -1;

void setup() {
    Serial.setRxBufferSize(8192);
    Serial.begin(2000000);
    delay(1000);
    Serial.println("=== Isolation Test ===");

    initRadioKit();
    Serial.println("=== Ready ===");
}

void loop() {
    RadioKit.update();

    // Read slider and update text on change
    int val = testSlider.rk.value;
    if (val != lastVal) {
        lastVal = val;
        snprintf(_textBuf, sizeof(_textBuf), "%d", val);
        testText.rk.content = _textBuf;
    }
}
