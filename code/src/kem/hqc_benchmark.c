#include "kem/hqc_benchmark.h"

/*
 * HQC - 4a Rodada NIST (Code-based, LRPC codes)
 *
 * Diferenca em relacao ao ML-KEM:
 *   - Baseado em codigos corretores de erros (nao lattice)
 *   - Chaves e ciphertexts consideravelmente maiores
 *   - Diversificacao: seguro mesmo se reticulados forem comprometidos
 */
static const struct {
    const char *oqs_name;
    const char *nist_name;
    const char *level;
} HQC_VARIANTS[] = {
    { OQS_KEM_alg_hqc_128, "HQC-128", "1" },
    { OQS_KEM_alg_hqc_192, "HQC-192", "3" },
    { OQS_KEM_alg_hqc_256, "HQC-256", "5" },
};
#define N_HQC (sizeof(HQC_VARIANTS) / sizeof(HQC_VARIANTS[0]))

void hqc_benchmark_run_all(FILE *csv_fp) {
    utils_print_section("HQC - 4a Rodada NIST (Code-based)");
    for (size_t i = 0; i < N_HQC; i++) {
        printf("\n[%zu/%zu] Benchmarking %s ...\n",
               i+1, N_HQC, HQC_VARIANTS[i].nist_name);
        kem_result_t result = {0};
        if (kem_benchmark_run(HQC_VARIANTS[i].oqs_name,
                              HQC_VARIANTS[i].nist_name,
                              HQC_VARIANTS[i].level, &result) == BENCH_OK) {
            kem_benchmark_print(&result);
            kem_benchmark_save_csv(csv_fp, &result);
        } else {
            fprintf(stderr, "[SKIP] %s - compile liboqs com -DOQS_ENABLE_KEM_HQC=ON\n",
                    HQC_VARIANTS[i].nist_name);
        }
    }
}
