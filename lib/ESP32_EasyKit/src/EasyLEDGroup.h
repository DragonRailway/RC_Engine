/**
 * @file EasyLEDGroup.h
 * @brief Coordinated multi-LED step-sequencer group for ESP32_EasyKit.
 *
 * Controls N EasyLED instances using timed step sequences (duty vectors).
 */
#pragma once

#include <cstdint>
#include <vector>
#include <initializer_list>
#include <Arduino.h>
#include "EasyLED.h"

static constexpr uint8_t EASYLED_GROUP_MAX_MEMBERS = 8;

struct EasyLEDStep {
    uint8_t duty[EASYLED_GROUP_MAX_MEMBERS]; ///< Duty percentage (0-100) per member
    uint16_t durationMs;                      ///< Step duration in milliseconds
};

class EasyLEDGroup {
public:
    EasyLEDGroup();
    EasyLEDGroup(std::initializer_list<EasyLED*> members);
    explicit EasyLEDGroup(const std::vector<EasyLED*>& members);

    void addMember(EasyLED* member);
    void clearMembers();
    size_t getMemberCount() const { return m_members.size(); }

    /// Start custom pattern step table
    bool startPattern(const EasyLEDStep* steps, uint8_t stepCount, bool repeat = true);
    bool startPattern(const std::vector<EasyLEDStep>& steps, bool repeat = true);

    /// Built-in pattern factories
    bool alternate(uint16_t intervalMs);
    bool syncFlash(uint16_t onMs, uint16_t offMs);
    bool chase(uint16_t intervalMs);
    bool doubleStrobe(uint16_t intervalMs, uint16_t gapMs);

    /// Stop pattern, set all member duties to 0
    void stop();

    /// Check if a pattern is actively running
    bool isRunning() const { return m_running; }

    /// Advance timeline & update member duties
    void update();

private:
    std::vector<EasyLED*> m_members;
    std::vector<EasyLEDStep> m_steps;
    bool m_running = false;
    bool m_repeat = true;
    uint8_t m_currentStepIndex = 0;
    uint32_t m_stepStartTimeMs = 0;

    void applyCurrentStep();
};
