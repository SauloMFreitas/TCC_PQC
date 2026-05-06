#ifndef SIG_BENCHMARK_H
#define SIG_BENCHMARK_H

#include "general_include.h"

bench_status_t sig_benchmark_run(const char *oqs_alg_name,
                                 const char *nist_name,
                                 const char *nist_level,
                                 sig_result_t *out);
void sig_benchmark_print(const sig_result_t *r);
void sig_benchmark_save_csv(FILE *fp, const sig_result_t *r);

bench_status_t sig_scale_run(const char *oqs_alg_name,
                             const char *nist_name,
                             const char *nist_level,
                             sig_scale_result_t *out);
void sig_scale_print(const sig_scale_result_t *r);
void sig_scale_save_csv(FILE *fp, const sig_scale_result_t *r);

#endif /* SIG_BENCHMARK_H */
