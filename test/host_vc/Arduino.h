#ifndef MOCK_ARDUINO_VC_H
#define MOCK_ARDUINO_VC_H

#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

#define DEC 10
#define HEX 16
#define BIN 2
#define OCT 8

#ifndef HIGH
#define HIGH 0x1
#endif
#ifndef LOW
#define LOW 0x0
#endif
#ifndef INPUT
#define INPUT 0x0
#endif
#ifndef OUTPUT
#define OUTPUT 0x1
#endif
#ifndef INPUT_PULLUP
#define INPUT_PULLUP 0x2
#endif
#ifndef INPUT_PULLDOWN
#define INPUT_PULLDOWN 0x3
#endif

extern uint8_t host_gpio_pin_mode[128];
extern uint8_t host_gpio_pin_val[128];

inline void pinMode(uint8_t pin, uint8_t mode) {
    if (pin < 128) host_gpio_pin_mode[pin] = mode;
}
inline void digitalWrite(uint8_t pin, uint8_t val) {
    if (pin < 128) host_gpio_pin_val[pin] = val;
}
inline int digitalRead(uint8_t pin) {
    if (pin < 128) return host_gpio_pin_val[pin];
    return LOW;
}

using String = std::string;

inline char* itoa(int value, char* str, int base) {
    if (base == 16) sprintf(str, "%x", value);
    else sprintf(str, "%d", value);
    return str;
}
inline char* utoa(unsigned int value, char* str, int base) {
    if (base == 16) sprintf(str, "%x", value);
    else sprintf(str, "%u", value);
    return str;
}
inline char* ltoa(long value, char* str, int base) {
    if (base == 16) sprintf(str, "%lx", value);
    else sprintf(str, "%ld", value);
    return str;
}
inline char* ultoa(unsigned long value, char* str, int base) {
    if (base == 16) sprintf(str, "%lx", value);
    else sprintf(str, "%lu", value);
    return str;
}
inline char* dtostrf(double val, signed char width, unsigned char prec, char* sout) {
    char fmt[20];
    sprintf(fmt, "%%%d.%df", width, prec);
    sprintf(sout, fmt, val);
    return sout;
}

extern uint32_t host_virtual_millis;
extern uint32_t host_analog_read_mv;
extern bool     host_radiokit_connected;

inline uint32_t millis() { return host_virtual_millis; }
inline void delay(uint32_t ms) { host_virtual_millis += ms; }
inline uint32_t micros() { return host_virtual_millis * 1000; }
inline int64_t esp_timer_get_time() { return (int64_t)host_virtual_millis * 1000; }

inline uint32_t analogReadMilliVolts(int pin) {
    return host_analog_read_mv; // Default ~3.96V (safe battery)
}

// Math helpers
template<typename T, typename L, typename H>
inline T constrain(T val, L low, H high) {
    if (val < (T)low) return (T)low;
    if (val > (T)high) return (T)high;
    return val;
}

template<typename T>
inline T min(T a, T b) { return (a < b) ? a : b; }

template<typename T>
inline T max(T a, T b) { return (a > b) ? a : b; }

inline int32_t map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max) {
    if (in_max == in_min) return out_min;
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Print and Stream Base Classes
class Print {
public:
    virtual size_t write(uint8_t) { return 1; }
    virtual size_t write(const uint8_t*, size_t s) { return s; }
    size_t print(const std::string& s) { return s.length(); }
    size_t print(const char* s) { return strlen(s); }
    size_t print(int, int = DEC) { return 1; }
    size_t print(unsigned int, int = DEC) { return 1; }
    size_t print(long, int = DEC) { return 1; }
    size_t print(unsigned long, int = DEC) { return 1; }
    size_t println(const std::string& s) { return s.length(); }
    size_t println(const char* s = "") { return strlen(s); }
    size_t println(int, int = DEC) { return 1; }
    size_t println(unsigned int, int = DEC) { return 1; }
    size_t println(long, int = DEC) { return 1; }
    size_t println(unsigned long, int = DEC) { return 1; }
    virtual void flush() {}
};

class Stream : public Print {
public:
    virtual int read() { return -1; }
    virtual int available() { return 0; }
    template<typename... Args>
    void printf(const char* fmt, Args... args) {}
};

class DummySerial : public Stream {};

extern DummySerial Serial;

// FreeRTOS MUX and Task Stubs
typedef uint32_t portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(m) do { __asm__ volatile("" ::: "memory"); } while(0)
#define portEXIT_CRITICAL(m)  do { __asm__ volatile("" ::: "memory"); } while(0)
typedef void* SemaphoreHandle_t;
typedef void* TaskHandle_t;
inline SemaphoreHandle_t xSemaphoreCreateMutex() { return (SemaphoreHandle_t)1; }

#endif // MOCK_ARDUINO_VC_H
