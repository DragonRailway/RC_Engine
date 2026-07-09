#pragma once

/**
 * @brief Shared sound type definitions for the SoundEngine.
 * 
 * This header defines the unified SoundID enum used by both
 * RcEngineSound and VehicleProfile, ensuring they stay in sync.
 */

enum SoundID {
    IDLE, REV, START, KNOCK, TURBO, WASTEGATE, HORN,
    JAKE_BRAKE, FAN, SIREN, BRAKE, PARKING_BRAKE,
    SHIFTING, REVERSING, INDICATOR, COUPLING, SUPERCHARGER,
    UNCOUPLING, SOUND1, TIRE_SQUEAL, HYDRAULIC_PUMP,
    HYDRAULIC_FLOW, TRACK_RATTLE, BUCKET_RATTLE,
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
};
