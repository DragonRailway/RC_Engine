#pragma once

#include <stdlib.h>
#include <string.h>

#ifdef ESP32
#include <esp32-hal-psram.h>
#define SOUND_PSMALLOC(s) ps_malloc(s)
#else
#define SOUND_PSMALLOC(s) malloc(s)
#endif

/**
 * @brief Shared sound type definitions for the SoundEngine.
 * 
 * This header defines the unified SoundID enum used by both
 * RcEngineSound and VehicleProfile, ensuring they stay in sync.
 *
 * NOTE: the canonical VehicleType enum lives in RcEngineSound.h
 * (TRUCK / LOCOMOTIVE / EXCAVATOR / UNKNOWN). The duplicate that
 * used to live here was removed — keep vehicle type definitions
 * in one place.
 */

enum SoundID {
    IDLE, REV, START, KNOCK, TURBO, WASTEGATE, HORN,
    JAKE_BRAKE, FAN, SIREN, BRAKE, PARKING_BRAKE,
    SHIFTING, REVERSING, INDICATOR, COUPLING, SUPERCHARGER,
    UNCOUPLING, SOUND1, TIRE_SQUEAL, HYDRAULIC_PUMP,
    HYDRAULIC_FLOW, TRACK_RATTLE, BUCKET_RATTLE,
    BELL, DOOR, SCANNER, MUSIC, WHISTLE, GUN, OUT_OF_FUEL, OTHERS,
    SOUND_COUNT
};

/**
 * @brief Single sound slot holding sample data and metadata.
 */
struct SoundSlot {
    int8_t* samples = nullptr;
    uint32_t sampleCount = 0;
    uint16_t sampleRate = 22050;
};

/**
 * @brief Complete sound data for a vehicle profile.
 * 
 * Contains one SoundSlot per SoundID, plus a flag indicating
 * whether the memory was dynamically allocated (for cleanup).
 */
struct SoundData {
    SoundSlot slots[SOUND_COUNT];
    bool isDynamic = false;

    void clear() {
        if (isDynamic) {
            for (int i = 0; i < SOUND_COUNT; i++) {
                if (slots[i].samples) {
                    free(slots[i].samples);
                    slots[i].samples = nullptr;
                }
                slots[i].sampleCount = 0;
            }
        }
        isDynamic = false;
    }

    SoundData& operator=(const SoundData& other) {
        if (this == &other) return *this;
        clear();
        for (int i = 0; i < SOUND_COUNT; i++) {
            slots[i].sampleCount = other.slots[i].sampleCount;
            slots[i].sampleRate = other.slots[i].sampleRate;
            slots[i].samples = nullptr;
            if (other.slots[i].samples && other.slots[i].sampleCount > 0) {
                slots[i].samples = (int8_t*)SOUND_PSMALLOC(other.slots[i].sampleCount);
                if (slots[i].samples) {
                    memcpy(slots[i].samples, other.slots[i].samples, other.slots[i].sampleCount);
                } else {
                    slots[i].sampleCount = 0;
                }
            }
        }
        isDynamic = true;
        return *this;
    }

    SoundData& operator=(SoundData&& other) noexcept {
        if (this == &other) return *this;
        clear();
        for (int i = 0; i < SOUND_COUNT; i++) {
            slots[i] = other.slots[i];
            other.slots[i] = SoundSlot();
        }
        isDynamic = other.isDynamic;
        other.isDynamic = false;
        return *this;
    }
};
