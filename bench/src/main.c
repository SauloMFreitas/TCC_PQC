#include "general_include.h"
#include "kem/kyber_benchmark.h"
#include "kem/hqc_benchmark.h"
#include "sig/dilithium_benchmark.h"
#include "sig/falcon_benchmark.h"
#include "sig/sphincs_benchmark.h"

#include <string.h>
#include <libgen.h>   /* basename() */

#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MKDIR(p) mkdir((p), 0755)
#endif

/* ----------------------------------------------------------------
 * Extrai o sufixo de otimizacao a partir do nome do executavel.
 *
 * Exemplos:
 *   ./full_bench-O2            -> "O2"
 *   ./full_bench-O3_native     -> "O3_native"
 *   ./full_bench-O3_native_lto -> "O3_native_lto"
 *
 * Retorna ponteiro para dentro do buffer estatico interno.
 * Se o prefixo "full_bench-" nao for encontrado, retorna "unknown".
 * ---------------------------------------------------------------- */
static const char *extract_suffix(const char *argv0) {
    static char buf[256];
    strncpy(buf, argv0, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *base = basename(buf);
    const char *prefix = "full_bench-";
    char *pos = strstr(base, prefix);
    if (pos) return pos + strlen(prefix);
    return "unknown";
}

/* ----------------------------------------------------------------
 * Garante que um diretorio existe, criando-o se necessario.
 * Cria tambem o diretorio pai (results/) se nao existir.
 * ---------------------------------------------------------------- */
static void ensure_dir(const char *path) {
    MKDIR(RESULTS_DIR);   /* garante que results/ existe */
    MKDIR(path);          /* cria results/<suffix>/      */
}

/* ----------------------------------------------------------------
 * Monta o caminho do CSV dentro da pasta do sufixo.
 * Ex: build_csv_path(buf, sizeof(buf), "O2_native", "kem")
 *  -> "results/O2_native/kem.csv"
 * ---------------------------------------------------------------- */
static void build_csv_path(char *out, size_t size,
                           const char *suffix,
                           const char *name)
{
    snprintf(out, size, "%s/%s/%s.csv", RESULTS_DIR, suffix, name);
}

static void print_banner(const char *suffix) {
    printf("\n  Benchmark Comparativo - Algoritmos Finalistas NIST PQC\n");
    printf("  liboqs       : %s\n", OQS_VERSION_TEXT);
    printf("  Timer        : %s\n", TIMER_PLATFORM);
    printf("  Otimizacao   : %s\n", suffix);
    printf("  Iteracoes    : %d  | Warmup: %d  | Msg: %d bytes\n",
           BENCH_ITERATIONS, BENCH_WARMUP, MSG_SIZE);
    printf("  ---------------------------------------------------\n\n");
}

int main(int argc, char *argv[]) {

    /* Sufixo extraido do nome do proprio executavel */
    const char *suffix = extract_suffix(argv[0]);

    print_banner(suffix);

    /* Cria results/ e results/<suffix>/ */
    char result_dir[256];
    snprintf(result_dir, sizeof(result_dir), "%s/%s", RESULTS_DIR, suffix);
    ensure_dir(result_dir);

    const char *filter = (argc > 1) ? argv[1] : "all";

    int run_mlkem = 0, run_hqc = 0;
    int run_mldsa = 0, run_falcon = 0, run_sphincs = 0;
    int scale_mldsa = 0, scale_falcon = 0, scale_sphincs = 0;

    if      (!strcmp(filter, "all"))           { run_mlkem=run_hqc=run_mldsa=run_falcon=run_sphincs=scale_mldsa=scale_falcon=scale_sphincs=1; }
    else if (!strcmp(filter, "kem"))           { run_mlkem=run_hqc=1; }
    else if (!strcmp(filter, "sig"))           { run_mldsa=run_falcon=run_sphincs=1; }
    else if (!strcmp(filter, "mlkem"))         { run_mlkem=1; }
    else if (!strcmp(filter, "hqc"))           { run_hqc=1; }
    else if (!strcmp(filter, "mldsa"))         { run_mldsa=1; }
    else if (!strcmp(filter, "falcon"))        { run_falcon=1; }
    else if (!strcmp(filter, "sphincs"))       { run_sphincs=1; }
    else if (!strcmp(filter, "scale"))         { run_mldsa=run_falcon=run_sphincs=1;
                                                 scale_mldsa=scale_falcon=scale_sphincs=1; }
    else if (!strcmp(filter, "scale_mldsa"))   { run_mldsa=1;   scale_mldsa=1; }
    else if (!strcmp(filter, "scale_falcon"))  { run_falcon=1;  scale_falcon=1; }
    else if (!strcmp(filter, "scale_sphincs")) { run_sphincs=1; scale_sphincs=1; }
    else {
        fprintf(stderr,
            "Uso: %s [modo]\n"
            "  all / kem / sig / mlkem / hqc / mldsa / falcon / sphincs\n"
            "  scale / scale_mldsa / scale_falcon / scale_sphincs\n",
            argv[0]);
        return EXIT_FAILURE;
    }

    /* Monta os caminhos: results/<suffix>/kem.csv etc. */
    char kem_path[256], sig_path[256], scale_path[256];
    build_csv_path(kem_path,   sizeof(kem_path),   suffix, "kem");
    build_csv_path(sig_path,   sizeof(sig_path),   suffix, "sig");
    build_csv_path(scale_path, sizeof(scale_path), suffix, "sig_scale");

    FILE *kem_csv = NULL, *sig_csv = NULL, *sig_scale_csv = NULL;

    if (run_mlkem || run_hqc) {
        kem_csv = fopen(kem_path, "w");
        if (kem_csv) { utils_write_kem_csv_header(kem_csv);
                       printf("[INFO] KEM     -> %s\n", kem_path); }
    }
    if (run_mldsa || run_falcon || run_sphincs) {
        sig_csv = fopen(sig_path, "w");
        if (sig_csv) { utils_write_sig_csv_header(sig_csv);
                       printf("[INFO] SIG     -> %s\n", sig_path); }
    }
    if (scale_mldsa || scale_falcon || scale_sphincs) {
        sig_scale_csv = fopen(scale_path, "w");
        if (sig_scale_csv) { utils_write_sig_scale_csv_header(sig_scale_csv);
                             printf("[INFO] SCALE   -> %s\n", scale_path); }
    }

    printf("\n");

    if (run_mlkem)   kyber_benchmark_run_all(kem_csv);
    if (run_hqc)     hqc_benchmark_run_all(kem_csv);
    if (run_mldsa)   dilithium_benchmark_run_all(sig_csv, scale_mldsa  ? sig_scale_csv : NULL);
    if (run_falcon)  falcon_benchmark_run_all   (sig_csv, scale_falcon  ? sig_scale_csv : NULL);
    if (run_sphincs) sphincs_benchmark_run_all  (sig_csv, scale_sphincs ? sig_scale_csv : NULL);

    if (kem_csv)       fclose(kem_csv);
    if (sig_csv)       fclose(sig_csv);
    if (sig_scale_csv) fclose(sig_scale_csv);

    printf("\n  Benchmark concluido. Resultados em: %s/\n\n", result_dir);
    return EXIT_SUCCESS;
}
