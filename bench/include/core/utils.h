#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#define BENCH_ITERATIONS    200   /* measured iterations per operation (paper, Sec. 3) */
#define BENCH_WARMUP        10    /* warm-up iterations, not measured */
#define MSG_SIZE            32

#define RESULTS_DIR          "results"
#define KEM_CSV_PATH         "results/kem_results.csv"
#define SIG_CSV_PATH         "results/sig_results.csv"
#define SIG_SCALE_CSV_PATH   "results/sig_scale_results.csv"

#define SCALE_MSG_SIZES_COUNT  21 
extern const size_t SCALE_MSG_SIZES[SCALE_MSG_SIZES_COUNT];

double utils_memory_kb(void);
void utils_write_kem_csv_header(FILE *fp);
void utils_write_sig_csv_header(FILE *fp);
void utils_write_sig_scale_csv_header(FILE *fp);
void utils_print_section(const char *title);
void utils_print_divider(void);

extern const uint8_t BENCH_MSG[MSG_SIZE];

#endif /* UTILS_H */
