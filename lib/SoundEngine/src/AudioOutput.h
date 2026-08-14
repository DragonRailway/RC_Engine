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

    // DAC offset fade (pop prevention) - applied in audioTask, not ISR
    static volatile uint8_t currentOffset;
    static volatile bool offsetRamping;

    static void begin(RcEngineSound* eng) {
        engine = eng;
        active = false;
        bufferPos = 0;
        currentOffset = 0;
        offsetRamping = false;

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

        timer = timerBegin(SAMPLE_RATE);   // new 1-arg API: frequency in Hz, not a divider
        timerAttachInterrupt(timer, &onTimer);

        // 8KB stack: getNextSample() needs ~1KB for its VoiceState snapshot and
        // i2s_channel_write() is also deep; 4KB was tight for both in one task.
        xTaskCreatePinnedToCore(audioTask, "audio", 8192, NULL, 5, &audioTaskHandle, 1);

        Serial.println("[AudioOutput] Initialized (22,050 Hz)");
    }

    static void start() {
        if (!timer || !engine) return;
        active = true;
        currentOffset = 0; // Start with offset at 0, audioTask will ramp it
        offsetRamping = true;
        timerAlarm(timer, 1, true, 0);    // alarm every tick (count >= 1 is required)
        Serial.println("[AudioOutput] Started");
    }

    static void stop() {
        active = false;
        offsetRamping = false;
        currentOffset = 0;
        if (timer) timerAlarm(timer, 1, false, 0);
        size_t bytes_written;
        int16_t silence[BUFFER_SIZE] = {};
        i2s_channel_write(tx_handle, silence, sizeof(silence), &bytes_written, 100);
        Serial.println("[AudioOutput] Stopped");
    }

    // Minimal ISR: pacing only. Sample generation and all FPU math live in the
    // audio task — the Xtensa FPU coprocessor is NOT available in ISR context on
    // this IDF build (a floating-point op in onTimer panics with a Coprocessor
    // exception), and getNextSample() also needs ~1.3KB of stack for its voice
    // snapshot which is unsafe on the small ISR stack.
    static void onTimer() {
        if (!active) return;

        bufferPos = bufferPos + 1;
        if (bufferPos >= BUFFER_SIZE) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            vTaskNotifyGiveFromISR(audioTaskHandle, &xHigherPriorityTaskWoken);
            bufferPos = 0;
        }
    }

    enum SelftestMode {
        SELFTEST_OFF = 0,
        SELFTEST_SINE = 1,
        SELFTEST_IMPULSE = 2,
        SELFTEST_SWEEP = 3,
        SELFTEST_SILENCE = 4
    };

    static volatile SelftestMode selftestMode;
    static volatile uint32_t selftestSampleIndex;

    static void setSelftestMode(SelftestMode mode) {
        selftestMode = mode;
        selftestSampleIndex = 0;
        Serial.printf("[AudioOutput] Selftest mode set to %d\n", mode);
    }

    static TaskHandle_t audioTaskHandle;

    // Audio output task: generate samples (FPU-safe task context), apply offset
    // fade, write to I2S. Paced by the 22.05kHz timer ISR notification.
    static void audioTask(void* param) {
        while (true) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            if (!active || !tx_handle || !engine) continue;

            int64_t t_start = esp_timer_get_time();

            // Generate one buffer of samples in task context
            if (selftestMode != SELFTEST_OFF) {
                for (int i = 0; i < BUFFER_SIZE; i++) {
                    uint32_t idx = selftestSampleIndex;
                    selftestSampleIndex = idx + 1;
                    float t = (float)idx / (float)SAMPLE_RATE;
                    int8_t sample = 0;
                    if (selftestMode == SELFTEST_SINE) {
                        sample = (int8_t)(sinf(2.0f * M_PI * 440.0f * t) * 100.0f);
                    } else if (selftestMode == SELFTEST_IMPULSE) {
                        sample = (idx % 2205 == 0) ? 127 : 0;
                    } else if (selftestMode == SELFTEST_SWEEP) {
                        float freq = 100.0f + fmodf(t * 1000.0f, 3900.0f);
                        sample = (int8_t)(sinf(2.0f * M_PI * freq * t) * 100.0f);
                    } else if (selftestMode == SELFTEST_SILENCE) {
                        sample = 0;
                    }
                    buffer[i] = (int16_t)sample * 256;
                }
            } else {
                for (int i = 0; i < BUFFER_SIZE; i++) {
                    int8_t sample = engine->getNextSample();
                    buffer[i] = (int16_t)sample * 256;
                }
            }

            int64_t t_gen = esp_timer_get_time();

            // Apply volume ramp: scale 0.0→1.0 over ~12ms (276 samples)
            // At 22,050Hz with 64-sample buffers, increment ~30/128 per buffer
            if (offsetRamping && currentOffset < 128) {
                currentOffset += 30;
                if (currentOffset >= 128) {
                    currentOffset = 128;
                    offsetRamping = false;
                }
            }

            // Scale buffer by ramp factor (0.0→1.0) instead of adding offset
            // This prevents pops by fading from silence to full volume
            float scale = currentOffset / 128.0f;
            for (int i = 0; i < BUFFER_SIZE; i++) {
                buffer[i] = (int16_t)(buffer[i] * scale);
            }

            size_t bytes_written;
            i2s_channel_write(tx_handle, buffer, sizeof(buffer), &bytes_written, portMAX_DELAY);
            int64_t t_i2s = esp_timer_get_time();

#ifdef AUDIO_DEBUG
            static uint32_t decimate = 0;
            decimate++;
            if (decimate >= 100) {
                decimate = 0;
                int16_t peak = 0;
                int64_t sum_sq = 0;
                int clips = 0;
                for (int i = 0; i < BUFFER_SIZE; i++) {
                    int16_t val = buffer[i];
                    if (abs(val) > peak) peak = abs(val);
                    sum_sq += (int64_t)val * val;
                    if (val <= -32768 || val >= 32767) clips++;
                }
                uint16_t rms = (uint16_t)sqrtf((float)sum_sq / BUFFER_SIZE);
                uint32_t gen_us = (uint32_t)(t_gen - t_start);
                uint32_t i2s_us = (uint32_t)(t_i2s - t_gen);
                Serial.printf("[AUDIO_STATS] {\"peak\":%d,\"rms\":%d,\"clips\":%d,\"nan\":0,\"task_us\":%u,\"i2s_us\":%u}\n",
                              peak, rms, clips, gen_us, i2s_us);
            }
#endif
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
volatile uint8_t AudioOutput::currentOffset = 0;
volatile bool AudioOutput::offsetRamping = false;
volatile AudioOutput::SelftestMode AudioOutput::selftestMode = AudioOutput::SELFTEST_OFF;
volatile uint32_t AudioOutput::selftestSampleIndex = 0;
