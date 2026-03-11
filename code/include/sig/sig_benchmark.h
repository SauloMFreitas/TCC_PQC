#ifndef SIG_BENCHMARK_H
#define SIG_BENCHMARK_H

#include "general_include.h"

/*
 * sig_benchmark.h — Interface genérica para benchmarks de Assinatura Digital
 */

/**
 * Executa o benchmark completo para um algoritmo de assinatura.
 *
 * @param oqs_alg_name  String de algoritmo aceita pela liboqs
 * @param nist_name     Nome legível (ex: "ML-DSA-65")
 * @param nist_level    Nível de segurança NIST (ex: "3")
 * @param out           Estrutura de resultado preenchida
 * @return BENCH_OK ou BENCH_ERROR
 */
bench_status_t sig_benchmark_run(const char *oqs_alg_name,
                                 const char *nist_name,
                                 const char *nist_level,
                                 sig_result_t *out);

/**
 * Imprime resultado formatado no terminal.
 */
void sig_benchmark_print(const sig_result_t *r);

/**
 * Escreve uma linha de resultado no arquivo CSV.
 */
void sig_benchmark_save_csv(FILE *fp, const sig_result_t *r);

#endif /* SIG_BENCHMARK_H */