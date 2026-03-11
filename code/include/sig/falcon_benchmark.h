#ifndef FALCON_BENCHMARK_H
#define FALCON_BENCHMARK_H

#include "sig_benchmark.h"

/*
 * falcon_benchmark.h — Benchmark para FN-DSA (ex-Falcon)
 *
 * Mapeamento de nomes:
 *   Nome antigo (liboqs) │ Nome padronizado NIST │ Nível │ Segurança equiv.
 *   ─────────────────────┼───────────────────────┼───────┼─────────────────
 *   Falcon-512           │ FN-DSA-512            │  1    │ AES-128
 *   Falcon-1024          │ FN-DSA-1024           │  5    │ AES-256
 *
 * Base matemática: NTRU Lattice + Fast Fourier Sampling
 * Padronizado em: FIPS 206 (agosto 2024)
 *
 * Característica marcante: assinaturas muito compactas.
 * Atenção: KeyGen usa aritmética de ponto flutuante de alta precisão,
 * o que pode causar variância maior nos tempos de geração de chave.
 * Sign é probabilístico (aleatoriedade em cada assinatura).
 */

/**
 * Executa benchmark dos dois tamanhos de FN-DSA (Falcon).
 */
void falcon_benchmark_run_all(FILE *csv_fp);

#endif /* FALCON_BENCHMARK_H */