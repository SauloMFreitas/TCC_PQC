/**
 * @file massif_single.c
 * @brief Executor de operação única para perfilamento de heap com Valgrind massif.
 *
 * @details
 * Este programa executa **exatamente uma chamada** de **uma operação** de
 * **um algoritmo** PQC por invocação.  Seu propósito exclusivo é ser
 * instrumentado pelo Valgrind massif, que captura snapshots do heap ao longo
 * do tempo de execução e reporta o pico de alocação.
 *
 * A ausência de laços de repetição é intencional: o massif mede o perfil de
 * heap de uma única operação isolada, sem distorção causada por iterações
 * consecutivas ou aquecimento de cache de alocador.
 *
 * @section uso Uso
 * @code
 *   valgrind --tool=massif --pages-as-heap=no \
 *            --massif-out-file=massif.<alg>.<op>.out \
 *            ./massif_single <alg_name> <operacao>
 * @endcode
 *
 * @section exemplos Exemplos de linha de comando
 * @code
 *   ./massif_single ML-KEM-512       keygen
 *   ./massif_single ML-KEM-512       encap
 *   ./massif_single ML-KEM-512       decap
 *   ./massif_single ML-DSA-44        sign
 *   ./massif_single FN-DSA-512       verify   # aceita nome NIST
 *   ./massif_single Falcon-512       verify   # aceita nome liboqs
 *   ./massif_single SLH-DSA-SHAKE-128s keygen
 * @endcode
 *
 * @section nomes Mapeamento de nomes NIST → liboqs
 * O liboqs ainda utiliza nomes pré-padronização para FN-DSA e SLH-DSA-SHAKE.
 * Este programa aceita tanto os nomes FIPS/NIST quanto os nomes internos do
 * liboqs, convertendo-os automaticamente via resolve_alg_name().
 *
 * | Nome NIST (TCC)        | Nome liboqs                      |
 * |------------------------|----------------------------------|
 * | FN-DSA-512             | Falcon-512                       |
 * | FN-DSA-1024            | Falcon-1024                      |
 * | SLH-DSA-SHAKE-128s     | SPHINCS+-SHAKE-128s-simple       |
 * | SLH-DSA-SHAKE-128f     | SPHINCS+-SHAKE-128f-simple       |
 * | SLH-DSA-SHAKE-192s     | SPHINCS+-SHAKE-192s-simple       |
 * | SLH-DSA-SHAKE-192f     | SPHINCS+-SHAKE-192f-simple       |
 * | SLH-DSA-SHAKE-256s     | SPHINCS+-SHAKE-256s-simple       |
 * | SLH-DSA-SHAKE-256f     | SPHINCS+-SHAKE-256f-simple       |
 * | ML-KEM-*, ML-DSA-*, HQC-* | (inalterado — liboqs já usa esses nomes) |
 *
 * @section codigos Códigos de saída
 * | Código | Significado                                      |
 * |--------|--------------------------------------------------|
 * |   0    | Sucesso                                          |
 * |   1    | Argumentos inválidos (número ou operação)        |
 * |   2    | Algoritmo não disponível nesta build do liboqs   |
 * |   3    | Falha criptográfica (OQS_STATUS != OQS_SUCCESS)  |
 *
 * @section compilacao Compilação
 * Via CMakeLists.txt (target recomendado):
 * @code
 *   cmake --build build --target massif_single
 * @endcode
 * Ou manualmente (desabilitar otimizações preserva informação de debug para massif):
 * @code
 *   gcc -O0 -g massif_single.c -o massif_single -loqs
 * @endcode
 *
 * @note Compilar com -O0 é importante para que o Valgrind massif produza
 *       rastreamentos de chamada (call stacks) legíveis e precisos.
 *
 * @author Saulo Moraes Zanon Figueiredo
 * @date   2025
 * @see    memory_benchmark.c  — medição complementar de stack e heap via mallinfo2
 * @see    run_massif.sh       — script que invoca este programa para todos os algoritmos
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oqs/oqs.h>

/* =========================================================================
 * Funções auxiliares
 * ========================================================================= */

