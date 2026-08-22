#ifndef ARDUINO_H_STUB
#define ARDUINO_H_STUB

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
#include <algorithm>

#include <type_traits>

template<typename A, typename B>
inline typename std::common_type<A, B>::type min(A a, B b) {
    return (a < b) ? a : b;
}

template<typename A, typename B>
inline typename std::common_type<A, B>::type max(A a, B b) {
    return (a > b) ? a : b;
}

template<typename T, typename L, typename H>
inline T constrain(T amt, L low, H high) {
    if (amt < (T)low) return (T)low;
    if (amt > (T)high) return (T)high;
    return amt;
}
#endif

static inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
    if (in_max == in_min) return out_min;
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#ifdef __cplusplus
extern "C" {
#endif

// Global virtual millis clock for host testing
extern uint32_t host_virtual_millis;

static inline uint32_t millis(void) {
    return host_virtual_millis;
}

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
class DummySerial {
public:
    template<typename... Args>
    void print(Args... args) {}

    template<typename... Args>
    void println(Args... args) {}

    template<typename... Args>
    void printf(Args... args) {}
};

extern DummySerial Serial;
#endif

typedef uint32_t portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
// Use compiler memory barriers to prevent GCC's tree-dse from eliminating
// stores to member variables inside "critical sections" (e.g. renderBlock
// write-back of state/startPos). Without these, -O1+ incorrectly treats
// those stores as dead.
#define portENTER_CRITICAL(m) do { __asm__ volatile("" ::: "memory"); } while(0)
#define portEXIT_CRITICAL(m)  do { __asm__ volatile("" ::: "memory"); } while(0)
#define portENTER_CRITICAL_ISR(m) do { __asm__ volatile("" ::: "memory"); } while(0)
#define portEXIT_CRITICAL_ISR(m)  do { __asm__ volatile("" ::: "memory"); } while(0)

#endif // ARDUINO_H_STUB
