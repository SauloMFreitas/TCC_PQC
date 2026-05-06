#ifndef GENERAL_INCLUDE_H
#define GENERAL_INCLUDE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include <oqs/oqs.h>

#include "core/timer.h"
#include "core/stats.h"
#include "core/utils.h"

typedef enum {
    BENCH_OK    =  0,
    BENCH_ERROR = -1
} bench_status_t;

typedef struct {
    char   nist_name[64];
    char   oqs_name[64];
    char   nist_level[8];
    size_t pk_bytes;
    size_t sk_bytes;
    size_t ct_bytes;
    size_t ss_bytes;
    stats_t keygen;
    stats_t encaps;
    stats_t decaps;
    double  memory_kb;
} kem_result_t;

typedef struct {
    char   nist_name[64];
    char   oqs_name[64];
    char   nist_level[8];
    size_t pk_bytes;
    size_t sk_bytes;
    size_t sig_max_bytes;
    size_t sig_real_bytes;
    stats_t keygen;
    stats_t sign;
    stats_t verify;
    double  memory_kb;
} sig_result_t;

typedef struct {
    size_t  msg_size;
    stats_t sign;
    stats_t verify;
} sig_scale_point_t;

typedef struct {
    char nist_name[64];
    char nist_level[8];
    sig_scale_point_t points[SCALE_MSG_SIZES_COUNT];
} sig_scale_result_t;

#endif /* GENERAL_INCLUDE_H */
