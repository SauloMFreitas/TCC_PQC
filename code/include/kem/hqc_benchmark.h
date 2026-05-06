#ifndef HQC_BENCHMARK_H
#define HQC_BENCHMARK_H

#include "kem_benchmark.h"

/*
 * HQC - 4a Rodada NIST (Code-based, LRPC)
 *
 *  HQC-128 | nivel 1 | AES-128
 *  HQC-192 | nivel 3 | AES-192
 *  HQC-256 | nivel 5 | AES-256
 */
void hqc_benchmark_run_all(FILE *csv_fp);

#endif /* HQC_BENCHMARK_H */
