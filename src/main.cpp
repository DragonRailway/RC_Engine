#include "RcEngineSound.h"
#include "driver/i2s.h"
#include <Arduino.h>

// Choose your vehicle preset
#include "vehicles/ScaniaV8.h"

RcEngineSound engine;

void setupI2S() {
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = 22050,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // Stereo
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 128,
      .use_apll = false};

  i2s_pin_config_t pin_config = {.bck_io_num = I2S_BCLK,
                                 .ws_io_num = I2S_LRCLK,
                                 .data_out_num = I2S_DIN,
                                 .data_in_num = I2S_PIN_NO_CHANGE};

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void audioTask(void *pvParameters) {
  int16_t samples[2]; // Stereo (Left, Right)
  size_t bytes_written;
  uint32_t total_samples = 0;

  while (1) {
    uint8_t sample8 = engine.getNextSample();
    int16_t sample16 = (int16_t)(sample8 - 128) << 8;

    samples[0] = sample16;
    samples[1] = sample16;

    i2s_write(I2S_NUM_0, samples, sizeof(samples), &bytes_written,
              portMAX_DELAY);

    total_samples++;
    if (total_samples % 22050 == 0) {
      Serial.printf("AudioTask: 1s | State: %d | Sample: %d\n",
                    engine.getState(), sample8);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Enable MAX98357A
  pinMode(I2S_SD_MODE, OUTPUT);
  digitalWrite(I2S_SD_MODE, HIGH);

  // ── Pack all ScaniaV8 sounds into SoundData ──
  SoundData myVehicle = {.samples = (const int8_t *)samples,
                         .sampleCount = sampleCount,
                         .sampleRate = 22050,

                         .startSamples = (const int8_t *)startSamples,
                         .startSampleCount = startSampleCount,

                         .revSamples = (const int8_t *)revSamples,
                         .revSampleCount = revSampleCount,

                         .turboSamples = (const int8_t *)turboSamples,
                         .turboSampleCount = turboSampleCount,

                         .knockSamples = (const int8_t *)knockSamples,
                         .knockSampleCount = knockSampleCount,

                         .wastegateSamples = (const int8_t *)wastegateSamples,
                         .wastegateSampleCount = wastegateSampleCount,

                         .hornSamples = (const int8_t *)hornSamples,
                         .hornSampleCount = hornSampleCount,
                         .hornSampleRate = 22050};

  // ── Configure using ScaniaV8.h values ──
  RcEngineSound::Config cfg;
  cfg.acc = acc;
  cfg.dec = dec;
  cfg.inertia = 30; // Heavy truck feel
  cfg.masterVolume = 20;
  cfg.startVolume = startVolumePercentage;
  cfg.idleVolume = idleVolumePercentage;
  cfg.revVolume = revVolumePercentage;
  cfg.turboVolume = turboVolumePercentage;
  cfg.knockVolume = dieselKnockVolumePercentage;
  cfg.wastegateVolume = wastegateVolumePercentage;
  cfg.hornVolume = hornVolumePercentage;
  cfg.revSwitchPoint = revSwitchPoint;
  cfg.idleEndPoint = idleEndPoint;

  engine.begin(myVehicle, cfg);

  setupI2S();
  xTaskCreatePinnedToCore(audioTask, "AudioTask", 4096, NULL, 5, NULL, 0);

  delay(500);
  Serial.println("=== Engine Sound System Ready ===");
  Serial.println("Starting engine...");
  engine.startEngine();
}

// ── Simulated RC Input Scenario ──
// Realistic driving pattern: idle → accelerate → cruise → rapid release
// (wastegate) → horn → stop
void loop() {
  static int throttle = 0;
  static int phase = 0;
  static uint32_t phaseTimer = millis();
  uint32_t elapsed = millis() - phaseTimer;

  switch (phase) {
  case 0: // Idle for 3 seconds
    throttle = 0;
    if (elapsed > 3000) {
      phase = 1;
      phaseTimer = millis();
      Serial.println(">> Phase 1: Accelerating");
    }
    break;
  case 1: // Accelerate smoothly to 400
    throttle = min(400, (int)(elapsed / 8));
    if (throttle >= 400) {
      phase = 2;
      phaseTimer = millis();
      Serial.println(">> Phase 2: Cruising at 400");
    }
    break;
  case 2: // Cruise at 400 for 3 seconds
    throttle = 400;
    if (elapsed > 3000) {
      phase = 3;
      phaseTimer = millis();
      Serial.println(">> Phase 3: Rapid throttle release (wastegate!)");
    }
    break;
  case 3: // Rapid throttle drop → triggers wastegate
    throttle = 50;
    if (elapsed > 2000) {
      phase = 4;
      phaseTimer = millis();
      Serial.println(">> Phase 4: Full throttle + Horn");
    }
    break;
  case 4: // Full throttle with horn
    throttle = min(500, (int)(50 + elapsed / 6));
    engine.triggerHorn(true);
    if (throttle >= 500) {
      phase = 5;
      phaseTimer = millis();
      Serial.println(">> Phase 5: Horn off, cruising at max");
    }
    break;
  case 5: // Cruise at max RPM, horn off
    throttle = 500;
    engine.triggerHorn(false);
    if (elapsed > 2000) {
      phase = 6;
      phaseTimer = millis();
      Serial.println(">> Phase 6: Decelerate to idle");
    }
    break;
  case 6: // Decelerate
    throttle = max(0, 500 - (int)(elapsed / 5));
    if (throttle <= 0) {
      phase = 7;
      phaseTimer = millis();
      Serial.println(">> Phase 7: Idle before shutdown");
    }
    break;
  case 7: // Idle for 2 seconds before stopping
    throttle = 0;
    if (elapsed > 2000) {
      phase = 8;
      phaseTimer = millis();
      Serial.println(">> Phase 8: Engine shutdown");
      engine.stopEngine();
    }
    break;
  case 8: // Wait for engine to stop, then restart cycle
    throttle = 0;
    if (elapsed > 5000) {
      phase = 0;
      phaseTimer = millis();
      Serial.println(">> Restarting cycle...");
      engine.startEngine();
    }
    break;
  }

  engine.update(throttle);

  static uint32_t logTimer = 0;
  if (millis() - logTimer > 500) {
    logTimer = millis();
    Serial.printf("Phase:%d Throttle:%d RPM:%d State:%d\n", phase, throttle,
                  engine.getRpm(), engine.getState());
  }
  delay(20);
}
