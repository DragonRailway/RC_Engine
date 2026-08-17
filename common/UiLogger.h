#pragma once

#include <Arduino.h>
#include <stdarg.h>
#include "RADIOKIT.h"

class UiLogger {
public:
    static void init() {
        _bootQueue[0] = '\0';
        _bootQueueLen = 0;
        _hasErrors = false;
        _rkStarted = false;
    }

    static void onRadioKitStarted() {
        _rkStarted = true;
        flushBootQueue();
    }

    static void log(const char* msg) {
        if (!msg || msg[0] == '\0') return;
        _hasErrors = true;

        if (!_rkStarted) {
            size_t len = strlen(msg);
            if (_bootQueueLen + len + 2 < sizeof(_bootQueue)) {
                memcpy(_bootQueue + _bootQueueLen, msg, len);
                _bootQueueLen += len;
                _bootQueue[_bootQueueLen++] = '\n';
                _bootQueue[_bootQueueLen] = '\0';
            }
            return;
        }

        serial_monitor_1.setHidden(false);
        serial_monitor_2.setHidden(false);
        serial_monitor_1.println(msg);
        serial_monitor_2.println(msg);
    }

    static void logf(const char* fmt, ...) {
        char buf[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
        }
        log(buf);
    }

    static void flushBootQueue() {
        if (_hasErrors && _bootQueueLen > 0) {
            serial_monitor_1.setHidden(false);
            serial_monitor_2.setHidden(false);
            serial_monitor_1.print(_bootQueue);
            serial_monitor_2.print(_bootQueue);
            _bootQueueLen = 0;
            _bootQueue[0] = '\0';
        }
    }

    static void clear() {
        _hasErrors = false;
        _bootQueueLen = 0;
        _bootQueue[0] = '\0';
        if (_rkStarted) {
            serial_monitor_1.setHidden(true);
            serial_monitor_2.setHidden(true);
        }
    }

    static bool hasErrors() {
        return _hasErrors;
    }

private:
    static inline char _bootQueue[1024] = {0};
    static inline size_t _bootQueueLen = 0;
    static inline bool _hasErrors = false;
    static inline bool _rkStarted = false;
};
