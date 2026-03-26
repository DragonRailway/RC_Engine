#include <Arduino.h>
#include "driver/i2s.h"
#include "RcEngineSound.h"

/**
 * INTEGRATED VEHICLE PRESETS:
 * 
 * Since you have copied all presets to 'lib/RcEngineSound/src/vehicles',
 * they are now part of the library.
 * 
 * Each preset file (e.g., ScaniaV8.h) defines its own sound assets.
 * To avoid symbol conflicts, we recommend including only ONE vehicle preset at a time.
 */

// Choose your vehicle preset
#include "vehicles/ScaniaV8.h"

// MAX98357A Pins (Lolin S3 Mini)
#define I2S_BCLK 41
#define I2S_LRC 42
#define I2S_DOUT 40

RcEngineSound engine;

void setupI2S() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 22050, 
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT, // Mono
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 128,
        .use_apll = false
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRC,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
}

void audioTask(void *pvParameters) {
    int16_t sample16;
    size_t bytes_written;

    while (1) {
        uint8_t sample8 = engine.getNextSample();
        // Convert 8-bit unsigned (centered at 128) to 16-bit signed
        sample16 = (int16_t)(sample8 - 128) << 8;
        
        i2s_write(I2S_NUM_0, &sample16, sizeof(sample16), &bytes_written, portMAX_DELAY);
    }
}

void setup() {
    Serial.begin(115200);
    
    // 1. Pack the sounds from the included vehicle header into SoundData
    SoundData myVehicle = {
        .samples = (const int8_t*)samples, // Defined in ScaniaV8idle.h (via ScaniaV8.h)
        .sampleCount = sampleCount,
        .sampleRate = 22050,
        
        .startSamples = (const int8_t*)startSamples, // Defined in ScaniaV8start.h
        .startSampleCount = startSampleCount,
        
        .revSamples = (const int8_t*)revSamples, // Defined in ScaniaV8rev.h
        .revSampleCount = revSampleCount,
        
        .hornSamples = (const int8_t*)hornSamples, // Defined in ScaniaV8trainHorn.h
        .hornSampleCount = hornSampleCount,
        .hornSampleRate = 22050
    };

    // 2. Configure engine parameters using values from ScaniaV8.h
    RcEngineSound::Config cfg;
    cfg.acc = acc; // From ScaniaV8.h
    cfg.dec = dec; // From ScaniaV8.h
    cfg.idleVolume = idleVolumePercentage;
    cfg.revVolume = revVolumePercentage;
    cfg.startVolume = startVolumePercentage;
    cfg.revSwitchPoint = revSwitchPoint;
    cfg.idleEndPoint = idleEndPoint;

    // 3. Initialize and start
    engine.begin(myVehicle, cfg);
    engine.startEngine();

    setupI2S();
    xTaskCreatePinnedToCore(audioTask, "AudioTask", 4096, NULL, 5, NULL, 0);
}

void loop() {
    // Read your RC receiver here (SBUS/PWM) and map to 0-500
    // For this demo, we simulate a recurring throttle sweep.
    static int throttle = 0;
    static int dir = 2;
    throttle += dir;
    if (throttle >= 500) { dir = -2; engine.triggerHorn(true); }
    if (throttle <= 0) { dir = 2; engine.triggerHorn(false); }

    engine.update(throttle);
    
    Serial.printf("Throttle: %d, RPM: %d\n", throttle, engine.getRpm());
    delay(20);
}