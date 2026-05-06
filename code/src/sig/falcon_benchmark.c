#include "sig/falcon_benchmark.h"

static const struct { const char *oqs_name; const char *nist_name; const char *level; } FALCON_VARIANTS[] = {
    { OQS_SIG_alg_falcon_512,  "FN-DSA-512",  "1" },
    { OQS_SIG_alg_falcon_1024, "FN-DSA-1024", "5" },
};
#define N_FALCON (sizeof(FALCON_VARIANTS) / sizeof(FALCON_VARIANTS[0]))

void falcon_benchmark_run_all(FILE *csv_fp, FILE *scale_csv_fp) {
    utils_print_section("FN-DSA (ex-Falcon) - FIPS 206");
    for (size_t i = 0; i < N_FALCON; i++) {
        printf("\n[%zu/%zu] Benchmarking %s ...\n", i+1, N_FALCON, FALCON_VARIANTS[i].nist_name);
        sig_result_t result = {0};
        if (sig_benchmark_run(FALCON_VARIANTS[i].oqs_name, FALCON_VARIANTS[i].nist_name,
                              FALCON_VARIANTS[i].level, &result) == BENCH_OK) {
            sig_benchmark_print(&result);
            sig_benchmark_save_csv(csv_fp, &result);
        } else {
            fprintf(stderr, "[SKIP] %s nao disponivel.\n", FALCON_VARIANTS[i].nist_name);
            continue;
        }
        if (scale_csv_fp) {
            sig_scale_result_t scale = {0};
            if (sig_scale_run(FALCON_VARIANTS[i].oqs_name, FALCON_VARIANTS[i].nist_name,
                              FALCON_VARIANTS[i].level, &scale) == BENCH_OK) {
                sig_scale_print(&scale);
                sig_scale_save_csv(scale_csv_fp, &scale);
            }
        }
    }
}
