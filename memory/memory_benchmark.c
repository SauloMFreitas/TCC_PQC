/**
 * @file memory_benchmark.c
 * @brief Medição de consumo de memória (heap e stack) para algoritmos PQC via liboqs.
 *
 * @details
 * Este programa integra o benchmark experimental do TCC
 * "Análise Experimental Comparativa de Algoritmos de Criptografia Pós-Quântica
 * do NIST: Desempenho Temporal, Otimização de Compilação e Escalabilidade".
 *
 * São avaliados os seguintes algoritmos, organizados por família NIST/FIPS:
 *
 *   KEMs (FIPS 203 / sem padrão FIPS):
 *     - ML-KEM-512, ML-KEM-768, ML-KEM-1024  (FIPS 203)
 *     - HQC-128, HQC-192, HQC-256
 *
 *   Assinaturas Digitais:
 *     - ML-DSA-44, ML-DSA-65, ML-DSA-87      (FIPS 204)
 *     - FN-DSA-512, FN-DSA-1024              (FIPS 206 — liboqs: Falcon-512/1024)
 *     - SLH-DSA-SHAKE-128s/f, 192s/f, 256s/f (FIPS 205 — liboqs: SPHINCS+-SHAKE-*-simple)
 *
 * ---------------------------------------------------------------------------
 * TÉCNICAS DE MEDIÇÃO
 * ---------------------------------------------------------------------------
 *
 * 1. STACK — "Stack Painting":
 *    Uma thread POSIX é criada com um buffer de THREAD_STACK_SIZE bytes alocado
 *    no heap e apontado como stack via pthread_attr_setstack(). Antes da criação
 *    da thread, o buffer é inteiramente preenchido com o byte sentinela 0xAB.
 *    Após o retorno da operação criptográfica, o buffer é varrido do endereço mais
 *    baixo para o mais alto. O primeiro byte diferente de 0xAB marca o início da
 *    região de stack efetivamente usada, e a diferença até o topo do buffer é o
 *    consumo real de stack da operação.
 *
 *    Fundamento: o stack de uma thread em arquiteturas x86/x86-64 cresce para
 *    endereços menores (descending stack). Logo, a região não tocada fica nos
 *    endereços mais baixos do buffer (ainda com 0xAB), e a região usada fica nos
 *    endereços mais altos.
 *
 * 2. HEAP — mallinfo2() (delta líquido):
 *    O campo uordblks da estrutura mallinfo2 reporta o total de bytes de heap
 *    alocados E AINDA NÃO LIBERADOS no momento da leitura. O delta antes/depois
 *    da operação indica se alguma alocação permaneceu viva após o retorno.
 *    Na prática, o liboqs libera todas as alocações transientes dentro de cada
 *    chamada, resultando em delta zero para todos os algoritmos avaliados.
 *    Para capturar o PICO de heap (alocações transientes durante a execução),
 *    use valgrind --tool=massif (ver massif_single.c).
 *
 * 3. RSS — /proc/self/status (VmRSS):
 *    O RSS (Resident Set Size) é lido do campo VmRSS antes e depois de cada
 *    operação. Como o buffer de stack já estava mapeado antes da operação
 *    começar (via pthread_attr_setstack), o RSS não se altera durante a execução,
 *    resultando em delta zero. Mantido para completude e futura extensão.
 *
 * ---------------------------------------------------------------------------
 * NOTA SOBRE NOMES DE ALGORITMOS
 * ---------------------------------------------------------------------------
 * A versão do liboqs utilizada neste trabalho ainda não adota os nomes FIPS
 * oficiais. O mapeamento usado internamente é:
 *
 *   Nome NIST/FIPS         Constante liboqs (OQS_SIG_alg_*)
 *   ─────────────────────  ────────────────────────────────────────────────
 *   FN-DSA-512             OQS_SIG_alg_falcon_512    → "Falcon-512"
 *   FN-DSA-1024            OQS_SIG_alg_falcon_1024   → "Falcon-1024"
 *   SLH-DSA-SHAKE-128s     OQS_SIG_alg_sphincs_shake_128s_simple
 *   SLH-DSA-SHAKE-128f     OQS_SIG_alg_sphincs_shake_128f_simple
 *   SLH-DSA-SHAKE-192s     OQS_SIG_alg_sphincs_shake_192s_simple
 *   SLH-DSA-SHAKE-192f     OQS_SIG_alg_sphincs_shake_192f_simple
 *   SLH-DSA-SHAKE-256s     OQS_SIG_alg_sphincs_shake_256s_simple
 *   SLH-DSA-SHAKE-256f     OQS_SIG_alg_sphincs_shake_256f_simple
 *
 * O CSV de saída (memory_results.csv) usa os nomes internos do liboqs.
 * O arquivo memory_results_tcc.csv (gerado manualmente) contém a coluna
 * "nome_liboqs" para o mapeamento e usa os nomes NIST como identificador.
 *
 * ---------------------------------------------------------------------------
 * COMPILAÇÃO
 * ---------------------------------------------------------------------------
 * Via CMake (recomendado):
 *   cd build && cmake .. && make memory_benchmark
 *
 * Manual:
 *   gcc -O0 -g memory_benchmark.c -o memory_benchmark \
 *       -I../third_party/liboqs_install/include \
 *       -L../third_party/liboqs_install/lib -loqs -lpthread
 *
 * @note Compilar com -O0 é importante para que o consumo de stack reflita
 *       o comportamento real do algoritmo sem otimizações do compilador.
 *
 * @author Saulo de Moura Zandona Freitas
 * @date   2025
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <malloc.h>       /* mallinfo2 */
#include <pthread.h>
#include <sys/resource.h> /* getrusage */
#include <oqs/oqs.h>

