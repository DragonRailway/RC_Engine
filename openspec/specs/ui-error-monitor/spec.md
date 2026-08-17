# UI Error Monitor Specification

## Purpose
Defines requirements for dynamic on-screen error and warning monitoring widgets within the RadioKit control app, providing real-time configuration and runtime error visibility while auto-hiding on clean operation.

## Requirements

### Requirement: Default hidden state on initialization
The system SHALL initialize all UI serial monitor widgets (`serial_monitor_1` on page 0 and `serial_monitor_2` on page 1) in a hidden state upon startup when no errors are present.

#### Scenario: Clean boot with valid configuration
- **WHEN** the firmware boots and loads valid hardware and vehicle configuration files with zero warnings or errors
- **THEN** both `serial_monitor_1` and `serial_monitor_2` remain hidden (`hidden = true`).

### Requirement: Boot-time warning capture and flush
The system SHALL buffer any configuration warnings produced during boot before RadioKit is initialized, and flush them to the UI serial monitor widgets upon RadioKit startup.

#### Scenario: Warnings detected during initial boot config load
- **WHEN** `ConfigParser` encounters semantic warnings or unrecognized tokens during initial boot loading
- **THEN** warnings are queued and automatically written to `serial_monitor_1` and `serial_monitor_2` after `initRadioKit()` runs, setting both monitors to unhidden (`setHidden(false)`).

### Requirement: Dynamic unhiding on runtime or hot-reload error
The system SHALL unhide the UI serial monitor widgets and display error/warning text whenever a configuration error, validation warning, or runtime fault occurs.

#### Scenario: Configuration error during hot-reload
- **WHEN** a configuration file with syntax or validation errors is uploaded over LittleFS
- **THEN** the system unhides `serial_monitor_1` and `serial_monitor_2` and streams the error description to the UI widgets only.

### Requirement: Auto-hide on clean recovery
The system SHALL clear the UI serial monitor text and return the widgets to hidden state when a clean configuration is hot-reloaded without errors or warnings.

#### Scenario: Fixed configuration reloaded successfully
- **WHEN** a valid configuration file is uploaded and reloaded with zero warnings/errors
- **THEN** the system sets `serial_monitor_1.setHidden(true)` and `serial_monitor_2.setHidden(true)`.
