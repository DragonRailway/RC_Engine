/**
 * @file EasyLEDGroup.cpp
 * @brief Implementation of EasyLEDGroup class.
 */
#include "EasyLEDGroup.h"

EasyLEDGroup::EasyLEDGroup() = default;

EasyLEDGroup::EasyLEDGroup(std::initializer_list<EasyLED*> members) {
    for (auto* m : members) {
        if (m && m_members.size() < EASYLED_GROUP_MAX_MEMBERS) {
            m_members.push_back(m);
        }
    }
}

EasyLEDGroup::EasyLEDGroup(const std::vector<EasyLED*>& members) {
    for (auto* m : members) {
        if (m && m_members.size() < EASYLED_GROUP_MAX_MEMBERS) {
            m_members.push_back(m);
        }
    }
}

void EasyLEDGroup::addMember(EasyLED* member) {
    if (member && m_members.size() < EASYLED_GROUP_MAX_MEMBERS) {
        m_members.push_back(member);
    }
}

void EasyLEDGroup::clearMembers() {
    stop();
    m_members.clear();
}

bool EasyLEDGroup::startPattern(const EasyLEDStep* steps, uint8_t stepCount, bool repeat) {
    if (!steps || stepCount == 0 || m_members.empty()) {
        stop();
        return false;
    }

    m_steps.assign(steps, steps + stepCount);
    m_repeat = repeat;
    m_currentStepIndex = 0;
    m_stepStartTimeMs = millis();
    m_running = true;

    applyCurrentStep();
    return true;
}

bool EasyLEDGroup::startPattern(const std::vector<EasyLEDStep>& steps, bool repeat) {
    if (steps.empty() || m_members.empty()) {
        stop();
        return false;
    }
    return startPattern(steps.data(), static_cast<uint8_t>(steps.size()), repeat);
}

bool EasyLEDGroup::alternate(uint16_t intervalMs) {
    if (m_members.empty()) return false;

    size_t count = m_members.size();
    std::vector<EasyLEDStep> steps;

    for (size_t i = 0; i < count; ++i) {
        EasyLEDStep step{};
        step.durationMs = intervalMs;
        step.duty[i] = 100;
        steps.push_back(step);
    }

    return startPattern(steps, true);
}

bool EasyLEDGroup::syncFlash(uint16_t onMs, uint16_t offMs) {
    if (m_members.empty()) return false;

    std::vector<EasyLEDStep> steps(2);
    steps[0].durationMs = onMs;
    steps[1].durationMs = offMs;

    for (size_t i = 0; i < m_members.size(); ++i) {
        steps[0].duty[i] = 100;
        steps[1].duty[i] = 0;
    }

    return startPattern(steps, true);
}

bool EasyLEDGroup::chase(uint16_t intervalMs) {
    if (m_members.empty()) return false;

    size_t count = m_members.size();
    std::vector<EasyLEDStep> steps;

    for (size_t i = 0; i < count; ++i) {
        EasyLEDStep step{};
        step.durationMs = intervalMs;
        step.duty[i] = 100;
        steps.push_back(step);
    }

    return startPattern(steps, true);
}

bool EasyLEDGroup::doubleStrobe(uint16_t intervalMs, uint16_t gapMs) {
    if (m_members.empty()) return false;

    std::vector<EasyLEDStep> steps;

    if (m_members.size() == 2) {
        // Alternating double strobe for pairs
        EasyLEDStep s0{}, s1{}, s2{}, s3{}, s4{};
        s0.duty[0] = 100; s0.duty[1] = 0;   s0.durationMs = intervalMs;
        s1.duty[0] = 0;   s1.duty[1] = 100; s1.durationMs = intervalMs;
        s2.duty[0] = 100; s2.duty[1] = 0;   s2.durationMs = intervalMs;
        s3.duty[0] = 0;   s3.duty[1] = 100; s3.durationMs = intervalMs;
        s4.duty[0] = 0;   s4.duty[1] = 0;   s4.durationMs = gapMs;
        steps = {s0, s1, s2, s3, s4};
    } else {
        // Sync double strobe
        EasyLEDStep s0{}, s1{}, s2{}, s3{};
        s0.durationMs = intervalMs;
        s1.durationMs = intervalMs;
        s2.durationMs = intervalMs;
        s3.durationMs = gapMs;
        for (size_t i = 0; i < m_members.size(); ++i) {
            s0.duty[i] = 100;
            s1.duty[i] = 0;
            s2.duty[i] = 100;
            s3.duty[i] = 0;
        }
        steps = {s0, s1, s2, s3};
    }

    return startPattern(steps, true);
}

void EasyLEDGroup::stop() {
    m_running = false;
    m_steps.clear();
    m_currentStepIndex = 0;

    for (auto* m : m_members) {
        if (m) {
            m->write(0.0f);
        }
    }
}

void EasyLEDGroup::update() {
    if (!m_running || m_steps.empty() || m_members.empty()) return;

    uint32_t now = millis();
    uint32_t elapsed = now - m_stepStartTimeMs;

    if (elapsed >= m_steps[m_currentStepIndex].durationMs) {
        m_currentStepIndex++;
        if (m_currentStepIndex >= m_steps.size()) {
            if (m_repeat) {
                m_currentStepIndex = 0;
            } else {
                stop();
                return;
            }
        }
        m_stepStartTimeMs = now;
        applyCurrentStep();
    }
}

void EasyLEDGroup::applyCurrentStep() {
    if (m_currentStepIndex >= m_steps.size()) return;

    const auto& step = m_steps[m_currentStepIndex];
    for (size_t i = 0; i < m_members.size(); ++i) {
        if (m_members[i]) {
            uint8_t dutyPct = step.duty[i];
            m_members[i]->write(static_cast<float>(dutyPct));
        }
    }
}
