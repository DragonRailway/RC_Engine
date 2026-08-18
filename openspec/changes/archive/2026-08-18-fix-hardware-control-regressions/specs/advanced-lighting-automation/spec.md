## MODIFIED Requirements

### Requirement: Manual turn signal toggle buttons
The system SHALL treat the manual `left_indicator` and `right_indicator` buttons as toggle controls with edge-latched suppression. Engaging a turn indicator SHALL capture the steering wheel angle at button press as the baseline. The active indicator SHALL automatically cancel (setting the firmware indicator state to false and turning off blinking) when:
1. The steering wheel is turned into the indicated direction by at least 20% relative to the baseline (`delta <= -20%` for Left, `delta >= +20%` for Right) and then returns towards the baseline / center by at least 15%.
2. The steering wheel is turned in the opposite direction beyond threshold (+15% relative to the baseline for Left indicator, -15% relative to the baseline for Right indicator).

When an indicator is auto-cancelled by firmware, the firmware SHALL mark that indicator as suppressed so that repeated incoming BLE frames with `state == true` do not re-trigger a new activation edge. The suppression SHALL be cleared only when the incoming widget state transitions to `false` (user taps button to OFF). Engaging an indicator SHALL automatically turn off and suppress the opposite indicator.

#### Scenario: Left indicator cancelled on steering return
- **WHEN** `left_indicator` is ON, the steering wheel turns left by >=20% from baseline, and then returns towards center/baseline
- **THEN** `left_indicator` is automatically turned OFF, blinking stops, and repeated incoming ON frames do not re-trigger the indicator until released

#### Scenario: Right indicator cancelled on steering return
- **WHEN** `right_indicator` is ON, the steering wheel turns right by >=20% from baseline, and then returns towards center/baseline
- **THEN** `right_indicator` is automatically turned OFF, blinking stops, and repeated incoming ON frames do not re-trigger the indicator until released

#### Scenario: Left indicator cancelled on opposite steer
- **WHEN** `left_indicator` is ON and the steering wheel turns right by >15% relative to baseline
- **THEN** `left_indicator` is immediately turned OFF, blinking stops, and re-triggering is suppressed

#### Scenario: Right indicator cancelled on opposite steer
- **WHEN** `right_indicator` is ON and the steering wheel turns left by >15% relative to baseline
- **THEN** `right_indicator` is immediately turned OFF, blinking stops, and re-triggering is suppressed

#### Scenario: Mutual exclusion between indicators
- **WHEN** `left_indicator` is ON and the user activates `right_indicator`
- **THEN** `left_indicator` is set to OFF and suppressed, and `right_indicator` is set to ON