/**
 * @brief Termina o programa com uma mensagem de erro na stderr.
 *
 * @param msg  Mensagem descritiva do erro.  Será prefixada por "ERRO: ".
 *
 * @note Não retorna (chama exit(1)).
 */
static void die(const char *msg) {
    fprintf(stderr, "ERRO: %s\n", msg);
    exit(1);
}

/**
 * @brief Converte nomes NIST/FIPS para os nomes internos aceitos pelo liboqs.
 *
 * @details
 * O liboqs 0.10.x ainda utiliza nomes pré-padronização para os algoritmos
 * de assinatura FN-DSA e SLH-DSA-SHAKE.  Esta função traduz os nomes
 * oficiais do NIST para os equivalentes esperados pelas funções
 * OQS_SIG_new() e OQS_KEM_new().
 *
 * Algoritmos cujos nomes já coincidem entre NIST e liboqs (ML-KEM-*, ML-DSA-*,
 * HQC-*) são retornados sem modificação.
 *
 * @param name  Nome do algoritmo, podendo ser o nome NIST ou o nome liboqs.
 * @return      Ponteiro para string literal com o nome aceito pelo liboqs.
 *              Retorna @p name inalterado se não houver mapeamento.
 *
 * @note O ponteiro retornado aponta para uma string literal estática;
 *       não deve ser liberado com free().
 */
static const char *resolve_alg_name(const char *name) {
    /* FN-DSA (FIPS 206) → Falcon (nome liboqs) */
    if (strcmp(name, "FN-DSA-512")  == 0) return "Falcon-512";
    if (strcmp(name, "FN-DSA-1024") == 0) return "Falcon-1024";

    /* SLH-DSA-SHAKE (FIPS 205, variantes SHAKE) → SPHINCS+-SHAKE-*-simple */
    if (strcmp(name, "SLH-DSA-SHAKE-128s") == 0) return "SPHINCS+-SHAKE-128s-simple";
    if (strcmp(name, "SLH-DSA-SHAKE-128f") == 0) return "SPHINCS+-SHAKE-128f-simple";
    if (strcmp(name, "SLH-DSA-SHAKE-192s") == 0) return "SPHINCS+-SHAKE-192s-simple";
    if (strcmp(name, "SLH-DSA-SHAKE-192f") == 0) return "SPHINCS+-SHAKE-192f-simple";
    if (strcmp(name, "SLH-DSA-SHAKE-256s") == 0) return "SPHINCS+-SHAKE-256s-simple";
    if (strcmp(name, "SLH-DSA-SHAKE-256f") == 0) return "SPHINCS+-SHAKE-256f-simple";

    /* Já está no formato liboqs — passa direto (ML-KEM, ML-DSA, HQC, etc.) */
    return name;
}

/**
 * @brief Determina se um algoritmo (pelo nome liboqs) é um KEM ou uma SIG.
 *
 * @details
 * A detecção é feita por correspondência de prefixo com os nomes conhecidos
 * de KEMs suportados pelo liboqs.  Qualquer algoritmo que não corresponda
 * a nenhum prefixo KEM é tratado como algoritmo de assinatura (SIG).
 *
 * @param alg  Nome do algoritmo no formato liboqs (após resolve_alg_name).
 * @return     1 se KEM, 0 se SIG.
 */
static int is_kem(const char *alg) {
    /* Prefixos de KEMs reconhecidos pelo liboqs */
    return (strncmp(alg, "ML-KEM",   6) == 0 ||
            strncmp(alg, "Kyber",    5) == 0 ||
            strncmp(alg, "HQC",      3) == 0 ||
            strncmp(alg, "BIKE",     4) == 0 ||
            strncmp(alg, "FrodoKEM", 8) == 0 ||
            strncmp(alg, "Classic",  7) == 0);
}

/* =========================================================================
 * Execução de KEM
 * ========================================================================= */

