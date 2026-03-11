#ifndef STATS_H
#define STATS_H

#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>


/*
* stats.h — statistics for benchmarking
*/


/**
 * statistics structure for benchmark results
 */
typedef struct
{
    double mean_us;      // mean time in microseconds
    double stddev_us;    // standard deviation in microseconds
    double min_us;       // minimum time in microseconds
    double max_us;       // maximum time in microseconds
    double median_us;    // median time in microseconds
    double cv_us;        // coefficient of variation % (stddev / mean)
}stats_t;

/**
 * compute statistics in a sample array of microsecond timings
 * @param stats output structure to hold the computed statistics
 * @param samples array of microsecond timings
 * @param n number of samples in the array
 * !ATENTION: the function does not change the array of samples, but make a copy of it to compute the median
 */
void stats_compute(stats_t *stats, const double *samples, size_t n);


/**
 * print the computed statistics in a human-readable format
 * @param label a label to identify the benchmark (e.g., "KeyGen")
 * @param stats the statistics structure to print
 */
void stats_print(const char *label, const stats_t *s);

#endif /* STATS_H */