/* ============================================================
 * CONSTANTES DE CONFIGURAÇÃO
 * ============================================================ */

/** @brief Byte sentinela para pintura do stack.
 *  Valor 0xAB escolhido por ser improvável como conteúdo legítimo de stack
 *  e fácil de identificar visualmente em dumps de memória. */
#define STACK_SENTINEL      ((uint8_t)0xAB)

/** @brief Tamanho do buffer de stack da thread de medição (4 MB).
 *  Todos os algoritmos PQC avaliados neste trabalho consomem bem menos que isso
 *  (máximo observado: HQC-256 decap ~ 181 KB). Aumente apenas se ocorrer
 *  segfault durante a medição de um algoritmo não avaliado neste estudo. */
#define THREAD_STACK_SIZE   (4 * 1024 * 1024)

/** @brief Número de execuções de aquecimento (warm-up) antes da medição.
 *  Necessário para estabilizar caches do processador e estado interno do liboqs. */
#define WARMUP_RUNS         2

/** @brief Número de execuções medidas para estatística (reservado para extensão futura). */
#define BENCH_RUNS          5

/** @brief Nome do arquivo CSV de saída com os resultados brutos. */
#define OUTPUT_CSV          "memory_results.csv"

/** @brief Capacidade máxima do vetor de resultados. */
#define MAX_RESULTS         256


/* ============================================================
 * ESTRUTURAS DE DADOS
 * ============================================================ */

/**
 * @brief Resultado completo de memória para uma única operação.
 *
 * Agrupa as três métricas coletadas: delta de heap via mallinfo2,
 * uso de stack via stack painting e variação de RSS via /proc/self/status.
 */
typedef struct {
    long   heap_delta_bytes; /**< Variação líquida de heap (mallinfo2.uordblks) em bytes.
                              *   Zero indica que nenhuma alocação permaneceu após o retorno. */
    size_t stack_bytes;      /**< Bytes de stack efetivamente consumidos (stack painting). */
    long   rss_before_kb;   /**< RSS do processo antes da operação (KB, via VmRSS). */
    long   rss_after_kb;    /**< RSS do processo após a operação (KB, via VmRSS). */
    long   rss_delta_kb;    /**< Variação de RSS: rss_after - rss_before (KB). */
} MemStats;

/**
 * @brief Resultado associado a um algoritmo e operação específicos.
 *
 * Armazena o nome do algoritmo (string do liboqs), o nome da operação
 * ("keygen", "encap"/"sign", "decap"/"verify"), as métricas medidas
 * e um flag de sucesso da operação criptográfica.
 */
typedef struct {
    const char *alg_name;  /**< Nome do algoritmo conforme a API liboqs. */
    const char *operation; /**< Nome da operação medida.                  */
    MemStats    stats;     /**< Métricas coletadas nesta execução.        */
    int         success;   /**< 1 se a operação retornou OQS_SUCCESS.     */
} BenchResult;


