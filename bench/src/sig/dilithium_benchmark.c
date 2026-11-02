#include "sig/dilithium_benchmark.h"

static const struct { const char *oqs_name; const char *nist_name; const char *level; } DILITHIUM_VARIANTS[] = {
    { OQS_SIG_alg_ml_dsa_44, "ML-DSA-44", "2" },
    { OQS_SIG_alg_ml_dsa_65, "ML-DSA-65", "3" },
    { OQS_SIG_alg_ml_dsa_87, "ML-DSA-87", "5" },
};
#define N_DILITHIUM (sizeof(DILITHIUM_VARIANTS) / sizeof(DILITHIUM_VARIANTS[0]))

void dilithium_benchmark_run_all(FILE *csv_fp, FILE *scale_csv_fp) {
    utils_print_section("ML-DSA (ex-CRYSTALS-Dilithium) - FIPS 204");
    for (size_t i = 0; i < N_DILITHIUM; i++) {
        printf("\n[%zu/%zu] Benchmarking %s ...\n", i+1, N_DILITHIUM, DILITHIUM_VARIANTS[i].nist_name);
        sig_result_t result = {0};
        if (sig_benchmark_run(DILITHIUM_VARIANTS[i].oqs_name, DILITHIUM_VARIANTS[i].nist_name,
                              DILITHIUM_VARIANTS[i].level, &result) == BENCH_OK) {
            sig_benchmark_print(&result);
            sig_benchmark_save_csv(csv_fp, &result);
        } else {
            fprintf(stderr, "[SKIP] %s nao disponivel.\n", DILITHIUM_VARIANTS[i].nist_name);
            continue;
        }
        if (scale_csv_fp) {
            sig_scale_result_t scale = {0};
            if (sig_scale_run(DILITHIUM_VARIANTS[i].oqs_name, DILITHIUM_VARIANTS[i].nist_name,
                              DILITHIUM_VARIANTS[i].level, &scale) == BENCH_OK) {
                sig_scale_print(&scale);
                sig_scale_save_csv(scale_csv_fp, &scale);
            }
        }
    }
}
