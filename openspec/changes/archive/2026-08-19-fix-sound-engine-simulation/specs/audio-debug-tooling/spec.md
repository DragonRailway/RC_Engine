# Delta Spec: Audio Debug Tooling

## MODIFIED Requirements

### Requirement: Host-side DSP testing
The repository SHALL provide a host test harness (`scripts/host_dsp_test.py` plus a stub-Arduino native build of `RcEngineSound`) that compiles and runs the real engine code on the host, drives deterministic scripts, and asserts on the generated sample stream: pitch via zero-crossing rate matches the commanded RPM/pitch factor, loop regions are respected, one-shot voices deactivate exactly once, knock cadence matches the configured pattern continuously across idle sample buffer circular wraparounds, idle/rev cross-fading decays smoothly toward 0% idle at high RPM, and no NaN or int8 overflow occurs.

#### Scenario: Knock cadence across circular idle wraparound
- **WHEN** the engine is running at idle or low RPM across multiple idle loop wraparound cycles
- **THEN** the knock pulse triggers at regular intervals and does not silence after the first idle loop iteration

#### Scenario: Idle/Rev cross-fade proportion decay
- **WHEN** RPM transitions from `idleEndPoint` to `revSwitchPoint`
- **THEN** `idleProportion` decreases monotonically from 100% down to 0% and rev volume increases