/* ============================================================
 * LEITURA DE MÉTRICAS DO SISTEMA
 * ============================================================ */

/**
 * @brief Retorna o total de bytes de heap alocados e não liberados.
 *
 * Usa mallinfo2(), disponível no glibc 2.33+. O campo uordblks contém
 * o total de bytes em uso no heap gerenciado pelo alocador do processo.
 * Alocações transientes (feitas e liberadas dentro de uma função) NÃO
 * aparecem no delta antes/depois — apenas alocações persistentes.
 *
 * @return Bytes de heap em uso no momento da chamada.
 */
static size_t get_heap_usage(void) {
    struct mallinfo2 mi = mallinfo2();
    return (size_t)mi.uordblks;
}

/**
 * @brief Retorna o RSS (Resident Set Size) do processo em kilobytes.
 *
 * Lê o campo VmRSS do arquivo /proc/self/status, que reporta quantos
 * KB de memória física estão mapeados para o processo no momento.
 *
 * @return RSS em KB, ou -1 em caso de erro de leitura.
 */
static long get_rss_kb(void) {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return -1;

    char line[256];
    long rss = -1;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line + 6, "%ld", &rss);
            break;
        }
    }
    fclose(f);
    return rss;
}


/* ============================================================
 * STACK PAINTING VIA PTHREAD
 * ============================================================ */

/**
 * @brief Contexto passado à thread de medição de stack.
 *
 * Contém ponteiro para a função a medir, seu argumento, o buffer de
 * stack pintado e o campo de resultado preenchido pela thread.
 */
typedef struct {
    void     (*func)(void *); /**< Função criptográfica a executar.           */
    void      *arg;            /**< Argumento passado à função.                */
    uint8_t   *stack_buf;      /**< Buffer de stack pintado com STACK_SENTINEL. */
    size_t     stack_size;     /**< Tamanho do buffer em bytes.                 */
    size_t     stack_used;     /**< SAÍDA: bytes de stack efetivamente usados.  */
} StackCtx;

/**
 * @brief Função executada pela thread de medição de stack.
 *
 * Executa a operação criptográfica e, em seguida, varre o buffer de stack
 * do endereço mais baixo para o mais alto, contando quantos bytes sentinela
 * foram sobrescritos. O stack em x86-64 cresce para endereços menores
 * (descending), portanto a região não utilizada fica nos endereços mais
 * baixos do buffer.
 *
 * @param arg Ponteiro para StackCtx preenchido pelo chamador.
 * @return NULL (convenção de pthread).
 */
static void *stack_thread_func(void *arg) {
    StackCtx *ctx = (StackCtx *)arg;

    /* Executa a operação criptográfica a ser medida */
    ctx->func(ctx->arg);

    /*
     * Varredura do buffer de baixo (menor endereço) para cima.
     *
     * Em x86-64, o stack cresce em direção a endereços menores.
     * Portanto:
     *   - Endereços BAIXOS do buffer → região não usada (ainda 0xAB)
     *   - Endereços ALTOS  do buffer → região usada pelo stack
     *
     * O índice `i` avança enquanto encontrar bytes sentinela.
     * A partir do primeiro byte ≠ 0xAB, começa a região usada.
     * stack_used = tamanho_total - i (bytes a partir do primeiro uso)
     */
    size_t i = 0;
    while (i < ctx->stack_size && ctx->stack_buf[i] == STACK_SENTINEL)
        i++;

    ctx->stack_used = (i < ctx->stack_size) ? (ctx->stack_size - i) : 0;
    return NULL;
}

/**
 * @brief Mede o consumo de stack de uma função via "stack painting".
 *
 * Aloca um buffer de THREAD_STACK_SIZE bytes, preenche-o com o byte
 * sentinela 0xAB e cria uma thread POSIX usando esse buffer como stack
 * (pthread_attr_setstack). Após o retorno da thread, conta quantos bytes
 * foram sobrescritos pelo stack, obtendo o consumo real da operação.
 *
 * @param func  Ponteiro para a função a medir (assinatura: void f(void *)).
 * @param arg   Argumento passado à função.
 * @return      Bytes de stack consumidos, ou 0 em caso de erro de alocação.
 */
