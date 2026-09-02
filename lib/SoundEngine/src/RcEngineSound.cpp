#include "RcEngineSound.h"

void RcEngineSound::begin(const SoundData& soundData, const Config& config) {
    cfg = config;
    setConfig(config);
    m_synth.begin(soundData);
}

void RcEngineSound::begin(const SoundData& soundData) {
    begin(soundData, cfg);
}

void RcEngineSound::setConfig(const Config& config) {
    cfg = config;

    EngineSim::Config simCfg;
    strncpy(simCfg.name, cfg.name, sizeof(simCfg.name) - 1);
    strncpy(simCfg.description, cfg.description, sizeof(simCfg.description) - 1);
    strncpy(simCfg.soundSet, cfg.soundSet, sizeof(simCfg.soundSet) - 1);
    simCfg.type = static_cast<EngineSim::VehicleType>(cfg.type);

    simCfg.engine.hasEngine = cfg.engine.hasEngine;
    simCfg.engine.acc = cfg.engine.acc;
    simCfg.engine.dec = cfg.engine.dec;
    simCfg.engine.brakeDec = cfg.engine.brakeDec;
    simCfg.engine.inertia = cfg.engine.inertia;
    simCfg.engine.escRampTime = cfg.engine.escRampTime;
    simCfg.engine.maxRpm = cfg.engine.maxRpm;
    simCfg.engine.minRpm = cfg.engine.minRpm;
    simCfg.engine.maxPitchFactor = cfg.engine.maxPitchFactor;
    simCfg.engine.revSwitchPoint = cfg.engine.revSwitchPoint;
    simCfg.engine.idleEndPoint = cfg.engine.idleEndPoint;
    simCfg.engine.jakeBrakeMinRpm = cfg.engine.jakeBrakeMinRpm;
    simCfg.engine.jakeBrakeDecelRate = cfg.engine.jakeBrakeDecelRate;
    simCfg.engine.superchargerStartPoint = cfg.engine.superchargerStartPoint;
    simCfg.engine.stopDuration = cfg.engine.stopDuration;

    simCfg.transmission.type = static_cast<EngineSim::TransmissionType>(cfg.transmission.type);
    simCfg.transmission.numberOfGears = cfg.transmission.numberOfGears;
    for (int i = 0; i < 6; i++) {
        simCfg.transmission.gearRampTimes[i] = cfg.transmission.gearRampTimes[i];
    }
    simCfg.transmission.torqueConverterSlip = cfg.transmission.torqueConverterSlip;

    simCfg.features.hydraulicEnabled = cfg.features.hydraulicEnabled;
    simCfg.features.hydrostaticMode = cfg.features.hydrostaticMode;
    simCfg.features.trackRattleEnabled = cfg.features.trackRattleEnabled;
    simCfg.features.dumpBedEnabled = cfg.features.dumpBedEnabled;
    simCfg.features.tireSquealThreshold = cfg.features.tireSquealThreshold;
    simCfg.features.tireSquealMaxSpeed = cfg.features.tireSquealMaxSpeed;
    simCfg.features.trackRattleIntervalMin = cfg.features.trackRattleIntervalMin;
    simCfg.features.trackRattleIntervalMax = cfg.features.trackRattleIntervalMax;

    simCfg.sound.master = cfg.sound.master;
    simCfg.sound.crawlerModeThreshold = cfg.sound.crawlerModeThreshold;
    simCfg.sound.tireSqueal = cfg.sound.tireSqueal;
    simCfg.sound.hydraulicPump = cfg.sound.hydraulicPump;
    simCfg.sound.trackRattle = cfg.sound.trackRattle;
    simCfg.sound.jakeBrake = cfg.sound.jakeBrake;
    simCfg.sound.shifting = cfg.sound.shifting;

    m_sim.begin(simCfg);

    SoundSynth::Config synthCfg;
    memcpy(&synthCfg.sound, &cfg.sound, sizeof(SoundSynth::Config::Sound));

    synthCfg.engine.knockPattern = static_cast<SoundSynth::KnockPattern>(cfg.engine.knockPattern);
    synthCfg.engine.knockInterval = cfg.engine.knockInterval;
    synthCfg.engine.knockAdaptiveVolume = cfg.engine.knockAdaptiveVolume;
    synthCfg.engine.minKnockVolume = cfg.engine.minKnockVolume;
    synthCfg.engine.knockStartRpm = cfg.engine.knockStartRpm;
    synthCfg.engine.maxRpm = cfg.engine.maxRpm;
    synthCfg.engine.revSwitchPoint = cfg.engine.revSwitchPoint;
    synthCfg.engine.idleEndPoint = cfg.engine.idleEndPoint;
    synthCfg.engine.superchargerStartPoint = cfg.engine.superchargerStartPoint;

    synthCfg.loopPoints.hornBegin = cfg.loopPoints.hornBegin;
    synthCfg.loopPoints.hornEnd = cfg.loopPoints.hornEnd;
    synthCfg.loopPoints.sirenBegin = cfg.loopPoints.sirenBegin;
    synthCfg.loopPoints.sirenEnd = cfg.loopPoints.sirenEnd;
    synthCfg.loopPoints.reversingBegin = cfg.loopPoints.reversingBegin;
    synthCfg.loopPoints.reversingEnd = cfg.loopPoints.reversingEnd;
    synthCfg.loopPoints.sound1Begin = cfg.loopPoints.sound1Begin;
    synthCfg.loopPoints.sound1End = cfg.loopPoints.sound1End;

    m_synth.setConfig(synthCfg);
}

void RcEngineSound::update(int16_t throttle) {
    if (m_sim.getState() == EngineSim::STARTING && m_synth.isStartSoundComplete()) {
        m_sim.setRunning();
    }
    m_sim.update(throttle);
    m_synth.syncState(m_sim);
}

void RcEngineSound::update(const EngineSim::Input& input, uint32_t dtMs) {
    if (m_sim.getState() == EngineSim::STARTING && m_synth.isStartSoundComplete()) {
        m_sim.setRunning();
    }
    m_sim.update(input, dtMs);
    m_synth.syncState(m_sim);
}
