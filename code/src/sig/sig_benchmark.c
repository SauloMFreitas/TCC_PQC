#include "sig/sig_benchmark.h"

bench_status_t sig_benchmark_run(const char *oqs_alg_name,
                                 const char *nist_name,
                                 const char *nist_level,
                                 sig_result_t *out)
{
    OQS_SIG *sig = OQS_SIG_new(oqs_alg_name);
    if (!sig) { fprintf(stderr, "[ERRO] Assinatura nao disponivel: %s\n", oqs_alg_name); return BENCH_ERROR; }

    uint8_t *pk     = malloc(sig->length_public_key);
    uint8_t *sk     = malloc(sig->length_secret_key);
    uint8_t *sigbuf = malloc(sig->length_signature);
    size_t   siglen = 0;
    double *t_keygen = malloc(BENCH_ITERATIONS * sizeof(double));
    double *t_sign   = malloc(BENCH_ITERATIONS * sizeof(double));
    double *t_verify = malloc(BENCH_ITERATIONS * sizeof(double));

    if (!pk || !sk || !sigbuf || !t_keygen || !t_sign || !t_verify) {
        fprintf(stderr, "[ERRO] Falha de alocacao para %s\n", oqs_alg_name); goto fail;
    }

    snprintf(out->nist_name,  sizeof(out->nist_name),  "%s", nist_name);
    snprintf(out->oqs_name,   sizeof(out->oqs_name),   "%s", oqs_alg_name);
    snprintf(out->nist_level, sizeof(out->nist_level), "%s", nist_level);
    out->pk_bytes = sig->length_public_key; out->sk_bytes = sig->length_secret_key;
    out->sig_max_bytes = sig->length_signature;

    for (int i = 0; i < BENCH_WARMUP; i++) {
        OQS_SIG_keypair(sig, pk, sk);
        OQS_SIG_sign(sig, sigbuf, &siglen, BENCH_MSG, MSG_SIZE, sk);
        OQS_SIG_verify(sig, BENCH_MSG, MSG_SIZE, sigbuf, siglen, pk);
    }

    double mem_before = utils_memory_kb();
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        double t0, t1;
        t0 = timer_us(); if (OQS_SIG_keypair(sig, pk, sk) != OQS_SUCCESS) goto fail;
        t1 = timer_us(); t_keygen[i] = t1 - t0;
        t0 = timer_us(); if (OQS_SIG_sign(sig, sigbuf, &siglen, BENCH_MSG, MSG_SIZE, sk) != OQS_SUCCESS) goto fail;
        t1 = timer_us(); t_sign[i] = t1 - t0;
        t0 = timer_us(); if (OQS_SIG_verify(sig, BENCH_MSG, MSG_SIZE, sigbuf, siglen, pk) != OQS_SUCCESS) {
            fprintf(stderr, "[ERRO] Verify falhou em %s (iter %d)\n", oqs_alg_name, i); goto fail;
        }
        t1 = timer_us(); t_verify[i] = t1 - t0;
    }

    stats_compute(t_keygen, BENCH_ITERATIONS, &out->keygen);
    stats_compute(t_sign,   BENCH_ITERATIONS, &out->sign);
    stats_compute(t_verify, BENCH_ITERATIONS, &out->verify);
    out->sig_real_bytes = siglen;
    out->memory_kb = utils_memory_kb() - mem_before;

    free(t_keygen); free(t_sign); free(t_verify);
    free(pk); free(sigbuf); OQS_MEM_secure_free(sk, sig->length_secret_key); OQS_SIG_free(sig);
    return BENCH_OK;
fail:
    free(t_keygen); free(t_sign); free(t_verify);
    free(pk); free(sigbuf); if (sk) OQS_MEM_secure_free(sk, sig->length_secret_key); OQS_SIG_free(sig);
    return BENCH_ERROR;
}

void sig_benchmark_print(const sig_result_t *r) {
    printf("\n  +------------------------------------------------------+\n");
    printf("  | %-20s  OQS: %-26s|\n", r->nist_name, r->oqs_name);
    printf("  | Nivel NIST: %-3s                                      |\n", r->nist_level);
    printf("  +------------------------------------------------------+\n");
    printf("  |   Chave publica : %6zu B  | Chave secreta: %6zu B  |\n", r->pk_bytes, r->sk_bytes);
    printf("  |   Assin. max    : %6zu B  | Assin. real  : %6zu B  |\n", r->sig_max_bytes, r->sig_real_bytes);
    printf("  +------------------------------------------------------+\n");
    printf("  | Tempos (us)  | media +/-dp    min      mediana        |\n");
    stats_print("KeyGen", &r->keygen);
    stats_print("Sign",   &r->sign);
    stats_print("Verify", &r->verify);
    printf("  | Memoria heap: %.2f KB                               |\n", r->memory_kb);
    printf("  +------------------------------------------------------+\n");
}

void sig_benchmark_save_csv(FILE *fp, const sig_result_t *r) {
    if (!fp) return;
    fprintf(fp, "%s,%s,%s,%zu,%zu,%zu,%zu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
            r->nist_name, r->oqs_name, r->nist_level,
            r->pk_bytes, r->sk_bytes, r->sig_max_bytes, r->sig_real_bytes,
            r->keygen.mean_us, r->keygen.stddev_us, r->keygen.min_us, r->keygen.median_us,
            r->sign.mean_us,   r->sign.stddev_us,   r->sign.min_us,   r->sign.median_us,
            r->verify.mean_us, r->verify.stddev_us, r->verify.min_us, r->verify.median_us,
            r->memory_kb);
}

