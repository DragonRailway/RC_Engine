#pragma once

#include <Arduino.h>
#include "RADIOKIT.h"
#include "UiLogger.h"
#include "Config.h"
#include "HardwareInit.h"
#include "boards.h"  // board selected at build time via platformio.ini env define
#include <RcEngineSound.h>
#include <VehicleProfile.h>

/**
 * @brief Vehicle control loop: maps RadioKit widget inputs to motor, servo,
 *        lights and the sound engine, and publishes telemetry.
 *
 * Includes:
 *   - Auto LiPo cell detection & low-voltage safety cutoff
 *   - Steering auto turn-signal cancel & dynamic decel brake lights
 *   - Headlight 3-state stepping (Off, Low Beam 40%, High Beam 100%)
 *   - Synchronized hazard light flashing
 *   - Engine Start/Stop power state & physics-based Jake Brake / Wastegate FX
 *   - Work Machine Hydraulics, Hydraulic Pump Load Governor (+20% RPM bump),
 *     speed-dependent Track Rattle, Bucket Rattle, Dump Bed, & Physical Aux Servos.
 */
class VehicleController {
public:
    // Modular control variables for Work Machine Hydraulics & Actuators
    static int16_t aux_hydraulic1;
    static bool    bucket_rattle_trigger;
    static bool    dump_bed_toggle;

    static const char* engineStateStr(RcEngineSound::EngineState s);
    static void applyConfiguredLightMask(const HardwareConfig::Lights& L, bool auxHwConfigured);
    static void init(HardwareConfig* hw, RcEngineSound* engine, VehicleProfile* profile);

    static bool isBatteryWarning() { return s_batteryWarning; }
    static bool isBatteryCutoff() { return s_batteryCutoff; }
    static bool isDisconnectWarning() { return s_inWarningPhase; }
    static bool isChargingState() { return HardwareInit::isCharging(); }
    static bool isReverserInterlocked() { return s_reverserInterlocked; }
    static bool getActiveDirection() { return s_activeDirection; }

    static void update();

private:
    static HardwareConfig* s_hw;
    static RcEngineSound*  s_engine;
    static VehicleProfile* s_profile;

    static RcEngineSound::EngineState s_engineStatePrev;
    static bool     s_brakePrev;
    static bool     s_hornPrev;
    static bool     s_bellPrev;
    static bool     s_reversePrev;
    static uint32_t s_lastTelemetry;
    static char     s_battBuf[8];

    static char     s_speedBuf[8];

    // Battery safety
    static uint8_t  s_cellCount;
    static float    s_warningVoltage;
    static float    s_cutoffVoltage;
    static float    s_filteredBatV;
    static uint32_t s_lowVoltageStart;
    static bool     s_batteryWarning;
    static bool     s_batteryCutoff;
    static uint32_t s_disconnectStart;
    static bool     s_inWarningPhase;

    // Lighting automation
    static int16_t  s_prevThrottlePct;
    static uint32_t s_decelBrakeTime;
    static uint8_t  s_headlightMode;
    static uint8_t  s_lastHeadBright;
    static uint8_t  s_lastFullBright;
    static bool     s_fogLampPrev;
    static bool     s_autoTurnLeft;
    static bool     s_autoTurnRight;
    static bool     s_leftTurnArmed;
    static bool     s_rightTurnArmed;
    static int16_t  s_leftTurnBaseline;
    static int16_t  s_leftTurnPeak;
    static int16_t  s_rightTurnBaseline;
    static int16_t  s_rightTurnPeak;
    static bool     s_leftIndPrev;
    static bool     s_rightIndPrev;
    static bool     s_leftIndActive;
    static bool     s_rightIndActive;
    static bool     s_leftIndSuppressed;
    static bool     s_rightIndSuppressed;
    static bool     s_engineStartTogglePrev;
    static bool     s_cutoffLightResetDone;
    static bool     s_jakeBrakePrev;
    static HardwareInit::TurnMode s_turnModePrev;
    static uint32_t s_lastIndicatorClick;
    static uint8_t  s_gearPrev;
    static bool     s_parkingBrakePrev;
    static bool     s_wasConnected;
    static bool     s_reconnectThrottleInterlock;
    static bool     s_disconnectEngineStopDone;

    // Drivetrain Virtual Mass Inertia Simulation
    static float    s_currentMotorSpeed;
    static uint32_t s_lastInertiaTime;
    static bool     s_prevMotorMoving;

    // Dynamic Steering Auto-Centering
    static float    s_currentSteerAngle;
    static int8_t   s_lastSteerInputVal;
    static uint32_t s_lastSteerTouchMs;
    static uint32_t s_lastSteerPhysicsMs;

    // Locomotive Dynamics & Directional Lighting
    static bool     s_locoInitialized;
    static bool     s_activeDirection;
    static bool     s_reverserInterlocked;
    static bool     s_dirSwitchPrev;
    static float    s_slewHeadDuty;
    static float    s_slewTailDuty;
    static float    s_slewCabDuty;
    static float    s_slewStepDuty;
    static uint32_t s_lastLightSlewMs;
    static bool     s_ditchRunning;

    static int16_t computeRampedMotorSpeed(int16_t targetSpeed, bool parkingBrake, uint8_t gear, int16_t brakePct, RcEngineSound::EngineState eState);
    static int16_t updateDynamicSteering(int16_t rawSteer, int16_t motorSpeed, bool reverse, bool isLoco);
    static void applyLightsWithAutomation(uint8_t bits, bool turnL, bool turnR, bool decelBrake, bool manualBrake, uint8_t headlightMode, bool autoReverseLight, bool fogLamp, bool isLoco);
    static void updateTelemetry(int16_t motorSpeed, int16_t steerVal, int16_t throttlePct, uint8_t gear, bool brakePressed, bool turnL, bool turnR, uint8_t bits, float batV);
};