static size_t measure_stack(void (*func)(void *), void *arg) {
    /* Aloca o buffer que servirá de stack para a thread */
    uint8_t *buf = (uint8_t *)malloc(THREAD_STACK_SIZE);
    if (!buf) {
        perror("malloc (stack buffer)");
        return 0;
    }

    /* Pinta o buffer inteiro com o byte sentinela */
    memset(buf, STACK_SENTINEL, THREAD_STACK_SIZE);

    StackCtx ctx = {
        .func       = func,
        .arg        = arg,
        .stack_buf  = buf,
        .stack_size = THREAD_STACK_SIZE,
        .stack_used = 0,
    };

    /* Configura os atributos da thread para usar nosso buffer como stack */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (pthread_attr_setstack(&attr, buf, THREAD_STACK_SIZE) != 0) {
        perror("pthread_attr_setstack");
        pthread_attr_destroy(&attr);
        free(buf);
        return 0;
    }

    /* Cria e aguarda a thread de medição */
    pthread_t tid;
    pthread_create(&tid, &attr, stack_thread_func, &ctx);
    pthread_join(tid, NULL);
    pthread_attr_destroy(&attr);

    size_t used = ctx.stack_used;
    free(buf);
    return used;
}


/* ============================================================
 * FUNÇÃO DE MEDIÇÃO UNIFICADA
 * ============================================================ */

/**
 * @brief Executa uma operação criptográfica medindo heap, stack e RSS.
 *
 * Realiza WARMUP_RUNS execuções de aquecimento sem medição para estabilizar
 * caches e estado interno do liboqs. Em seguida, executa a operação uma vez
 * para medir heap e RSS, e uma vez dentro de uma thread pintada para medir stack.
 *
 * @param func  Função wrapper da operação criptográfica.
 * @param arg   Contexto (KemCtx ou SigCtx) com buffers e objeto de algoritmo.
 * @return      MemStats preenchida com as três métricas.
 */
static MemStats measure_memory(void (*func)(void *), void *arg) {
    MemStats stats = {0};

    /* Aquecimento: estabiliza caches e estado interno do liboqs */
    for (int i = 0; i < WARMUP_RUNS; i++)
        func(arg);

    /* Medição de heap (mallinfo2) e RSS (/proc/self/status) */
    size_t h_before   = get_heap_usage();
    long   rss_before = get_rss_kb();

    func(arg); /* execução medida */

    size_t h_after   = get_heap_usage();
    long   rss_after = get_rss_kb();

    stats.heap_delta_bytes = (long)h_after - (long)h_before;
    stats.rss_before_kb   = rss_before;
    stats.rss_after_kb    = rss_after;
    stats.rss_delta_kb    = rss_after - rss_before;

    /* Medição de stack via thread com buffer pintado */
    stats.stack_bytes = measure_stack(func, arg);

    return stats;
}


/* ============================================================
 * CONTEXTOS E WRAPPERS PARA KEM
 * ============================================================ */

/**
 * @brief Contexto compartilhado entre as três operações de um KEM.
 *
 * Agrupa o objeto OQS_KEM e todos os buffers necessários para keygen,
 * encap e decap, além do código de retorno da última operação executada.
 */
typedef struct {
    OQS_KEM *kem;     /**< Objeto do KEM inicializado via OQS_KEM_new(). */
    uint8_t *pk;      /**< Chave pública (length_public_key bytes).      */
    uint8_t *sk;      /**< Chave secreta (length_secret_key bytes).      */
    uint8_t *ct;      /**< Ciphertext   (length_ciphertext bytes).       */
    uint8_t *ss_enc;  /**< Shared secret gerado no encapsulamento.       */
    uint8_t *ss_dec;  /**< Shared secret recuperado no decapsulamento.   */
    OQS_STATUS status;/**< Código de retorno da última operação.         */
} KemCtx;

/** @brief Wrapper de keygen do KEM para uso com measure_stack/measure_memory. */
static void kem_keygen_wrap(void *arg) {
    KemCtx *c = (KemCtx *)arg;
    c->status = OQS_KEM_keypair(c->kem, c->pk, c->sk);
}

