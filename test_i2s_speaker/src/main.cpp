// Minimal I2S speaker test for TRACKLINK_V3
// Plays a 440 Hz sine wave to verify DAC/speaker hardware
// Pins: I2S_LRC=17, I2S_BCLK=18, I2S_DIN=21, I2S_SD=47

#include <Arduino.h>
#include <driver/i2s_std.h>

// TRACKLINK_V3 I2S pins
#define I2S_LRC  17
#define I2S_BCLK 18
#define I2S_DIN  21
#define I2S_SD   47

static i2s_chan_handle_t tx_handle;

void setup() {
    Serial.begin(2000000);
    delay(1000);
    Serial.println("=== I2S Speaker Test (TRACKLINK_V3) ===");
    Serial.printf("Pins: LRC=%d, BCLK=%d, DIN=%d, SD=%d\n", I2S_LRC, I2S_BCLK, I2S_DIN, I2S_SD);

    // Enable DAC amp
    pinMode(I2S_SD, OUTPUT);
    digitalWrite(I2S_SD, HIGH);
    Serial.println("[1] I2S_SD (amp enable) set HIGH");

    // Allocate I2S channel
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;

    esp_err_t err = i2s_new_channel(&chan_cfg, &tx_handle, NULL);
    if (err != ESP_OK) {
        Serial.printf("[FAIL] i2s_new_channel: %d\n", err);
        while (1) delay(1000);
    }
    Serial.printf("[2] I2S channel allocated OK\n");

    // Configure standard I2S mode (Philips, 16-bit, stereo, 22050 Hz)
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(22050),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)I2S_BCLK,
            .ws   = (gpio_num_t)I2S_LRC,
            .dout = (gpio_num_t)I2S_DIN,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false }
        }
    };

    err = i2s_channel_init_std_mode(tx_handle, &std_cfg);
    if (err != ESP_OK) {
        Serial.printf("[FAIL] i2s_channel_init_std_mode: %d\n", err);
        while (1) delay(1000);
    }
    Serial.printf("[3] I2S standard mode initialized OK\n");

    err = i2s_channel_enable(tx_handle);
    if (err != ESP_OK) {
        Serial.printf("[FAIL] i2s_channel_enable: %d\n", err);
        while (1) delay(1000);
    }
    Serial.printf("[4] I2S channel enabled OK\n");

    Serial.println("--- All I2S setup succeeded. Playing 440 Hz sine wave... ---");
}

void loop() {
    // Generate one buffer of 440 Hz sine wave (64 frames, stereo, 16-bit)
    static const int SAMPLE_RATE = 22050;
    static const int BUF_FRAMES = 64;
    static const float FREQ = 440.0f;
    static uint32_t pos = 0;

    int16_t buf[BUF_FRAMES * 2];

    for (int i = 0; i < BUF_FRAMES; i++) {
        float t = (float)(pos + i) / (float)SAMPLE_RATE;
        int16_t sample = (int16_t)(sinf(2.0f * M_PI * FREQ * t) * 15000.0f);
        buf[i * 2]     = sample;
        buf[i * 2 + 1] = sample;
    }
    pos += BUF_FRAMES;

    // Write to I2S with bounded timeout (not portMAX_DELAY!)
    size_t bytes_written = 0;
    int64_t t_before = esp_timer_get_time();
    esp_err_t err = i2s_channel_write(tx_handle, buf, sizeof(buf), &bytes_written, pdMS_TO_TICKS(100));
    int64_t t_after = esp_timer_get_time();

    // Log every ~1 second (22050 samples/sec ÷ 64 frames/buf ≈ 344 buf/sec)
    static uint32_t logCounter = 0;
    logCounter++;
    if (logCounter % 344 == 0) {
        uint32_t elapsed_us = (uint32_t)(t_after - t_before);
        Serial.printf("[PLAYING] pos=%lu, write_us=%lu, bytes=%d, err=%d\n",
                      pos, elapsed_us, (int)bytes_written, err);
    }
}
