#include <Arduino.h>
#include "driver/i2s.h"
#include "RcEngineSound.h"

// Choose your vehicle preset
#include "vehicles/ScaniaV8.h"

RcEngineSound engine;

void setupI2S() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 22050, 
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // Stereo (more robust for many DACs)
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 128,
        .use_apll = false
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRCLK,
        .data_out_num = I2S_DIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
}

void audioTask(void *pvParameters) {
    int16_t samples[2]; // Stereo (Left, Right)
    size_t bytes_written;
    uint32_t total_samples = 0;

    while (1) {
        uint8_t sample8 = engine.getNextSample();
        // Convert 8-bit unsigned (centered at 128) to 16-bit signed
        int16_t sample16 = (int16_t)(sample8 - 128) << 8;
        
        // Duplicate mono sample to both channels
        samples[0] = sample16;
        samples[1] = sample16;
        
        i2s_write(I2S_NUM_0, samples, sizeof(samples), &bytes_written, portMAX_DELAY);
        
        total_samples++;
        if (total_samples % 22050 == 0) {
            Serial.printf("AudioTask: 1 second processed. Sample: %d\n", sample8);
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    // Enable MAX98357A
    pinMode(I2S_SD_MODE, OUTPUT);
    digitalWrite(I2S_SD_MODE, HIGH);

    // 1. Pack the sounds from the included vehicle header into SoundData
    SoundData myVehicle = {
        .samples = (const int8_t*)samples, 
        .sampleCount = sampleCount,
        .sampleRate = 22050,
        
        .startSamples = (const int8_t*)startSamples, 
        .startSampleCount = startSampleCount,
        
        .revSamples = (const int8_t*)revSamples, 
        .revSampleCount = revSampleCount,
        
        .hornSamples = (const int8_t*)hornSamples, 
        .hornSampleCount = hornSampleCount,
        .hornSampleRate = 22050
    };

    // 2. Configure engine parameters using values from ScaniaV8.h
    RcEngineSound::Config cfg;
    cfg.acc = acc; 
    cfg.dec = dec; 
    cfg.idleVolume = idleVolumePercentage;
    cfg.revVolume = revVolumePercentage;
    cfg.startVolume = startVolumePercentage;
    cfg.masterVolume = 100; // Explicitly 100%
    cfg.revSwitchPoint = revSwitchPoint;
    cfg.idleEndPoint = idleEndPoint;

    // 3. Initialize and start
    engine.begin(myVehicle, cfg);
    engine.startEngine();

    setupI2S();
    xTaskCreatePinnedToCore(audioTask, "AudioTask", 4096, NULL, 5, NULL, 0);
}

void loop() {
    // Simulated throttle sweep (0-500)
    static int throttle = 0;
    static int dir = 2;
    throttle += dir;
    if (throttle >= 500) { dir = -2; engine.triggerHorn(true); }
    if (throttle <= 0) { dir = 2; engine.triggerHorn(false); }

    engine.update(throttle);
    
    Serial.printf("Throttle: %d, RPM: %d\n", throttle, engine.getRpm());
    delay(20);
}
