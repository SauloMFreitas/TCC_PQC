#ifndef KEM_BENCHMARK_H
#define KEM_BENCHMARK_H

#include "general_include.h"

bench_status_t kem_benchmark_run(const char *oqs_alg_name,
                                 const char *nist_name,
                                 const char *nist_level,
                                 kem_result_t *out);

void kem_benchmark_print(const kem_result_t *r);
void kem_benchmark_save_csv(FILE *fp, const kem_result_t *r);

#endif /* KEM_BENCHMARK_H */
