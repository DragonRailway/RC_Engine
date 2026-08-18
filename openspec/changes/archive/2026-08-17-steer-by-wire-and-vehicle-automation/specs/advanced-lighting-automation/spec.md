## MODIFIED Requirements

### Requirement: Manual turn signal toggle buttons
The system SHALL treat the manual `left_indicator` and `right_indicator` buttons as toggle controls. Engaging a turn indicator SHALL activate the corresponding blink light output. The active indicator SHALL automatically cancel (setting the widget state to false and updating the app UI) when:
1. The steering wheel is turned into the indicated direction beyond threshold (`|steer| > 20%`) and then returns towards center (`|steer| < 8%`).
2. The steering wheel is turned in the opposite direction beyond threshold (`steer > +15%` for Left indicator, `steer < -15%` for Right indicator).

Engaging an indicator SHALL automatically turn off the opposite indicator.

#### Scenario: Left indicator cancelled on steering return
- **WHEN** `left_indicator` is ON, the steering wheel turns left past -20%, and then returns to greater than -8%
- **THEN** `left_indicator` is automatically turned OFF, blinking stops, and UI toggle state resets to false

#### Scenario: Right indicator cancelled on steering return
- **WHEN** `right_indicator` is ON, the steering wheel turns right past +20%, and then returns to less than +8%
- **THEN** `right_indicator` is automatically turned OFF, blinking stops, and UI toggle state resets to false

#### Scenario: Left indicator cancelled on opposite steer
- **WHEN** `left_indicator` is ON and the steering wheel turns right past +15%
- **THEN** `left_indicator` is immediately turned OFF, blinking stops, and UI toggle state resets to false

#### Scenario: Right indicator cancelled on opposite steer
- **WHEN** `right_indicator` is ON and the steering wheel turns left past -15%
- **THEN** `right_indicator` is immediately turned OFF, blinking stops, and UI toggle state resets to false

#### Scenario: Mutual exclusion between indicators
- **WHEN** `left_indicator` is ON and the user activates `right_indicator`
- **THEN** `left_indicator` is set to OFF and `right_indicator` is set to ON
