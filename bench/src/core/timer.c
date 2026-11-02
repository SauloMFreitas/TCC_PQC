#include "core/timer.h"

/*
 * Estima a frequencia do processador em MHz.
 * Conta ciclos RDTSC durante ~100ms e extrapola.
 *
 * Uso tipico:
 *   printf("CPU: %.0f MHz\n", timer_cpu_freq_mhz());
 *
 *   // Converter ciclos em tempo para AMD uProf:
 *   double freq = timer_cpu_freq_mhz();
 *   double tempo_us = (double)ciclos / freq;
 */
double timer_cpu_freq_mhz(void) {
    uint64_t c0 = timer_cycles();
    double   t0 = timer_us();

    while ((timer_us() - t0) < 100000.0)
        ;

    uint64_t c1 = timer_cycles();
    double   t1 = timer_us();

    double elapsed_us = t1 - t0;
    if (elapsed_us <= 0.0) return 0.0;
    return (double)(c1 - c0) / elapsed_us;
}