/**
 * @brief Executa uma única operação de um KEM e encerra o programa.
 *
 * @details
 * Inicializa o KEM, aloca todos os buffers necessários e executa a operação
 * especificada exatamente uma vez.  O fluxo de cada operação é:
 *
 * - **keygen**: gera um par de chaves (pk, sk).
 * - **encap**: gera um par de chaves, depois encapsula uma chave compartilhada
 *              produzindo o ciphertext (ct) e o segredo (ss_enc).
 * - **decap**: gera par de chaves, encapsula, depois desencapsula o ciphertext
 *              com a chave secreta, recuperando o segredo (ss_dec).
 *
 * As operações acumulativas (encap e decap realizam keygen antes) são
 * necessárias para fornecer entradas válidas à operação de interesse,
 * pois o massif captura o pico de heap durante toda a execução.
 *
 * @param alg_name  Nome do algoritmo no formato liboqs (ex: "ML-KEM-512").
 * @param op        Operação a executar: "keygen", "encap" ou "decap".
 *
 * @note Encerra com exit(2) se o algoritmo não estiver disponível na build.
 * @note Encerra com exit(3) se a operação criptográfica falhar.
 * @note Encerra com exit(1) se a operação for desconhecida.
 */
static void run_kem(const char *alg_name, const char *op) {
    OQS_KEM *kem = OQS_KEM_new(alg_name);
    if (!kem) {
        fprintf(stderr, "AVISO: KEM '%s' não disponível nesta build.\n", alg_name);
        exit(2);
    }

    /* Aloca todos os buffers necessários para as três operações */
    uint8_t *pk     = malloc(kem->length_public_key);
    uint8_t *sk     = malloc(kem->length_secret_key);
    uint8_t *ct     = malloc(kem->length_ciphertext);
    uint8_t *ss_enc = malloc(kem->length_shared_secret);
    uint8_t *ss_dec = malloc(kem->length_shared_secret);

    if (!pk || !sk || !ct || !ss_enc || !ss_dec)
        die("malloc falhou para buffers do KEM");

    OQS_STATUS rc = OQS_SUCCESS;

    if (strcmp(op, "keygen") == 0) {

        rc = OQS_KEM_keypair(kem, pk, sk);

    } else if (strcmp(op, "encap") == 0) {

        /* Encap requer uma chave pública válida → gera o par antes */
        OQS_KEM_keypair(kem, pk, sk);
        rc = OQS_KEM_encaps(kem, ct, ss_enc, pk);

    } else if (strcmp(op, "decap") == 0) {

        /* Decap requer chave secreta e ciphertext válidos */
        OQS_KEM_keypair(kem, pk, sk);
        OQS_KEM_encaps(kem, ct, ss_enc, pk);
        rc = OQS_KEM_decaps(kem, ss_dec, ct, sk);

    } else {
        fprintf(stderr, "Operação desconhecida para KEM: '%s'\n"
                        "Use: keygen | encap | decap\n", op);
        exit(1);
    }

    if (rc != OQS_SUCCESS) {
        fprintf(stderr, "FALHA em %s %s (rc=%d)\n", alg_name, op, rc);
        exit(3);
    }

    /*
     * Libera os buffers e o objeto KEM.
     * O Valgrind massif captura o pico ANTES desta liberação,
     * pois os snapshots são tirados durante a execução da operação.
     */
    free(pk); free(sk); free(ct); free(ss_enc); free(ss_dec);
    OQS_KEM_free(kem);
}

/* =========================================================================
 * Execução de SIG (assinatura digital)
 * ========================================================================= */

/**
 * @brief Executa uma única operação de um algoritmo de assinatura e encerra.
 *
 * @details
 * Inicializa o algoritmo de assinatura, aloca os buffers necessários e
 * executa a operação especificada exatamente uma vez.  O fluxo de cada
 * operação é:
 *
 * - **keygen**: gera um par de chaves (pk, sk).
 * - **sign**: gera um par de chaves, depois assina uma mensagem de teste
 *             fixa, produzindo a assinatura (signature).
 * - **verify**: gera par de chaves, assina, depois verifica a assinatura
 *               com a chave pública.
 *
 * A mensagem de teste é uma constante estática, pois o conteúdo da mensagem
 * não influencia o consumo de memória dos algoritmos avaliados.
 *
 * @param alg_name  Nome do algoritmo no formato liboqs
 *                  (ex: "Falcon-512", "SPHINCS+-SHAKE-128s-simple").
 * @param op        Operação a executar: "keygen", "sign" ou "verify".
 *
 * @note Encerra com exit(2) se o algoritmo não estiver disponível na build.
 * @note Encerra com exit(3) se a operação criptográfica falhar.
 * @note Encerra com exit(1) se a operação for desconhecida.
 */
