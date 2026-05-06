#include "kem/kem_benchmark.h"

bench_status_t kem_benchmark_run(const char *oqs_alg_name,
                                 const char *nist_name,
                                 const char *nist_level,
                                 kem_result_t *out)
{
    OQS_KEM *kem = OQS_KEM_new(oqs_alg_name);
    if (!kem) {
        fprintf(stderr, "[ERRO] KEM nao disponivel: %s\n", oqs_alg_name);
        return BENCH_ERROR;
    }

    uint8_t *pk  = malloc(kem->length_public_key);
    uint8_t *sk  = malloc(kem->length_secret_key);
    uint8_t *ct  = malloc(kem->length_ciphertext);
    uint8_t *ss1 = malloc(kem->length_shared_secret);
    uint8_t *ss2 = malloc(kem->length_shared_secret);
    double *t_keygen = malloc(BENCH_ITERATIONS * sizeof(double));
    double *t_encaps = malloc(BENCH_ITERATIONS * sizeof(double));
    double *t_decaps = malloc(BENCH_ITERATIONS * sizeof(double));

    if (!pk || !sk || !ct || !ss1 || !ss2 || !t_keygen || !t_encaps || !t_decaps) {
        fprintf(stderr, "[ERRO] Falha de alocacao para %s\n", oqs_alg_name);
        goto fail;
    }

    snprintf(out->nist_name,  sizeof(out->nist_name),  "%s", nist_name);
    snprintf(out->oqs_name,   sizeof(out->oqs_name),   "%s", oqs_alg_name);
    snprintf(out->nist_level, sizeof(out->nist_level), "%s", nist_level);
    out->pk_bytes = kem->length_public_key;
    out->sk_bytes = kem->length_secret_key;
    out->ct_bytes = kem->length_ciphertext;
    out->ss_bytes = kem->length_shared_secret;

    /* Aquecimento */
    for (int i = 0; i < BENCH_WARMUP; i++) {
        OQS_KEM_keypair(kem, pk, sk);
        OQS_KEM_encaps(kem, ct, ss1, pk);
        OQS_KEM_decaps(kem, ss2, ct, sk);
    }

    double mem_before = utils_memory_kb();

    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        double t0, t1;

        t0 = timer_us();
        if (OQS_KEM_keypair(kem, pk, sk) != OQS_SUCCESS) goto fail;
        t1 = timer_us();
        t_keygen[i] = t1 - t0;

        t0 = timer_us();
        if (OQS_KEM_encaps(kem, ct, ss1, pk) != OQS_SUCCESS) goto fail;
        t1 = timer_us();
        t_encaps[i] = t1 - t0;

        t0 = timer_us();
        if (OQS_KEM_decaps(kem, ss2, ct, sk) != OQS_SUCCESS) goto fail;
        t1 = timer_us();
        t_decaps[i] = t1 - t0;

        /* Verificacao de corretude */
        if (memcmp(ss1, ss2, kem->length_shared_secret) != 0) {
            fprintf(stderr, "[ERRO] Shared secrets divergem em %s (iter %d)\n",
                    oqs_alg_name, i);
            goto fail;
        }
    }

    stats_compute(t_keygen, BENCH_ITERATIONS, &out->keygen);
    stats_compute(t_encaps, BENCH_ITERATIONS, &out->encaps);
    stats_compute(t_decaps, BENCH_ITERATIONS, &out->decaps);
    out->memory_kb = utils_memory_kb() - mem_before;

    free(t_keygen); free(t_encaps); free(t_decaps);
    free(pk); free(ct); free(ss1); free(ss2);
    OQS_MEM_secure_free(sk, kem->length_secret_key);
    OQS_KEM_free(kem);
    return BENCH_OK;

fail:
    free(t_keygen); free(t_encaps); free(t_decaps);
    free(pk); free(ct); free(ss1); free(ss2);
    if (sk) OQS_MEM_secure_free(sk, kem->length_secret_key);
    OQS_KEM_free(kem);
    return BENCH_ERROR;
}

void kem_benchmark_print(const kem_result_t *r) {
    printf("\n  +------------------------------------------------------+\n");
    printf("  | %-20s  OQS: %-26s|\n", r->nist_name, r->oqs_name);
    printf("  | Nivel NIST: %-3s                                      |\n", r->nist_level);
    printf("  +------------------------------------------------------+\n");
    printf("  |   Chave publica : %6zu B  | Chave secreta: %6zu B  |\n",
           r->pk_bytes, r->sk_bytes);
    printf("  |   Ciphertext    : %6zu B  | Shared secret: %6zu B  |\n",
           r->ct_bytes, r->ss_bytes);
    printf("  +------------------------------------------------------+\n");
    printf("  | Tempos (us)  | media +/-dp    min      mediana        |\n");
    stats_print("KeyGen", &r->keygen);
    stats_print("Encaps", &r->encaps);
    stats_print("Decaps", &r->decaps);
    printf("  | Memoria heap: %.2f KB                               |\n", r->memory_kb);
    printf("  +------------------------------------------------------+\n");
}

void kem_benchmark_save_csv(FILE *fp, const kem_result_t *r) {
    if (!fp) return;
    fprintf(fp, "%s,%s,%s,%zu,%zu,%zu,%zu,"
                "%.3f,%.3f,%.3f,%.3f,"
                "%.3f,%.3f,%.3f,%.3f,"
                "%.3f,%.3f,%.3f,%.3f,%.3f\n",
            r->nist_name, r->oqs_name, r->nist_level,
            r->pk_bytes, r->sk_bytes, r->ct_bytes, r->ss_bytes,
            r->keygen.mean_us, r->keygen.stddev_us, r->keygen.min_us, r->keygen.median_us,
            r->encaps.mean_us, r->encaps.stddev_us, r->encaps.min_us, r->encaps.median_us,
            r->decaps.mean_us, r->decaps.stddev_us, r->decaps.min_us, r->decaps.median_us,
            r->memory_kb);
}
