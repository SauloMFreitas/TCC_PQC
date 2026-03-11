#ifndef DILITHIUM_BENCHMARK_H
#define DILITHIUM_BENCHMARK_H

#include "sig_benchmark.h"

/*
 * dilithium_benchmark.h — Benchmark para ML-DSA (ex-CRYSTALS-Dilithium)
 *
 * Mapeamento de nomes:
 *   Nome antigo (liboqs)  │ Nome padronizado NIST │ Nível │ Segurança equiv.
 *   ──────────────────────┼───────────────────────┼───────┼─────────────────
 *   Dilithium2            │ ML-DSA-44             │  2    │ SHA3-224
 *   Dilithium3            │ ML-DSA-65             │  3    │ AES-192
 *   Dilithium5            │ ML-DSA-87             │  5    │ AES-256
 *
 * Base matemática: Module Learning With Errors (MLWE) + Module SIS
 * Padronizado em: FIPS 204 (agosto 2024)
 *
 * Característica: assinatura determinística (sem aleatoriedade em sign)
 * Comparado ao Falcon: chaves e assinaturas maiores, mas implementação
 * mais simples e sem aritmética de ponto flutuante.
 */

/**
 * Executa benchmark dos três níveis de ML-DSA.
 */
void dilithium_benchmark_run_all(FILE *csv_fp);

#endif /* DILITHIUM_BENCHMARK_H */