## ADDED Requirements

### Requirement: Indicator click sound
The system SHALL play the indicator click sound while any turn indicator is active — whether from automatic steering-based turn signals or manual hazard flashing.

#### Scenario: Auto turn signal sounds
- **WHEN** a steering-based auto turn signal is active
- **THEN** the indicator click sound plays

#### Scenario: Hazard mode sounds
- **WHEN** hazard mode is active
- **THEN** the indicator click sound plays

#### Scenario: No indicator, no sound
- **WHEN** no turn indicator is active
- **THEN** the indicator click sound is silent
