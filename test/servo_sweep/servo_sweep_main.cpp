#include <Arduino.h>
#include <ESP32_EasyKit.h>

  #define SERVO_PIN        5   // S1 on MIKRO_V2

EasyServo servo;

// Strict 20-degree safe sweep range: 80° to 100° (center = 90°)
const float MIN_ANGLE = 80.0f;
const float MAX_ANGLE = 100.0f;
const float CENTER_ANGLE = 90.0f;

void setup() {
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);
    delay(500);

    Serial.println("\n========================================================");
    Serial.println("   MIKRO_V2 Safe 20° Servo Sweep (80° <-> 100°)        ");
    Serial.println("========================================================");
    Serial.printf("CPU Frequency:   %u MHz\n", getCpuFrequencyMhz());
    Serial.printf("XTAL Frequency:  %u MHz\n", getXtalFrequencyMhz());
    Serial.printf("Servo Pin:       GPIO %d (S1)\n", SERVO_PIN);
    Serial.printf("Safe Range:      %.1f° to %.1f° (Total Span: 20°)\n", MIN_ANGLE, MAX_ANGLE);

    // Attach servo on GPIO 5
    int attached = servo.attach(SERVO_PIN);
    if (!attached) {
        Serial.println("ERROR: Servo attach failed!");
        while (true) {
            delay(1000);
        }
    }

    // Set gentle easing speed (30 deg/s) with smooth S-curve
    servo.setSpeed(30.0f, 0.3f, 0.7f);
    servo.write(CENTER_ANGLE);

    Serial.println("Servo attached at center (90°). Starting 20° safe sweep...");
}

void loop() {
    static uint32_t lastMove = 0;
    static bool flip = false;
    static uint32_t lastPrint = 0;

    // Update non-blocking easing engine
    servo.update();

    // Alternate every 1.5 seconds between 80° and 100° (20° total travel)
    if (millis() - lastMove >= 1500) {
        lastMove = millis();
        flip = !flip;
        float target = flip ? MAX_ANGLE : MIN_ANGLE; // 100° or 80°
        servo.write(target);
        Serial.printf("\n>>> Safe Move Target: %.1f°\n", target);
    }

    // Telemetry printout
    if (millis() - lastPrint >= 500) {
        lastPrint = millis();
        Serial.printf("[Servo S1] Angle: %5.1f° | Pulse: %4u µs\n",
                      servo.readAngle(), servo.readMicroseconds());
    }
}
