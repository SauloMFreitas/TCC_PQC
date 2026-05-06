#include "kem/kyber_benchmark.h"

/*
 * ML-KEM (ex-CRYSTALS-Kyber) - FIPS 203
 *
 * Mapeamento:
 *   OQS_KEM_alg_ml_kem_512  -> ML-KEM-512  (nivel 1)
 *   OQS_KEM_alg_ml_kem_768  -> ML-KEM-768  (nivel 3)
 *   OQS_KEM_alg_ml_kem_1024 -> ML-KEM-1024 (nivel 5)
 */
static const struct {
    const char *oqs_name;
    const char *nist_name;
    const char *level;
} KYBER_VARIANTS[] = {
    { OQS_KEM_alg_ml_kem_512,  "ML-KEM-512",  "1" },
    { OQS_KEM_alg_ml_kem_768,  "ML-KEM-768",  "3" },
    { OQS_KEM_alg_ml_kem_1024, "ML-KEM-1024", "5" },
};
#define N_KYBER (sizeof(KYBER_VARIANTS) / sizeof(KYBER_VARIANTS[0]))

void kyber_benchmark_run_all(FILE *csv_fp) {
    utils_print_section("ML-KEM (ex-CRYSTALS-Kyber) - FIPS 203");
    for (size_t i = 0; i < N_KYBER; i++) {
        printf("\n[%zu/%zu] Benchmarking %s ...\n",
               i+1, N_KYBER, KYBER_VARIANTS[i].nist_name);
        kem_result_t result = {0};
        if (kem_benchmark_run(KYBER_VARIANTS[i].oqs_name,
                              KYBER_VARIANTS[i].nist_name,
                              KYBER_VARIANTS[i].level, &result) == BENCH_OK) {
            kem_benchmark_print(&result);
            kem_benchmark_save_csv(csv_fp, &result);
        } else {
            fprintf(stderr, "[SKIP] %s nao disponivel.\n", KYBER_VARIANTS[i].nist_name);
        }
    }
}
