#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/*
 * timer.h — high resolution time meassurement across platforms
 *
 * best available timer on each platform, with microsecond precision or better.:
 *
 *  ┌─────────────────┬──────────────────────────────────┬───────────────┐
 *  │ platform        │ API                              │ Precision     │
 *  ├─────────────────┼──────────────────────────────────┼───────────────┤
 *  │ Windows         │ QueryPerformanceCounter (QPC)    │ ~100 ns       │
 *  │ macOS           │ mach_absolute_time               │ ~1 ns         │
 *  │ Linux           │ clock_gettime(CLOCK_MONOTONIC)   │ ~1 ns         │
 *  └─────────────────┴──────────────────────────────────┴───────────────┘
 *
 * all functions return the elapsed time in microseconds (µs) as a double for easy statistics.
 *
 * Uso:
 *   double t0 = timer_us();
 *   ... operations ...
 *   double elapsed_us = timer_us() - t0;
 */

/* ================================================================
 * WINDOWS — QueryPerformanceCounter
 *
 * high resolution timer available on Windows.
 * The frequency is queried once and cached.
 * ================================================================ */
#if defined(_WIN32) || defined(_WIN64)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

static inline double timer_us(void) {
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart * 1e6 / (double)freq.QuadPart;
}

/* log platform identifier */
#define TIMER_PLATFORM  "Windows (QueryPerformanceCounter)"

/* ================================================================
 * macOS — mach_absolute_time
 *
 * high resolution API for macOS (~1 ns).
 * mach_timebase_info nanoseconds conversion available.
 * better than clock_gettime 'cause avoid overhead and syscalls.
 * ================================================================ */
#elif defined(__APPLE__) && defined(__MACH__)

#include <mach/mach_time.h>

static inline double timer_us(void) {
    static mach_timebase_info_data_t tb = {0, 0};
    if (tb.denom == 0) {
        mach_timebase_info(&tb);
    }
    uint64_t ticks = mach_absolute_time();
    /* ticks conversions → ns → µs */
    return (double)ticks * (double)tb.numer / (double)tb.denom / 1e3;
}

#define TIMER_PLATFORM  "macOS (mach_absolute_time)"

/* ================================================================
 * LINUX — clock_gettime(CLOCK_MONOTONIC)
 *
 *  ~1 ns resolution in most modern kernels.
 * CLOCK_MONOTONIC garantees monotonicity and is unaffected by system clock changes (e.g., NTP adjustments).
 * ================================================================ */
#elif defined(__linux__)

#include <time.h>

static inline double timer_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e6 + (double)ts.tv_nsec / 1e3;
}

#define TIMER_PLATFORM  "Linux (clock_gettime CLOCK_MONOTONIC)"

/* ================================================================
 * FALLBACK — other POSIX methods (FreeBSD, etc.)
 * ================================================================ */
#else

#include <time.h>

static inline double timer_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e6 + (double)ts.tv_nsec / 1e3;
}

#define TIMER_PLATFORM  "POSIX genérico (clock_gettime)"

#endif /* plataformas */

/**================================================================
 * RDTSC — CPU cycles (x86 / x86_64, todas as plataformas)
 *
 * Útil para análise com AMD uProf (Windows) ou perf (Linux).
 * CPUID antes do RDTSC força serialização do pipeline e evita
 * reordenamento especulativo que distorceria medições curtas.
 *
 *! ATENÇÃO: in modern CPU with frequency scaling, cycles don't
 *! translate in time — use timer_us() for real time measurements
 *! and timer_cycles() only for instruction counting.
 * ================================================================ */
#if defined(__x86_64__) || defined(__i386__)

static inline uint64_t timer_cycles(void) {
    uint32_t lo, hi;
    __asm__ volatile (
        "cpuid\n\t"
        "rdtsc\n\t"
        : "=a"(lo), "=d"(hi)
        :: "%rbx", "%rcx"
    );
    return ((uint64_t)hi << 32) | lo;
}

/* ARM64 — CNTVCT_EL0 (contador de ciclos do sistema) */
#elif defined(__aarch64__)

static inline uint64_t timer_cycles(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}

#else
/* Fallback: converte µs em "ciclos" aproximados */
static inline uint64_t timer_cycles(void) {
    return (uint64_t)(timer_us() * 1000.0);
}
#endif

/* ================================================================
 * units conversion macros
 * ================================================================ */
#define US_TO_MS(us)    ((us) / 1e3)
#define US_TO_S(us)     ((us) / 1e6)
#define MS_TO_US(ms)    ((ms) * 1e3)

#endif /* TIMER_H */