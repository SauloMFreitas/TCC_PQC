#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

/*
 * utils.h — general utilities for benchmarking: memory usage, CSV output, terminal formatting
 */

/* ----------------------------------------------------------------
 * global benchmark settings
 * ---------------------------------------------------------------- */
#define BENCH_ITERATIONS   200   /* medições por operação — conforme TCC (Seção 3.2) */
#define BENCH_WARMUP        10   /* iterações de aquecimento                          */
#define MSG_SIZE            32   /* tamanho da mensagem de teste (B)  */

#define RESULTS_DIR        "results"
#define KEM_CSV_PATH       "results/kem_results.csv"
#define SIG_CSV_PATH       "results/sig_results.csv"

/* ----------------------------------------------------------------
 * estimated heap memory usage
 * @return memory usage in KB (kilobytes)
 * ---------------------------------------------------------------- */
double utils_memory_kb(void);

/* ----------------------------------------------------------------
 * generate CSV header
 * ---------------------------------------------------------------- */
void utils_write_kem_csv_header(FILE *fp);
void utils_write_sig_csv_header(FILE *fp);

/* ----------------------------------------------------------------
 * print visual section header and divider line in terminal
 * ---------------------------------------------------------------- */
void utils_print_section(const char *title);
void utils_print_divider(void);

/* ----------------------------------------------------------------
 * test message used in sign/verify benchmarks
 * - fixed size (MSG_SIZE) for consistência entre algoritmos 
 * ---------------------------------------------------------------- */
extern const uint8_t BENCH_MSG[MSG_SIZE];

#endif /* UTILS_H */