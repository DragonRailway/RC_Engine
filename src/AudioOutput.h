#pragma once

#include <Arduino.h>
#include <driver/i2s_std.h>
#include <RcEngineSound.h>

class AudioOutput {
public:
    static constexpr int SAMPLE_RATE = 22050;
    static constexpr int BUFFER_SIZE = 64;

    static RcEngineSound* engine;
    static volatile bool active;
    static int16_t buffer[BUFFER_SIZE];
    static volatile uint32_t bufferPos;
    static hw_timer_t* timer;
    static i2s_chan_handle_t tx_handle;

    static void begin(RcEngineSound* eng) {
        engine = eng;
        active = false;
        bufferPos = 0;

        i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
        chan_cfg.auto_clear = true;

        esp_err_t err = i2s_new_channel(&chan_cfg, &tx_handle, NULL);
        if (err != ESP_OK) {
            Serial.printf("[AudioOutput] I2S channel alloc failed: %d\n", err);
            return;
        }

        i2s_std_config_t std_cfg = {
            .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
            .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
            .gpio_cfg = {
                .mclk = I2S_GPIO_UNUSED,
                .bclk = (gpio_num_t)AUDIO::I2S_BCLK,
                .ws = (gpio_num_t)AUDIO::I2S_LRC,
                .dout = (gpio_num_t)AUDIO::I2S_DIN,
                .din = I2S_GPIO_UNUSED,
                .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false }
            }
        };

        err = i2s_channel_init_std_mode(tx_handle, &std_cfg);
        if (err != ESP_OK) {
            Serial.printf("[AudioOutput] I2S init failed: %d\n", err);
            return;
        }

        err = i2s_channel_enable(tx_handle);
        if (err != ESP_OK) {
            Serial.printf("[AudioOutput] I2S enable failed: %d\n", err);
            return;
        }

        timer = timerBegin(80000000 / SAMPLE_RATE);
        timerAttachInterrupt(timer, &onTimer);

        xTaskCreatePinnedToCore(audioTask, "audio", 4096, NULL, 5, &audioTaskHandle, 1);

        Serial.println("[AudioOutput] Initialized (22,050 Hz)");
    }

    static void start() {
        if (!timer || !engine) return;
        active = true;
        timerAlarm(timer, 0, true, 0);
        Serial.println("[AudioOutput] Started");
    }

    static void stop() {
        active = false;
        if (timer) timerAlarm(timer, 0, false, 0);
        size_t bytes_written;
        int16_t silence[BUFFER_SIZE] = {};
        i2s_channel_write(tx_handle, silence, sizeof(silence), &bytes_written, 100);
        Serial.println("[AudioOutput] Stopped");
    }

    static void onTimer() {
        if (!active || !engine) return;

        int8_t sample = engine->getNextSample();
        int16_t out_sample = (int16_t)sample * 256;

        buffer[bufferPos] = out_sample;
        bufferPos++;

        if (bufferPos >= BUFFER_SIZE) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            vTaskNotifyGiveFromISR(audioTaskHandle, &xHigherPriorityTaskWoken);
            bufferPos = 0;
        }
    }

    static TaskHandle_t audioTaskHandle;

    static void audioTask(void* param) {
        while (true) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            if (active && tx_handle) {
                size_t bytes_written;
                i2s_channel_write(tx_handle, buffer, sizeof(buffer), &bytes_written, portMAX_DELAY);
            }
        }
    }
};

RcEngineSound* AudioOutput::engine = nullptr;
volatile bool AudioOutput::active = false;
int16_t AudioOutput::buffer[BUFFER_SIZE] = {};
volatile uint32_t AudioOutput::bufferPos = 0;
hw_timer_t* AudioOutput::timer = nullptr;
i2s_chan_handle_t AudioOutput::tx_handle = nullptr;
TaskHandle_t AudioOutput::audioTaskHandle = nullptr;
