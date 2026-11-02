#ifndef FALCON_BENCHMARK_H
#define FALCON_BENCHMARK_H

#include "sig_benchmark.h"

/* FN-DSA (ex-Falcon) - FIPS 206
 *  FN-DSA-512  | nivel 1
 *  FN-DSA-1024 | nivel 5
 * KeyGen usa Fast Fourier Sampling com rejeicao - CV% alto e normal. */
void falcon_benchmark_run_all(FILE *csv_fp, FILE *scale_csv_fp);

#endif /* FALCON_BENCHMARK_H */
