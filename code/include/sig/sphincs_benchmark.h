#ifndef SPHINCS_BENCHMARK_H
#define SPHINCS_BENCHMARK_H

#include "sig_benchmark.h"

/*
 * sphincs_benchmark.h — Benchmark para SLH-DSA (ex-SPHINCS+)
 *
 * Mapeamento de nomes:
 *   Nome antigo (liboqs)       │ Nome NIST             │ Nível
 *   ───────────────────────────┼───────────────────────┼───────
 *   SPHINCS+-SHAKE-128s        │ SLH-DSA-SHAKE-128s    │  1
 *   SPHINCS+-SHAKE-128f        │ SLH-DSA-SHAKE-128f    │  1
 *   SPHINCS+-SHAKE-192s        │ SLH-DSA-SHAKE-192s    │  3
 *   SPHINCS+-SHAKE-192f        │ SLH-DSA-SHAKE-192f    │  3
 *   SPHINCS+-SHAKE-256s        │ SLH-DSA-SHAKE-256s    │  5
 *   SPHINCS+-SHAKE-256f        │ SLH-DSA-SHAKE-256f    │  5
 *
 * Sufixo -s = small  → assinatura menor, operações mais lentas
 * Sufixo -f = fast   → assinatura maior, operações mais rápidas
 *
 * Base matemática: Hash-based (Hypertree de Merkle + FORS + WOTS+)
 * Padronizado em: FIPS 205 (agosto 2024)
 *
 * Característica marcante: segurança baseada APENAS em funções hash
 * (sem álgebra de reticulados). Resistente mesmo se lattices quebrarem.
 * Desvantagem: assinaturas grandes e sign lento comparado aos demais.
 */

/**
 * Executa benchmark das seis variantes de SLH-DSA (SHAKE).
 */
void sphincs_benchmark_run_all(FILE *csv_fp);

#endif /* SPHINCS_BENCHMARK_H */