#ifndef GENERAL_INCLUDE_H
#define GENERAL_INCLUDE_H

/*
 * general_include.h — Include central do projeto
 *
 * Todo arquivo .c de benchmark deve incluir apenas este header.
 * Ele reúne a liboqs, os módulos core e tipos comuns.
 */

/* ----------------------------------------------------------------
 * Biblioteca padrão C
 * ---------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

/* ----------------------------------------------------------------
 * liboqs — Open Quantum Safe
 * Padronizações NIST 2024:
 *   FIPS 203 → ML-KEM  (antes: CRYSTALS-Kyber)
 *   FIPS 204 → ML-DSA  (antes: CRYSTALS-Dilithium)
 *   FIPS 205 → SLH-DSA (antes: SPHINCS+)
 *   FIPS 206 → FN-DSA  (antes: Falcon)
 *   4ª Rodada→ HQC     (ainda não padronizado)
 * ---------------------------------------------------------------- */
#include <oqs/oqs.h>

/* ----------------------------------------------------------------
 * Módulos core do projeto
 * ---------------------------------------------------------------- */
#include "core/timer.h"
#include "core/stats.h"
#include "core/utils.h"

/* ----------------------------------------------------------------
 * Tipo de retorno unificado para todas as funções de benchmark
 * ---------------------------------------------------------------- */
typedef enum {
    BENCH_OK    =  0,
    BENCH_ERROR = -1
} bench_status_t;

/* ----------------------------------------------------------------
 * Resultado de KEM (ML-KEM / HQC)
 * ---------------------------------------------------------------- */
typedef struct {
    /* Identificação */
    char nist_name[64];    /* nome padronizado NIST  (ex: "ML-KEM-768") */
    char oqs_name[64];     /* nome interno liboqs    (ex: "Kyber768")   */
    char nist_level[8];    /* nível de segurança     (ex: "3")          */

    /* Tamanhos */
    size_t pk_bytes;       /* chave pública                             */
    size_t sk_bytes;       /* chave secreta                             */
    size_t ct_bytes;       /* ciphertext                                */
    size_t ss_bytes;       /* shared secret                             */

    /* Tempos */
    stats_t keygen;
    stats_t encaps;
    stats_t decaps;

    /* Memória */
    double memory_kb;
} kem_result_t;

/* ----------------------------------------------------------------
 * Resultado de Assinatura (ML-DSA / SLH-DSA / FN-DSA)
 * ---------------------------------------------------------------- */
typedef struct {
    /* Identificação */
    char nist_name[64];
    char oqs_name[64];
    char nist_level[8];

    /* Tamanhos */
    size_t pk_bytes;
    size_t sk_bytes;
    size_t sig_max_bytes;   /* tamanho máximo declarado pela lib       */
    size_t sig_real_bytes;  /* tamanho real medido após assinatura     */

    /* Tempos */
    stats_t keygen;
    stats_t sign;
    stats_t verify;

    /* Memória */
    double memory_kb;
} sig_result_t;

#endif /* GENERAL_INCLUDE_H */