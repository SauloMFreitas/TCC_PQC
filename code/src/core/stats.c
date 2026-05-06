#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "core/stats.h"

static int cmp_double(const void *a, const void *b) {
    double da = *(const double *)a;
    double db = *(const double *)b;
    return (da > db) - (da < db);
}

void stats_compute(const double *samples, int n, stats_t *out) {
    if (n <= 0 || !samples || !out) return;

    double *sorted = malloc(n * sizeof(double));
    memcpy(sorted, samples, n * sizeof(double));
    qsort(sorted, n, sizeof(double), cmp_double);

    out->min_us    = sorted[0];
    out->max_us    = sorted[n - 1];
    out->median_us = (n % 2 == 0)
        ? (sorted[n/2 - 1] + sorted[n/2]) / 2.0
        : sorted[n/2];
    free(sorted);

    double sum = 0.0, sum_sq = 0.0;
    for (int i = 0; i < n; i++) {
        sum    += samples[i];
        sum_sq += samples[i] * samples[i];
    }
    out->mean_us    = sum / n;
    double var      = (sum_sq / n) - (out->mean_us * out->mean_us);
    out->stddev_us  = (var > 0.0) ? sqrt(var) : 0.0;
    out->cv_percent = (out->mean_us > 0.0)
        ? (out->stddev_us / out->mean_us) * 100.0
        : 0.0;
}

void stats_print(const char *label, const stats_t *s) {
    printf("    %-10s | media: %9.2f us  +/-%7.2f  |"
           " min: %8.2f  med: %8.2f  CV: %.1f%%\n",
           label, s->mean_us, s->stddev_us,
           s->min_us, s->median_us, s->cv_percent);
}
