#include <stdio.h>
#include <string.h>
#include "core/utils.h"

#ifdef __GLIBC__
#include <malloc.h>
#endif

const uint8_t BENCH_MSG[MSG_SIZE] = {
    0x54,0x43,0x43,0x20,0x50,0x51,0x43,0x20,
    0x42,0x65,0x6e,0x63,0x68,0x6d,0x61,0x72,
    0x6b,0x20,0x32,0x30,0x32,0x34,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

const size_t SCALE_MSG_SIZES[SCALE_MSG_SIZES_COUNT] = {
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512,
    1024, 2048, 4096, 8192, 16384, 32768,
    65536, 131072, 262144, 524288, 1048576
};

double utils_memory_kb(void) {
#ifdef __GLIBC__
    struct mallinfo2 mi = mallinfo2();
    return (double)mi.uordblks / 1024.0;
#else
    return 0.0;
#endif
}

void utils_write_kem_csv_header(FILE *fp) {
    if (!fp) return;
    fprintf(fp,
        "algoritmo_nist,algoritmo_oqs,nivel_nist,"
        "pk_bytes,sk_bytes,ct_bytes,ss_bytes,"
        "keygen_mean_us,keygen_stddev_us,keygen_min_us,keygen_median_us,"
        "encaps_mean_us,encaps_stddev_us,encaps_min_us,encaps_median_us,"
        "decaps_mean_us,decaps_stddev_us,decaps_min_us,decaps_median_us,"
        "memoria_kb\n");
}

void utils_write_sig_csv_header(FILE *fp) {
    if (!fp) return;
    fprintf(fp,
        "algoritmo_nist,algoritmo_oqs,nivel_nist,"
        "pk_bytes,sk_bytes,sig_max_bytes,sig_real_bytes,"
        "keygen_mean_us,keygen_stddev_us,keygen_min_us,keygen_median_us,"
        "sign_mean_us,sign_stddev_us,sign_min_us,sign_median_us,"
        "verify_mean_us,verify_stddev_us,verify_min_us,verify_median_us,"
        "memoria_kb\n");
}

void utils_write_sig_scale_csv_header(FILE *fp) {
    if (!fp) return;
    fprintf(fp,
        "algoritmo_nist,nivel_nist,msg_bytes,"
        "sign_mean_us,sign_stddev_us,sign_min_us,sign_median_us,"
        "verify_mean_us,verify_stddev_us,verify_min_us,verify_median_us\n");
}

void utils_print_section(const char *title) {
    int len = (int)strlen(title);
    int pad = (54 - len) / 2;
    printf("\n+------------------------------------------------------+\n");
    printf("|%*s%s%*s|\n", pad, "", title, 54 - pad - len, "");
    printf("+------------------------------------------------------+\n");
}

void utils_print_divider(void) {
    printf("  ----------------------------------------------------\n");
}
