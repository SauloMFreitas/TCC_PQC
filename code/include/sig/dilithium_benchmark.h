#ifndef DILITHIUM_BENCHMARK_H
#define DILITHIUM_BENCHMARK_H

#include "sig_benchmark.h"

/* ML-DSA (ex-CRYSTALS-Dilithium) - FIPS 204
 *  ML-DSA-44 | nivel 2 | Dilithium2
 *  ML-DSA-65 | nivel 3 | Dilithium3
 *  ML-DSA-87 | nivel 5 | Dilithium5 */
void dilithium_benchmark_run_all(FILE *csv_fp, FILE *scale_csv_fp);

#endif /* DILITHIUM_BENCHMARK_H */
