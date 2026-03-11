#ifndef KEM_BENCHMARK_H
#define KEM_BENCHMARK_H

#include "../general_include.h"

/*
 * kem_benchmark.h — Interface genérica para benchmarks de KEM
 *
 * Função central que qualquer benchmark de KEM específico chama
 * internamente. Também expõe funções de saída (print / CSV).
 */

/**
 * Executa o benchmark completo para um KEM identificado por oqs_alg_name.
 *
 * @param oqs_alg_name  String de algoritmo aceita pela liboqs
 *                      (ex: OQS_KEM_alg_ml_kem_768)
 * @param nist_name     Nome legível para exibição (ex: "ML-KEM-768")
 * @param nist_level    Nível de segurança NIST como string (ex: "3")
 * @param out           Estrutura de resultado preenchida pela função
 * @return BENCH_OK em sucesso, BENCH_ERROR em falha
 */
bench_status_t kem_benchmark_run(const char *oqs_alg_name,
                                 const char *nist_name,
                                 const char *nist_level,
                                 kem_result_t *out);

/**
 * Imprime resultado formatado no terminal.
 */
void kem_benchmark_print(const kem_result_t *r);

/**
 * Escreve uma linha de resultado no arquivo CSV.
 */
void kem_benchmark_save_csv(FILE *fp, const kem_result_t *r);

#endif /* KEM_BENCHMARK_H */