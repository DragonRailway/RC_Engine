# Tasks: ConfigParser Defaults Refactor

## Section 1: Refactor ConfigParser Fallbacks

- [x] 1.1 Update `common/ConfigParser.h` drive motor parsing (`duty.min`, `duty.max`, `frequency`, `direction`) to use `config.<field>` fallbacks
- [x] 1.2 Update `common/ConfigParser.h` steering servo parsing (`frequency`, `endpoints`) to use `config.<field>` fallbacks
- [x] 1.3 Update `common/ConfigParser.h` sound parsing (`volume`) to use `config.<field>` fallbacks
- [x] 1.4 Update `common/ConfigParser.h` animation parsing (`easing_speed_deg_s`, `easing_k_in`, `easing_k_out`, `fade_duration_ms`) to use `config.<field>` fallbacks
- [x] 1.5 Update `common/ConfigParser.h` battery parsing (`cell_count`, `warning_voltage`, `cutoff_voltage`, `full_voltage`, `vScale`, `vOffset`) to use `config.<field>` fallbacks
- [x] 1.6 Update `common/ConfigParser.h` power parsing (`boot_latch_s`, `button_hold_s`, `disconnect_timeout_s`, `warning_window_s`, `cutoff_delay_s`) to use `config.<field>` fallbacks

## Section 2: Verification & Test Suite

- [x] 2.1 Run `python3 scripts/validate_configs.py` to confirm hardware configs validate cleanly
- [x] 2.2 Run `python3 scripts/host_vc_test.py` to verify host VC physics test harness passes all 13 test suites
- [x] 2.3 Build firmware environments (`pio run`, `pio run -e MIKRO_V2`) to ensure 0 compilation errors
