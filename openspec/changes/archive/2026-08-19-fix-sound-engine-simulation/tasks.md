# Tasks: Fix Sound Engine Simulation

- [x] 1.1 Update `lib/SoundEngine/src/RcEngineSound.cpp` `update()` with circular wraparound sample distance calculation for diesel knock cadence
- [x] 1.2 Update `lib/SoundEngine/src/RcEngineSound.cpp` `update()` with bounded monotonic idle/rev cross-fade mapping (100% idle at or below `idleEndPoint`, ramping to 0% at `revSwitchPoint`)
- [x] 1.3 Update `lib/SoundEngine/src/RcEngineSound.cpp` `getNextSample()` to batch write-backs of voice position and active flags into a single critical section per sample
- [x] 2.1 Run host DSP tests (`python3 scripts/host_dsp_test.py` or test harnesses) to verify audio synthesis continuity
- [x] 2.2 Compile firmware (`pio run`) to verify clean build without warnings or regressions
