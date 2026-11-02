#ifndef KYBER_BENCHMARK_H
#define KYBER_BENCHMARK_H

#include "kem_benchmark.h"

/*
 * ML-KEM (ex-CRYSTALS-Kyber) - FIPS 203
 *
 *  ML-KEM-512  | nivel 1 | AES-128
 *  ML-KEM-768  | nivel 3 | AES-192
 *  ML-KEM-1024 | nivel 5 | AES-256
 */
void kyber_benchmark_run_all(FILE *csv_fp);

#endif /* KYBER_BENCHMARK_H */
