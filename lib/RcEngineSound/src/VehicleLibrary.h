#ifndef VEHICLE_LIBRARY_H
#define VEHICLE_LIBRARY_H

#include "RcEngineSound.h"

/**
 * @brief Helper macros to adapt original vehicle headers to the RcEngineSound library.
 * 
 * The original vehicle headers (e.g. ScaniaV8.h) define many global variables.
 * These macros help bundle those into the library's SoundData and Config structs.
 */

#define DEFINE_SOUND_DATA(Name, Start, Idle, Rev, Horn) \
    SoundData Name = { \
        .samples = (const int8_t*)Idle##samples, \
        .sampleCount = Idle##sampleCount, \
        .sampleRate = Idle##sampleRate, \
        .startSamples = (const int8_t*)Start##startSamples, \
        .startSampleCount = Start##startSampleCount, \
        .revSamples = (const int8_t*)Rev##revSamples, \
        .revSampleCount = Rev##revSampleCount, \
        .hornSamples = (const int8_t*)Horn##hornSamples, \
        .hornSampleCount = Horn##hornSampleCount, \
        .hornSampleRate = Horn##sampleRate \
    };

// Since the original headers use the same variable names (samples, sampleCount), 
// we recommend including only ONE vehicle per compilation unit or using namespaces.

#endif // VEHICLE_LIBRARY_H