/** @brief Wrapper de encapsulamento do KEM. Requer pk válida no contexto. */
static void kem_encap_wrap(void *arg) {
    KemCtx *c = (KemCtx *)arg;
    c->status = OQS_KEM_encaps(c->kem, c->ct, c->ss_enc, c->pk);
}

/** @brief Wrapper de decapsulamento do KEM. Requer ct e sk válidos no contexto. */
static void kem_decap_wrap(void *arg) {
    KemCtx *c = (KemCtx *)arg;
    c->status = OQS_KEM_decaps(c->kem, c->ss_dec, c->ct, c->sk);
}


/* ============================================================
 * CONTEXTOS E WRAPPERS PARA ASSINATURAS DIGITAIS
 * ============================================================ */

/**
 * @brief Contexto compartilhado entre as três operações de um esquema de assinatura.
 *
 * Agrupa o objeto OQS_SIG, buffers de chaves e assinatura, e a mensagem de teste.
 * A mensagem é fixa para todos os algoritmos, garantindo comparabilidade.
 */
typedef struct {
    OQS_SIG  *sig;        /**< Objeto do esquema inicializado via OQS_SIG_new(). */
    uint8_t  *pk;         /**< Chave pública (length_public_key bytes).          */
    uint8_t  *sk;         /**< Chave secreta (length_secret_key bytes).          */
    uint8_t  *signature;  /**< Buffer para a assinatura (length_signature bytes). */
    size_t    sig_len;    /**< Comprimento real da assinatura após sign().        */
    const uint8_t *msg;   /**< Ponteiro para a mensagem de teste.                */
    size_t    msg_len;    /**< Comprimento da mensagem em bytes.                 */
    OQS_STATUS status;    /**< Código de retorno da última operação.             */
} SigCtx;

/** @brief Wrapper de keygen do esquema de assinatura. */
static void sig_keygen_wrap(void *arg) {
    SigCtx *c = (SigCtx *)arg;
    c->status = OQS_SIG_keypair(c->sig, c->pk, c->sk);
}

/** @brief Wrapper de assinatura. Requer sk válida no contexto. */
static void sig_sign_wrap(void *arg) {
    SigCtx *c = (SigCtx *)arg;
    c->status = OQS_SIG_sign(c->sig, c->signature, &c->sig_len,
                              c->msg, c->msg_len, c->sk);
}

/** @brief Wrapper de verificação. Requer pk, assinatura e sig_len válidos. */
static void sig_verify_wrap(void *arg) {
    SigCtx *c = (SigCtx *)arg;
    c->status = OQS_SIG_verify(c->sig, c->msg, c->msg_len,
                                c->signature, c->sig_len, c->pk);
}


/* ============================================================
 * RUNNERS DE BENCHMARK
 * ============================================================ */

/**
 * @brief Executa e registra as medições de memória para um algoritmo KEM.
 *
 * Mede keygen, encap e decap em sequência. Para encap, garante a existência
 * de um par de chaves válido executando keygen antes da medição. Para decap,
 * garante ciphertext válido executando encap antes da medição. Essa preparação
 * NÃO é contabilizada — apenas a operação-alvo é medida.
 *
 * @param alg_name  String do algoritmo conforme a API liboqs (ex: "ML-KEM-512").
 * @param results   Vetor de BenchResult onde os resultados serão armazenados.
 * @param count     Ponteiro para o índice atual do vetor (incrementado aqui).
 */