static void run_sig(const char *alg_name, const char *op) {
    OQS_SIG *sig = OQS_SIG_new(alg_name);
    if (!sig) {
        fprintf(stderr, "AVISO: SIG '%s' não disponível nesta build.\n", alg_name);
        exit(2);
    }

    /* Mensagem de teste estática — conteúdo não afeta consumo de memória */
    static const uint8_t MSG[] = "Mensagem de teste para benchmark PQC - TCC";
    const size_t MSG_LEN = sizeof(MSG) - 1;

    uint8_t *pk        = malloc(sig->length_public_key);
    uint8_t *sk        = malloc(sig->length_secret_key);
    uint8_t *signature = malloc(sig->length_signature);
    size_t   sig_len   = 0;  /* Preenchido por OQS_SIG_sign() */

    if (!pk || !sk || !signature)
        die("malloc falhou para buffers da SIG");

    OQS_STATUS rc = OQS_SUCCESS;

    if (strcmp(op, "keygen") == 0) {

        rc = OQS_SIG_keypair(sig, pk, sk);

    } else if (strcmp(op, "sign") == 0) {

        /* Sign requer chave secreta válida → gera o par antes */
        OQS_SIG_keypair(sig, pk, sk);
        rc = OQS_SIG_sign(sig, signature, &sig_len, MSG, MSG_LEN, sk);

    } else if (strcmp(op, "verify") == 0) {

        /* Verify requer assinatura e chave pública válidas */
        OQS_SIG_keypair(sig, pk, sk);
        OQS_SIG_sign(sig, signature, &sig_len, MSG, MSG_LEN, sk);
        rc = OQS_SIG_verify(sig, MSG, MSG_LEN, signature, sig_len, pk);

    } else {
        fprintf(stderr, "Operação desconhecida para SIG: '%s'\n"
                        "Use: keygen | sign | verify\n", op);
        exit(1);
    }

    if (rc != OQS_SUCCESS) {
        fprintf(stderr, "FALHA em %s %s (rc=%d)\n", alg_name, op, rc);
        exit(3);
    }

    free(pk); free(sk); free(signature);
    OQS_SIG_free(sig);
}

/* =========================================================================
 * Ponto de entrada
 * ========================================================================= */

/**
 * @brief Ponto de entrada: valida argumentos, resolve o nome do algoritmo
 *        e despacha para run_kem() ou run_sig().
 *
 * @details
 * Fluxo principal:
 * 1. Valida que exatamente dois argumentos foram fornecidos.
 * 2. Converte o nome do algoritmo do padrão NIST para o padrão liboqs
 *    via resolve_alg_name().
 * 3. Detecta se é KEM ou SIG via is_kem().
 * 4. Chama run_kem() ou run_sig() conforme o tipo.
 *
 * @param argc  Número de argumentos (deve ser 3: programa + alg + op).
 * @param argv  Vetor de argumentos:
 *                argv[1] = nome do algoritmo (NIST ou liboqs)
 *                argv[2] = operação ("keygen", "encap"/"decap" ou "sign"/"verify")
 *
 * @return  0 em caso de sucesso; 1 se os argumentos forem inválidos.
 *          (run_kem/run_sig podem encerrar com códigos 2 ou 3.)
 */
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr,
            "Uso: %s <alg_name> <operacao>\n"
            "Exemplos:\n"
            "  %s ML-KEM-512        keygen\n"
            "  %s ML-DSA-44         sign\n"
            "  %s FN-DSA-512        verify\n"
            "  %s SLH-DSA-SHAKE-128s keygen\n",
            argv[0], argv[0], argv[0], argv[0], argv[0]);
        return 1;
    }

    /*
     * Resolve o nome: aceita tanto o nome NIST/FIPS (ex: "FN-DSA-512")
     * quanto o nome interno do liboqs (ex: "Falcon-512").
     */
    const char *alg_name = resolve_alg_name(argv[1]);
    const char *op       = argv[2];

    if (is_kem(alg_name))
        run_kem(alg_name, op);
    else
        run_sig(alg_name, op);

    return 0;
}
