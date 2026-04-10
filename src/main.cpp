#include <Arduino.h>
#include "driver/i2s.h"
#include "RcEngineSound.h"
#include "ConfigLoader.h"

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
      // Periodic heartbeat/check
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Unified RC Sound System ===");

  // Check PSRAM
  if (psramInit()) {
    Serial.printf("PSRAM initialized. Total: %d, Free: %d\n", ESP.getPsramSize(), ESP.getFreePsram());
  } else {
    Serial.println("PSRAM initialization FAILED! Static sounds fallback not implemented.");
  }

  // Enable DAC power
  pinMode(I2S_SD_MODE, OUTPUT);
  digitalWrite(I2S_SD_MODE, HIGH);

  // Mount Filesystem
  if (!ConfigLoader::begin()) {
    Serial.println("Critial Failure: Could not mount LittleFS.");
    while(1) delay(100);
  }

  // Load Configuration and Sounds
  RcEngineSound::Config cfg;
  SoundData vehicleData;

  // 1. Load settings from JSON
  if (!ConfigLoader::loadConfig("/config.json", cfg)) {
    Serial.println("Warning: /config.json not found, using defaults.");
  }

  // 2. Load RAW samples from /sounds/ into PSRAM
  if (!ConfigLoader::loadAllSounds(vehicleData)) {
    Serial.println("Critical Failure: Could not load sound samples from LittleFS.");
    while(1) delay(100);
  }

  engine.begin(vehicleData, cfg);

  setupI2S();
  xTaskCreatePinnedToCore(audioTask, "AudioTask", 4096, NULL, 5, NULL, 0);

  Serial.println("System Ready. Starting engine simulation...");
  engine.startEngine();
}

void loop() {
  static int throttle = 0;
  static int phase = 0;
  static uint32_t phaseTimer = millis();
  uint32_t elapsed = millis() - phaseTimer;

  // Simple demo scenario: Startup -> Idle -> Rev -> Shutdown
  switch (phase) {
  case 0: // Idle for 5s
    throttle = 0;
    if (elapsed > 5000) {
      phase = 1; phaseTimer = millis();
      Serial.println(">> Accelerating...");
    }
    break;
  case 1: // Rev up
    throttle = min(500, (int)(elapsed / 8));
    if (throttle >= 500) {
      phase = 2; phaseTimer = millis();
      Serial.println(">> Cruising...");
    }
    break;
  case 2: // Horn test
    throttle = 500;
    engine.triggerHorn(true);
    if (elapsed > 2000) {
      phase = 3; phaseTimer = millis();
      engine.triggerHorn(false);
      Serial.println(">> Decelerating...");
    }
    break;
  case 3: // Rev down
    throttle = max(0, 500 - (int)(elapsed / 5));
    if (throttle <= 0) {
      phase = 4; phaseTimer = millis();
      Serial.println(">> Shutting down...");
      engine.stopEngine();
    }
    break;
  case 4: // Reset cycle
    if (elapsed > 10000) {
      phase = 0; phaseTimer = millis();
      Serial.println(">> Restarting engine...");
      engine.startEngine();
    }
    break;
  }

  engine.update(throttle);

  static uint32_t logTimer = 0;
  if (millis() - logTimer > 1000) {
    logTimer = millis();
    Serial.printf("State:%d RPM:%d FreePSRAM:%d\n", 
                  engine.getState(), engine.getRpm(), ESP.getFreePsram());
  }
  delay(20);
}