static void benchmark_kem(const char *alg_name,
                           BenchResult results[], int *count) {
    printf("  KEM  : %s\n", alg_name);

    OQS_KEM *kem = OQS_KEM_new(alg_name);
    if (!kem) {
        printf("    [AVISO] Não disponível nesta build: %s\n", alg_name);
        return;
    }

    /* Aloca buffers para todos os artefatos do KEM */
    KemCtx ctx = { .kem = kem };
    ctx.pk     = (uint8_t *)malloc(kem->length_public_key);
    ctx.sk     = (uint8_t *)malloc(kem->length_secret_key);
    ctx.ct     = (uint8_t *)malloc(kem->length_ciphertext);
    ctx.ss_enc = (uint8_t *)malloc(kem->length_shared_secret);
    ctx.ss_dec = (uint8_t *)malloc(kem->length_shared_secret);

    /* keygen: nenhuma preparação necessária */
    {
        BenchResult *r = &results[(*count)++];
        r->alg_name  = alg_name;
        r->operation = "keygen";
        r->stats     = measure_memory(kem_keygen_wrap, &ctx);
        r->success   = (ctx.status == OQS_SUCCESS);
    }

    /* Gera par de chaves válido para encap (fora da medição) */
    OQS_KEM_keypair(kem, ctx.pk, ctx.sk);

    /* encap: opera sobre a chave pública gerada acima */
    {
        BenchResult *r = &results[(*count)++];
        r->alg_name  = alg_name;
        r->operation = "encap";
        r->stats     = measure_memory(kem_encap_wrap, &ctx);
        r->success   = (ctx.status == OQS_SUCCESS);
    }

    /* Gera ciphertext válido para decap (fora da medição) */
    OQS_KEM_encaps(kem, ctx.ct, ctx.ss_enc, ctx.pk);

    /* decap: opera sobre o ciphertext e a chave secreta */
    {
        BenchResult *r = &results[(*count)++];
        r->alg_name  = alg_name;
        r->operation = "decap";
        r->stats     = measure_memory(kem_decap_wrap, &ctx);
        r->success   = (ctx.status == OQS_SUCCESS);
    }

    printf("    pk=%zu B  sk=%zu B  ct=%zu B  ss=%zu B\n",
           kem->length_public_key, kem->length_secret_key,
           kem->length_ciphertext, kem->length_shared_secret);

    free(ctx.pk); free(ctx.sk); free(ctx.ct);
    free(ctx.ss_enc); free(ctx.ss_dec);
    OQS_KEM_free(kem);
}

/**
 * @brief Executa e registra as medições de memória para um esquema de assinatura.
 *
 * Mede keygen, sign e verify em sequência. Para sign, garante par de chaves válido.
 * Para verify, garante assinatura válida. Assim como no KEM, apenas a operação-alvo
 * é contabilizada — a preparação ocorre fora da janela de medição.
 *
 * A mensagem de teste é fixa ("Mensagem de teste para benchmark PQC - TCC 2025"),
 * garantindo que diferenças de consumo reflitam apenas o algoritmo, não a entrada.
 *
 * @param alg_name  String do algoritmo conforme a API liboqs.
 * @param results   Vetor de BenchResult onde os resultados serão armazenados.
 * @param count     Ponteiro para o índice atual do vetor (incrementado aqui).
 */
static void benchmark_sig(const char *alg_name,
                           BenchResult results[], int *count) {
    printf("  SIG  : %s\n", alg_name);

    OQS_SIG *sig = OQS_SIG_new(alg_name);
    if (!sig) {
        printf("    [AVISO] Não disponível nesta build: %s\n", alg_name);
        return;
    }

    /* Mensagem fixa para todos os algoritmos — garante comparabilidade */
    static const uint8_t MSG[] = "Mensagem de teste para benchmark PQC - TCC 2025";

    SigCtx ctx = {
        .sig     = sig,
        .msg     = MSG,
        .msg_len = sizeof(MSG) - 1, /* exclui o '\0' terminal */
    };
    ctx.pk        = (uint8_t *)malloc(sig->length_public_key);
    ctx.sk        = (uint8_t *)malloc(sig->length_secret_key);
    ctx.signature = (uint8_t *)malloc(sig->length_signature);

    /* keygen: nenhuma preparação necessária */
    {
        BenchResult *r = &results[(*count)++];
        r->alg_name  = alg_name;
        r->operation = "keygen";
        r->stats     = measure_memory(sig_keygen_wrap, &ctx);
        r->success   = (ctx.status == OQS_SUCCESS);
    }

    /* Gera par de chaves válido para sign/verify (fora da medição) */
    OQS_SIG_keypair(sig, ctx.pk, ctx.sk);

    /* sign: opera sobre a mensagem e a chave secreta */
    {
        BenchResult *r = &results[(*count)++];
        r->alg_name  = alg_name;
        r->operation = "sign";
        r->stats     = measure_memory(sig_sign_wrap, &ctx);
        r->success   = (ctx.status == OQS_SUCCESS);
    }

    /* Gera assinatura válida para verify (fora da medição) */
    OQS_SIG_sign(sig, ctx.signature, &ctx.sig_len,
                 ctx.msg, ctx.msg_len, ctx.sk);

    /* verify: confirma assinatura com a chave pública */
    {
        BenchResult *r = &results[(*count)++];
        r->alg_name  = alg_name;
        r->operation = "verify";
        r->stats     = measure_memory(sig_verify_wrap, &ctx);
        r->success   = (ctx.status == OQS_SUCCESS);
    }

    printf("    pk=%zu B  sk=%zu B  sig_max=%zu B\n",
           sig->length_public_key, sig->length_secret_key,
           sig->length_signature);

    free(ctx.pk); free(ctx.sk); free(ctx.signature);
    OQS_SIG_free(sig);
}


