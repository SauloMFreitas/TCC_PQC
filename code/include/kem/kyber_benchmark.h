#ifndef KYBER_BENCHMARK_H
#define KYBER_BENCHMARK_H

#include "kem_benchmark.h"

/*
 * kyber_benchmark.h — Benchmark específico para ML-KEM (ex-CRYSTALS-Kyber)
 *
 * Mapeamento de nomes:
 *   Nome antigo (liboqs) │ Nome padronizado NIST │ Nível │ Segurança equiv.
 *   ─────────────────────┼───────────────────────┼───────┼─────────────────
 *   Kyber512             │ ML-KEM-512            │  1    │ AES-128
 *   Kyber768             │ ML-KEM-768            │  3    │ AES-192
 *   Kyber1024            │ ML-KEM-1024           │  5    │ AES-256
 *
 * Base matemática: Module Learning With Errors (MLWE)
 * Padronizado em: FIPS 203 (agosto 2024)
 */

/**
 * Executa benchmark dos três níveis de ML-KEM e escreve no CSV.
 * @param csv_fp  Arquivo CSV já aberto (pode ser NULL para skip)
 */
void kyber_benchmark_run_all(FILE *csv_fp);

#endif /* KYBER_BENCHMARK_H */