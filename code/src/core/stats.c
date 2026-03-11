#include "../include/core/stats.h"


void stats_compute(stats_t *stats, const double *samples, size_t n){
    if (n == 0 || !samples || !stats) {
        fprintf(stderr, "stats_compute: invalid input\n");
        return;
    }

    
}