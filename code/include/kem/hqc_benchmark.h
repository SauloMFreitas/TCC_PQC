#include "kem_benchmark.h"
#ifndef HQC_BENCHMARK_H
#define HQC_BENCHMARK_H

#include "kem_benchmark.h"

/*
 * hqc_benchmark.h — Benchmark específico para HQC
 *
 * Mapeamento de nomes:
 *   Nome (liboqs) │ Nome NIST (informal) │ Nível │ Segurança equiv.
 *   ──────────────┼──────────────────────┼───────┼─────────────────
 *   HQC-128       │ HQC-128              │  1    │ AES-128
 *   HQC-192       │ HQC-192              │  3    │ AES-192
 *   HQC-256       │ HQC-256              │  5    │ AES-256
 *
 * Base matemática: Code-based (LRPC codes — Low Rank Parity Check)
 * Status NIST: 4ª Rodada (candidato a padronização futura)
 *
 * Diferencial frente ao ML-KEM:
 *   - Base matemática diferente (não-lattice) → diversidade algorítmica
 *   - Chaves e ciphertext maiores
 *   - Adequado para cenários que exigem diversificação de primitivas
 */

/**
 * Executa benchmark dos três níveis de HQC e escreve no CSV.
 * @param csv_fp  Arquivo CSV já aberto (pode ser NULL para skip)
 */
void hqc_benchmark_run_all(FILE *csv_fp);

#endif /* HQC_BENCHMARK_H */