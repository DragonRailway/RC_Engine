#include "RcEngineSound.h"

RcEngineSound::RcEngineSound() :
    state(OFF),
    currentRpm(0),
    currentRpmFixed(0),
    curEngineSample(0),
    curRevSample(0),
    curTurboSample(0),
    curKnockSample(0),
    curWastegateSample(0),
    curStartSample(0),
    curHornSample(0),
    lastThrottle(0),
    hornActive(false),
    wastegateTriggered(false),
    wastegateTriggerMillis(0),
    engineStopRequested(false),
    lastUpdateTime(0),
    attenuatorMillis(0),
    attenuator(1)
{}

RcEngineSound::~RcEngineSound() {
    if (sounds.isDynamic) {
        if (sounds.samples) free(sounds.samples);
        if (sounds.startSamples) free(sounds.startSamples);
        if (sounds.revSamples) free(sounds.revSamples);
        if (sounds.turboSamples) free(sounds.turboSamples);
        if (sounds.knockSamples) free(sounds.knockSamples);
        if (sounds.wastegateSamples) free(sounds.wastegateSamples);
        if (sounds.hornSamples) free(sounds.hornSamples);
    }
}

void RcEngineSound::begin(const SoundData& soundData, const Config& config) {
    sounds = soundData;
    cfg = config;
}

void RcEngineSound::begin(const SoundData& soundData) {
    begin(soundData, Config());
}

void RcEngineSound::startEngine() {
    if (state == OFF) {
        state = STARTING;
        curStartSample = 0;
    }
}

void RcEngineSound::stopEngine() {
    if (state == RUNNING) {
        engineStopRequested = true;
    }
}

void RcEngineSound::triggerHorn(bool active) {
    hornActive = active;
}

// ─── Advanced Engine State Machine ───────────────────────────────────────────
void RcEngineSound::update(int16_t throttle) {
    uint32_t now = millis();
    if (lastUpdateTime == 0) lastUpdateTime = now;
    uint32_t dt = now - lastUpdateTime;
    lastUpdateTime = now;

    int32_t targetRpm = abs(throttle);
    if (targetRpm > cfg.maxRpm) targetRpm = cfg.maxRpm;

    if (state == RUNNING) {
        // ── Inertia-based RPM smoothing ──
        // Higher inertia = heavier vehicle feel
        // Uses exponential moving average: newRPM = oldRPM + (target - oldRPM) * factor
        int32_t inertiaFactor = max((int32_t)1, (int32_t)(101 - cfg.inertia)); // 1 (sluggish) to 100 (instant)
        int32_t diff = targetRpm - currentRpm;

        if (diff > 0) {
            // Accelerating: use acc + inertia
            int32_t step = max((int32_t)1, (int32_t)((diff * inertiaFactor) / 200 + cfg.acc));
            currentRpm = min(currentRpm + step, targetRpm);
        } else if (diff < 0) {
            // Decelerating: use dec + inertia
            int32_t step = max((int32_t)1, (int32_t)(((-diff) * inertiaFactor) / 200 + cfg.dec));
            currentRpm = max(currentRpm - step, targetRpm);
        }

        // ── Wastegate detection ──
        // Trigger wastegate if throttle drops rapidly (> 150 units in one update cycle)
        if (sounds.wastegateSamples && sounds.wastegateSampleCount > 0) {
            int16_t throttleDrop = lastThrottle - throttle;
            if (throttleDrop > 150 && currentRpmFixed > 200 && !wastegateTriggered) {
                wastegateTriggered = true;
                curWastegateSample = 0;
                wastegateTriggerMillis = now;
            }
        }

        // ── Engine stop request ──
        if (engineStopRequested && currentRpm < 30) {
            state = STOPPING;
            engineStopRequested = false;
            attenuator = 1;
            attenuatorMillis = now;
        }
    } else if (state == OFF) {
        currentRpm = 0;
    }

    currentRpmFixed = currentRpm;
    lastThrottle = throttle;
}