/* ============================================================
 * EXIBIÇÃO E EXPORTAÇÃO DOS RESULTADOS
 * ============================================================ */

/** @brief Imprime uma linha separadora na tabela de resultados no terminal. */
static void print_separator(void) {
    printf("%-40s %-8s %16s %14s %13s\n",
           "----------------------------------------",
           "--------",
           "----------------",
           "--------------",
           "-------------");
}

/**
 * @brief Exibe todos os resultados em formato tabular no terminal.
 *
 * Agrupa resultados por algoritmo (linha em branco entre algoritmos distintos)
 * e exibe as três métricas: delta de heap, stack e delta de RSS.
 *
 * @param results  Vetor de resultados preenchido pelos benchmarks.
 * @param count    Número de entradas válidas no vetor.
 */
static void print_table(BenchResult *results, int count) {
    printf("\n");
    printf("%-40s %-8s %16s %14s %13s\n",
           "Algoritmo", "Op",
           "Heap delta (B)", "Stack (bytes)", "RSS delta (KB)");
    print_separator();

    const char *last_alg = "";
    for (int i = 0; i < count; i++) {
        BenchResult *r = &results[i];
        /* Linha em branco ao trocar de algoritmo para legibilidade */
        if (strcmp(r->alg_name, last_alg) != 0) {
            printf("\n");
            last_alg = r->alg_name;
        }
        printf("%-40s %-8s %16ld %14zu %13ld\n",
               r->alg_name,
               r->operation,
               r->stats.heap_delta_bytes,
               r->stats.stack_bytes,
               r->stats.rss_delta_kb);
    }
    printf("\n");
    print_separator();
    printf("\n");
}

/**
 * @brief Exporta todos os resultados para o arquivo CSV definido em OUTPUT_CSV.
 *
 * Formato do CSV:
 *   algoritmo, operacao, heap_delta_bytes, stack_bytes,
 *   rss_before_kb, rss_after_kb, rss_delta_kb, sucesso
 *
 * Os nomes de algoritmo no CSV correspondem às strings internas do liboqs.
 * Para o mapeamento para nomes NIST, consulte memory_results_tcc.csv.
 *
 * @param results  Vetor de resultados preenchido pelos benchmarks.
 * @param count    Número de entradas válidas no vetor.
 */
static void save_csv(BenchResult *results, int count) {
    FILE *f = fopen(OUTPUT_CSV, "w");
    if (!f) { perror("fopen CSV"); return; }

    fprintf(f, "algoritmo,operacao,heap_delta_bytes,stack_bytes,"
               "rss_before_kb,rss_after_kb,rss_delta_kb,sucesso\n");

    for (int i = 0; i < count; i++) {
        BenchResult *r = &results[i];
        fprintf(f, "%s,%s,%ld,%zu,%ld,%ld,%ld,%d\n",
                r->alg_name,
                r->operation,
                r->stats.heap_delta_bytes,
                r->stats.stack_bytes,
                r->stats.rss_before_kb,
                r->stats.rss_after_kb,
                r->stats.rss_delta_kb,
                r->success);
    }

    fclose(f);
    printf("Resultados salvos em: %s\n\n", OUTPUT_CSV);
}


/* ============================================================
 * PONTO DE ENTRADA
 * ============================================================ */

/**
 * @brief Ponto de entrada do benchmark de memória.
 *
 * Executa os benchmarks de todos os algoritmos no escopo do TCC,
 * exibe os resultados no terminal e exporta para CSV.
 *
 * Ordem de execução:
 *   1. KEMs: ML-KEM-512/768/1024, HQC-128/192/256
 *   2. Assinaturas: ML-DSA-44/65/87, FN-DSA-512/1024, SLH-DSA-SHAKE-128s/f...256s/f
 *
 * @return 0 em caso de sucesso.
 */
