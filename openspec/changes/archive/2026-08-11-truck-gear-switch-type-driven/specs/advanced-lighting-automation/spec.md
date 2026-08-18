## ADDED Requirements

### Requirement: Manual turn signal toggle buttons
The system SHALL treat the manual `left_indicator` and `right_indicator` buttons as latched toggle controls (tap to latch on, tap again to release) merged with the steering-based auto turn signals: either source lights its side, and each source releases independently. A manual toggle that is latched SHALL NOT be cancelled by steering returning to center.

#### Scenario: Manual left indicator latched
- **WHEN** the left indicator button is tapped on
- **THEN** the left turn indicator stays lit until the button is tapped off

#### Scenario: Manual and auto merge
- **WHEN** the right indicator button is latched and the steering wheel is then turned right
- **THEN** the right turn indicator remains lit (both sources agree) and stays lit when steering returns to center until the button is released

#### Scenario: Auto signal while manual off
- **WHEN** no manual indicator is latched and steering exceeds the auto-turn threshold
- **THEN** the corresponding side flashes from the auto signal alone

## MODIFIED Requirements

### Requirement: Reversing light automation
The truck reversing light SHALL be illuminated automatically whenever the truck gear is in R (Reverse), in addition to the manual Item E bitmask override. On the locomotive the reversing light remains manual only.

#### Scenario: Gear R lights reversing lamp
- **WHEN** the truck gear is in R and the reversing light is configured
- **THEN** the reversing light is on

#### Scenario: Manual reversing override still works
- **WHEN** the truck light Item E is selected
- **THEN** the reversing light turns on regardless of gear position
