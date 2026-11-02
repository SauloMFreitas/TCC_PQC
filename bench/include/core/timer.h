#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/*
 * timer.h - Medicao de tempo de alta resolucao
 *
 *  Plataforma  | API                             | Precisao
 *  Windows     | QueryPerformanceCounter (QPC)   | ~100 ns
 *  macOS       | mach_absolute_time              | ~1 ns
 *  Linux       | clock_gettime(CLOCK_MONOTONIC)  | ~1 ns
 *
 * Todas retornam microssegundos (double).
 * Uso:
 *   double t0 = timer_us();
 *   ... operacao ...
 *   double elapsed = timer_us() - t0;
 */

/* ================================================================
 * WINDOWS - QueryPerformanceCounter
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

#define TIMER_PLATFORM  "Windows (QueryPerformanceCounter)"

/* ================================================================
 * macOS - mach_absolute_time
 * ================================================================ */
#elif defined(__APPLE__) && defined(__MACH__)

#include <mach/mach_time.h>

static inline double timer_us(void) {
    static mach_timebase_info_data_t tb = {0, 0};
    if (tb.denom == 0) {
        mach_timebase_info(&tb);
    }
    uint64_t ticks = mach_absolute_time();
    return (double)ticks * (double)tb.numer / (double)tb.denom / 1e3;
}

#define TIMER_PLATFORM  "macOS (mach_absolute_time)"

/* ================================================================
 * LINUX - clock_gettime(CLOCK_MONOTONIC)
 *
 * _POSIX_C_SOURCE evita conflito com linux/time.h da liboqs.
 * ================================================================ */
#elif defined(__linux__)

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include <time.h>

static inline double timer_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e6 + (double)ts.tv_nsec / 1e3;
}

#define TIMER_PLATFORM  "Linux (clock_gettime CLOCK_MONOTONIC)"

/* ================================================================
 * FALLBACK - outros sistemas POSIX
 * ================================================================ */
#else

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include <time.h>

static inline double timer_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e6 + (double)ts.tv_nsec / 1e3;
}

#define TIMER_PLATFORM  "POSIX (clock_gettime)"

#endif /* plataformas */

/* ================================================================
 * RDTSC - Ciclos de CPU (x86/x86_64)
 * CPUID antes do RDTSC forca serializacao do pipeline.
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

#elif defined(__aarch64__)

static inline uint64_t timer_cycles(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}

#else
static inline uint64_t timer_cycles(void) {
    return (uint64_t)(timer_us() * 1000.0);
}
#endif

/* Conversoes de unidades */
#define US_TO_MS(us)  ((us) / 1e3)
#define US_TO_S(us)   ((us) / 1e6)

/*
 * Estima frequencia da CPU em MHz.
 * Implementada em timer.c — chamada apenas uma vez no banner.
 */
double timer_cpu_freq_mhz(void);

#endif /* TIMER_H */