int main(void) {
    printf("============================================================\n");
    printf("  Benchmark de Memória PQC — liboqs\n");
    printf("  Heap : mallinfo2() (delta líquido)\n");
    printf("  Stack: stack painting (thread POSIX com buffer sentinela)\n");
    printf("  RSS  : /proc/self/status (VmRSS)\n");
    printf("  Buffer de stack: %d MB | Warm-up: %d | Runs: %d\n",
           (int)(THREAD_STACK_SIZE / 1024 / 1024), WARMUP_RUNS, BENCH_RUNS);
    printf("============================================================\n\n");

    BenchResult results[MAX_RESULTS];
    int count = 0;

    /* ----------------------------------------------------------------
     * KEMs
     * ---------------------------------------------------------------- */
    printf("[ KEMs ]\n");

    /* ML-KEM (FIPS 203) — baseado em módulos de reticulados (MLWE) */
    benchmark_kem(OQS_KEM_alg_ml_kem_512,  results, &count);
    benchmark_kem(OQS_KEM_alg_ml_kem_768,  results, &count);
    benchmark_kem(OQS_KEM_alg_ml_kem_1024, results, &count);

    /* HQC — baseado em códigos corretores de erros (syndrome decoding) */
    benchmark_kem(OQS_KEM_alg_hqc_128, results, &count);
    benchmark_kem(OQS_KEM_alg_hqc_192, results, &count);
    benchmark_kem(OQS_KEM_alg_hqc_256, results, &count);

    /* ----------------------------------------------------------------
     * Assinaturas Digitais
     * ---------------------------------------------------------------- */
    printf("\n[ Assinaturas Digitais ]\n");

    /* ML-DSA (FIPS 204) — Dilithium, baseado em reticulados (MLWE/MSIS) */
    benchmark_sig(OQS_SIG_alg_ml_dsa_44, results, &count);
    benchmark_sig(OQS_SIG_alg_ml_dsa_65, results, &count);
    benchmark_sig(OQS_SIG_alg_ml_dsa_87, results, &count);

    /* FN-DSA (FIPS 206) — Falcon, baseado em NTRU sobre redes de reticulados.
     * ATENÇÃO: liboqs usa os nomes "Falcon-512" e "Falcon-1024" (nomes antigos).
     * As constantes OQS_SIG_alg_falcon_* mapeiam para esses nomes. */
    benchmark_sig(OQS_SIG_alg_falcon_512,  results, &count);
    benchmark_sig(OQS_SIG_alg_falcon_1024, results, &count);

    /* SLH-DSA-SHAKE (FIPS 205) — SPHINCS+, baseado em funções de hash (SHAKE-256).
     * ATENÇÃO: liboqs usa o nome "SPHINCS+-SHAKE-*-simple" (nome do candidato).
     * Variantes avaliadas: todas as 6 combinações de nível {128,192,256} × {s,f}.
     *   - Sufixo 's': assinaturas menores, sign mais lento
     *   - Sufixo 'f': assinaturas maiores, sign mais rápido */
    benchmark_sig(OQS_SIG_alg_sphincs_shake_128s_simple, results, &count);
    benchmark_sig(OQS_SIG_alg_sphincs_shake_128f_simple, results, &count);
    benchmark_sig(OQS_SIG_alg_sphincs_shake_192s_simple, results, &count);
    benchmark_sig(OQS_SIG_alg_sphincs_shake_192f_simple, results, &count);
    benchmark_sig(OQS_SIG_alg_sphincs_shake_256s_simple, results, &count);
    benchmark_sig(OQS_SIG_alg_sphincs_shake_256f_simple, results, &count);

    /* ----------------------------------------------------------------
     * Saída
     * ---------------------------------------------------------------- */
    print_table(results, count);
    save_csv(results, count);

    printf("NOTA: heap_delta_bytes = 0 para todos os algoritmos é esperado.\n");
    printf("      O liboqs libera todas as alocações dentro de cada chamada.\n");
    printf("      Para medir o PICO de heap durante a execução, use:\n");
    printf("        valgrind --tool=massif ./massif_single <alg> <op>\n\n");

    return 0;
}
