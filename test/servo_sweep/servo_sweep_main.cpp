#include <Arduino.h>
#include <ESP32_EasyKit.h>

#define SERVO_PIN        5   // S1 on MIKRO_V2
#define POWER_LATCH_PIN 15   // Power latch / 5V rail enable on MIKRO_V2

EasyServo servo;

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Latch power / enable 5V rail on MIKRO_V2
    pinMode(POWER_LATCH_PIN, OUTPUT);
    digitalWrite(POWER_LATCH_PIN, HIGH);

    Serial.println("\n========================================================");
    Serial.println("   MIKRO_V2 Servo 20-Degree Sweep Test (80 MHz CPU)   ");
    Serial.println("========================================================");
    Serial.printf("CPU Frequency:   %u MHz\n", getCpuFrequencyMhz());
    Serial.printf("XTAL Frequency:  %u MHz\n", getXtalFrequencyMhz());
    Serial.printf("APB Frequency:   %u Hz\n", getApbFrequency());
    Serial.printf("Servo Pin:       GPIO %d\n", SERVO_PIN);
    Serial.printf("Power Latch Pin: GPIO %d (HIGH)\n", POWER_LATCH_PIN);

    // Attach servo on GPIO 5 (50 Hz, 500-2500 µs pulse range)
    int attachedPin = servo.attach(SERVO_PIN);
    if (attachedPin == 0) {
        Serial.println("ERROR: Servo attach failed!");
        while (true) {
            delay(1000);
        }
    }

    // Set easing speed for smooth sweep (40 deg/s)
    servo.setSpeed(40.0f, 0.3f, 0.7f);
    servo.write(80.0f); // Center - 10°

    Serial.println("Servo attached successfully! Starting 20-degree sweep (80° <-> 100°)...");
}

void loop() {
    static uint32_t lastTargetSwitch = 0;
    static bool forward = true;
    static uint32_t lastLog = 0;

    // Update non-blocking easing engine
    servo.update();

    // Switch target every 2 seconds: sweep 20 degrees between 80° and 100°
    if (millis() - lastTargetSwitch >= 2000) {
        lastTargetSwitch = millis();
        forward = !forward;
        float targetAngle = forward ? 100.0f : 80.0f;
        servo.write(targetAngle);
        Serial.printf("\n>>> Target Commanded: %.1f° (20° span)\n", targetAngle);
    }

    // Periodic telemetry log every 500 ms
    if (millis() - lastLog >= 500) {
        lastLog = millis();
        Serial.printf("[Telemetry] Angle: %5.1f° | Pulse: %4u µs | CPU: %u MHz | XTAL: %u MHz\n",
                      servo.readAngle(),
                      servo.readMicroseconds(),
                      getCpuFrequencyMhz(),
                      getXtalFrequencyMhz());
    }
}
