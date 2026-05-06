#include "sig/sphincs_benchmark.h"

static const struct {
    const char *oqs_name;
    const char *nist_name;
    const char *level;
} SPHINCS_VARIANTS[] = {
    { OQS_SIG_alg_sphincs_shake_128s_simple, "SLH-DSA-SHAKE-128s", "1" },
    { OQS_SIG_alg_sphincs_shake_128f_simple, "SLH-DSA-SHAKE-128f", "1" },
    { OQS_SIG_alg_sphincs_shake_192s_simple, "SLH-DSA-SHAKE-192s", "3" },
    { OQS_SIG_alg_sphincs_shake_192f_simple, "SLH-DSA-SHAKE-192f", "3" },
    { OQS_SIG_alg_sphincs_shake_256s_simple, "SLH-DSA-SHAKE-256s", "5" },
    { OQS_SIG_alg_sphincs_shake_256f_simple, "SLH-DSA-SHAKE-256f", "5" },
};
#define N_SPHINCS (sizeof(SPHINCS_VARIANTS) / sizeof(SPHINCS_VARIANTS[0]))

void sphincs_benchmark_run_all(FILE *csv_fp, FILE *scale_csv_fp) {
    utils_print_section("SLH-DSA (ex-SPHINCS+) - FIPS 205");
    printf("  [AVISO] Sign pode levar dezenas de ms por iteracao.\n");

    for (size_t i = 0; i < N_SPHINCS; i++) {
        printf("\n[%zu/%zu] Benchmarking %s ...\n",
               i+1, N_SPHINCS, SPHINCS_VARIANTS[i].nist_name);

        sig_result_t result = {0};
        if (sig_benchmark_run(SPHINCS_VARIANTS[i].oqs_name,
                              SPHINCS_VARIANTS[i].nist_name,
                              SPHINCS_VARIANTS[i].level, &result) == BENCH_OK) {
            sig_benchmark_print(&result);
            sig_benchmark_save_csv(csv_fp, &result);
        } else {
            fprintf(stderr, "[SKIP] %s nao disponivel.\n",
                    SPHINCS_VARIANTS[i].nist_name);
            continue;
        }

        if (scale_csv_fp) {
            sig_scale_result_t scale = {0};
            if (sig_scale_run(SPHINCS_VARIANTS[i].oqs_name,
                              SPHINCS_VARIANTS[i].nist_name,
                              SPHINCS_VARIANTS[i].level, &scale) == BENCH_OK) {
                sig_scale_print(&scale);
                sig_scale_save_csv(scale_csv_fp, &scale);
            }
        }
    }
}