/* ================================================================
 * ESCALABILIDADE DE MENSAGEM
 * ================================================================ */
bench_status_t sig_scale_run(const char *oqs_alg_name,
                             const char *nist_name,
                             const char *nist_level,
                             sig_scale_result_t *out)
{
    OQS_SIG *sig = OQS_SIG_new(oqs_alg_name);
    if (!sig) { fprintf(stderr, "[ERRO] Algoritmo nao disponivel: %s\n", oqs_alg_name); return BENCH_ERROR; }

    uint8_t *pk     = malloc(sig->length_public_key);
    uint8_t *sk     = malloc(sig->length_secret_key);
    uint8_t *sigbuf = malloc(sig->length_signature);
    double *t_sign   = malloc(BENCH_ITERATIONS * sizeof(double));
    double *t_verify = malloc(BENCH_ITERATIONS * sizeof(double));

    if (!pk || !sk || !sigbuf || !t_sign || !t_verify) {
        fprintf(stderr, "[ERRO] Falha de alocacao para escalabilidade de %s\n", oqs_alg_name); goto fail;
    }

    snprintf(out->nist_name,  sizeof(out->nist_name),  "%s", nist_name);
    snprintf(out->nist_level, sizeof(out->nist_level), "%s", nist_level);

    if (OQS_SIG_keypair(sig, pk, sk) != OQS_SUCCESS) {
        fprintf(stderr, "[ERRO] KeyGen falhou para %s\n", oqs_alg_name); goto fail;
    }

    printf("[INFO] Escalabilidade %s - %d tamanhos x %d iteracoes\n",
           nist_name, SCALE_MSG_SIZES_COUNT, BENCH_ITERATIONS);

    for (int s = 0; s < SCALE_MSG_SIZES_COUNT; s++) {
        size_t msg_len = SCALE_MSG_SIZES[s];
        uint8_t *msg = malloc(msg_len);
        if (!msg) { fprintf(stderr, "[ERRO] Falha ao alocar %zu bytes\n", msg_len); goto fail; }
        for (size_t b = 0; b < msg_len; b++) msg[b] = (uint8_t)(b & 0xFF);

        size_t siglen = 0;
        for (int w = 0; w < BENCH_WARMUP; w++) {
            OQS_SIG_sign(sig, sigbuf, &siglen, msg, msg_len, sk);
            OQS_SIG_verify(sig, msg, msg_len, sigbuf, siglen, pk);
        }

        for (int i = 0; i < BENCH_ITERATIONS; i++) {
            double t0, t1;
            t0 = timer_us();
            if (OQS_SIG_sign(sig, sigbuf, &siglen, msg, msg_len, sk) != OQS_SUCCESS) { free(msg); goto fail; }
            t1 = timer_us(); t_sign[i] = t1 - t0;
            t0 = timer_us();
            if (OQS_SIG_verify(sig, msg, msg_len, sigbuf, siglen, pk) != OQS_SUCCESS) { free(msg); goto fail; }
            t1 = timer_us(); t_verify[i] = t1 - t0;
        }

        out->points[s].msg_size = msg_len;
        stats_compute(t_sign,   BENCH_ITERATIONS, &out->points[s].sign);
        stats_compute(t_verify, BENCH_ITERATIONS, &out->points[s].verify);

        if (msg_len < 1024)
            printf("  [%2d/%d] msg=%6zu B  -> sign: %9.2f us  verify: %9.2f us\n",
                   s+1, SCALE_MSG_SIZES_COUNT, msg_len,
                   out->points[s].sign.mean_us, out->points[s].verify.mean_us);
        else
            printf("  [%2d/%d] msg=%6zu KB -> sign: %9.2f us  verify: %9.2f us\n",
                   s+1, SCALE_MSG_SIZES_COUNT, msg_len/1024,
                   out->points[s].sign.mean_us, out->points[s].verify.mean_us);
        free(msg);
    }

    free(t_sign); free(t_verify);
    free(pk); free(sigbuf); OQS_MEM_secure_free(sk, sig->length_secret_key); OQS_SIG_free(sig);
    return BENCH_OK;
fail:
    free(t_sign); free(t_verify);
    free(pk); free(sigbuf); if (sk) OQS_MEM_secure_free(sk, sig->length_secret_key); OQS_SIG_free(sig);
    return BENCH_ERROR;
}

void sig_scale_print(const sig_scale_result_t *r) {
    printf("\n  Escalabilidade: %s (Nivel %s)\n", r->nist_name, r->nist_level);
    printf("  %-10s | %-24s | %-24s\n", "Msg (B)", "Sign us (media +/-dp)", "Verify us (media +/-dp)");
    printf("  ----------+-------------------------+-------------------------\n");
    for (int i = 0; i < SCALE_MSG_SIZES_COUNT; i++) {
        const sig_scale_point_t *p = &r->points[i];
        printf("  %-10zu | %9.2f +/- %9.2f | %9.2f +/- %9.2f\n",
               p->msg_size, p->sign.mean_us, p->sign.stddev_us,
               p->verify.mean_us, p->verify.stddev_us);
    }
}

void sig_scale_save_csv(FILE *fp, const sig_scale_result_t *r) {
    if (!fp) return;
    for (int i = 0; i < SCALE_MSG_SIZES_COUNT; i++) {
        const sig_scale_point_t *p = &r->points[i];
        fprintf(fp, "%s,%s,%zu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
                r->nist_name, r->nist_level, p->msg_size,
                p->sign.mean_us,   p->sign.stddev_us,   p->sign.min_us,   p->sign.median_us,
                p->verify.mean_us, p->verify.stddev_us, p->verify.min_us, p->verify.median_us);
    }
}