// ─── Sophisticated 5-Voice Mixer ─────────────────────────────────────────────
uint8_t RcEngineSound::getNextSample() {
    int32_t mixed = 0;
    int32_t engineSample = 0;
    int32_t turboSample = 0;
    int32_t knockSample = 0;
    int32_t wastegateSample = 0;
    int32_t hornSample = 0;

    switch (state) {
        case STARTING:
            if (curStartSample < sounds.startSampleCount) {
                engineSample = (int8_t)sounds.startSamples[curStartSample];
                engineSample = engineSample * cfg.startVolume / 100;
                curStartSample++;
            } else {
                state = RUNNING;
                curEngineSample = 0;
                curRevSample = 0;
                curTurboSample = 0;
                curKnockSample = 0;
            }
            break;

        case RUNNING: {
            int32_t idleS = 0;
            int32_t revS = 0;

            // ── Voice 1: Idle sound (looping) ──
            if (sounds.sampleCount > 0) {
                if (curEngineSample >= sounds.sampleCount) curEngineSample = 0;
                idleS = (int8_t)sounds.samples[curEngineSample];
                curEngineSample++;
            }

            // ── Voice 2: Rev sound (cross-fade with idle based on RPM) ──
            if (sounds.revSamples && sounds.revSampleCount > 0) {
                if (curRevSample >= sounds.revSampleCount) curRevSample = 0;
                revS = (int8_t)sounds.revSamples[curRevSample];
                curRevSample++;

                // Cross-fade logic: 100% idle at low RPM, 100% rev at high RPM
                int16_t idleProportion = 100;
                if (currentRpmFixed > cfg.revSwitchPoint) {
                    idleProportion = map(currentRpmFixed, cfg.idleEndPoint, cfg.revSwitchPoint, 0, 100);
                    idleProportion = constrain(idleProportion, 0, 100);
                }
                
                // Apply per-voice volume and cross-fade proportion
                idleS = (idleS * cfg.idleVolume / 100) * idleProportion / 100;
                revS = (revS * cfg.revVolume / 100) * (100 - idleProportion) / 100;
                engineSample = idleS + revS;
            } else {
                engineSample = idleS * cfg.idleVolume / 100;
            }

            // ── Voice 3: Turbo whistle (RPM-dependent volume, looping) ──
            if (sounds.turboSamples && sounds.turboSampleCount > 0 && cfg.turboVolume > 0) {
                if (curTurboSample >= sounds.turboSampleCount) curTurboSample = 0;
                int32_t ts = (int8_t)sounds.turboSamples[curTurboSample];
                curTurboSample++;

                // Turbo volume scales with RPM: silent at idle, loud at max
                int32_t turboScale = map(currentRpmFixed, 0, cfg.maxRpm, 0, 100);
                turboScale = constrain(turboScale, 0, 100);
                turboSample = ts * cfg.turboVolume / 100 * turboScale / 100;
            }

            // ── Voice 4: Diesel knock (load-dependent, fixed pitch, periodic) ──
            if (sounds.knockSamples && sounds.knockSampleCount > 0 && cfg.knockVolume > 0) {
                if (curKnockSample >= sounds.knockSampleCount) curKnockSample = 0;
                int32_t ks = (int8_t)sounds.knockSamples[curKnockSample];
                curKnockSample++;

                // Knock volume is throttle/load dependent: louder under acceleration
                int32_t knockScale = map(currentRpmFixed, 0, cfg.maxRpm, 20, 100);
                knockScale = constrain(knockScale, 0, 100);
                knockSample = ks * cfg.knockVolume / 100 * knockScale / 100;
            }
            break;
        }

        case STOPPING:
            // Fade out idle sound
            if (sounds.sampleCount > 0) {
                if (curEngineSample >= sounds.sampleCount) curEngineSample = 0;
                engineSample = (int8_t)sounds.samples[curEngineSample];
                engineSample = engineSample * cfg.idleVolume / 100 / attenuator;
                curEngineSample++;
            }

            if (millis() - attenuatorMillis > 80) { // Faster fade-out steps
                attenuatorMillis = millis();
                attenuator++;
            }

            if (attenuator >= 40) {
                state = OFF;
            }
            break;

        default:
            engineSample = 0;
            break;
    }

    // ── Wastegate one-shot (plays across any engine state while triggered) ──
    if (wastegateTriggered && sounds.wastegateSamples) {
        if (curWastegateSample < sounds.wastegateSampleCount) {
            wastegateSample = (int8_t)sounds.wastegateSamples[curWastegateSample];
            wastegateSample = wastegateSample * cfg.wastegateVolume / 100;
            curWastegateSample++;
        } else {
            wastegateTriggered = false; // One-shot complete
        }
    }

    // ── Horn (looping while active) ──
    if (hornActive && sounds.hornSamples && sounds.hornSampleCount > 0) {
        if (curHornSample >= sounds.hornSampleCount) curHornSample = 0;
        hornSample = (int8_t)sounds.hornSamples[curHornSample] * cfg.hornVolume / 100;
        curHornSample++;
    }

    // ── Final mix: sum all voices, apply master volume, add DC offset ──
    mixed = engineSample + turboSample + knockSample + wastegateSample + (hornSample / 2);
    mixed = (mixed * cfg.masterVolume / 100) + 128;

    return (uint8_t)constrain(mixed, 0, 255);
}
