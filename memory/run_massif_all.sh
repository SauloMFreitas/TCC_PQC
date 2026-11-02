#!/usr/bin/env bash
# =============================================================
# run_massif_all.sh
#
# Roda valgrind --tool=massif para cada algoritmo PQC e operação,
# salvando os arquivos .massif em massif_out/.
#
# Pré-requisitos:
#   - valgrind instalado  (sudo apt install valgrind)
#   - massif_single compilado no diretório atual (build/) ou PATH
#
# Como usar:
#   cd memory/build
#   cmake .. -DCMAKE_PREFIX_PATH=../../third_party/liboqs_install
#   make massif_single
#   bash ../run_massif_all.sh
#
# Resultado:
#   massif_out/<ALG>_<OP>.massif   — arquivo raw do massif
#   massif_results_heap.csv        — CSV com pico de heap por alg/op
# =============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$(pwd)"             # deve ser chamado de dentro do build/
BINARY="$BUILD_DIR/massif_single"
OUT_DIR="$BUILD_DIR/massif_out"
CSV_OUT="$BUILD_DIR/massif_results_heap.csv"
PARSE_SCRIPT="$SCRIPT_DIR/parse_massif.py"

# ---- Verifica pré-requisitos ----
if [ ! -f "$BINARY" ]; then
    echo "ERRO: binário 'massif_single' não encontrado em $BUILD_DIR"
    echo "  Compile com: cmake .. && make massif_single"
    exit 1
fi

if ! command -v valgrind &>/dev/null; then
    echo "ERRO: valgrind não encontrado."
    echo "  Instale com: sudo apt install valgrind"
    exit 1
fi

mkdir -p "$OUT_DIR"

# ---- Tabela de algoritmos e operações ----
#
# Formato: "ALG_NAME  OPERAÇÃO"
# Separados por tabulação ou espaços.
# Os nomes devem ser exatamente os que o liboqs usa.
#
declare -a ENTRIES=(
    # ----------- KEMs -----------
    "ML-KEM-512   keygen"
    "ML-KEM-512   encap"
    "ML-KEM-512   decap"
    "ML-KEM-768   keygen"
    "ML-KEM-768   encap"
    "ML-KEM-768   decap"
    "ML-KEM-1024  keygen"
    "ML-KEM-1024  encap"
    "ML-KEM-1024  decap"
    "HQC-128      keygen"
    "HQC-128      encap"
    "HQC-128      decap"
    "HQC-192      keygen"
    "HQC-192      encap"
    "HQC-192      decap"
    "HQC-256      keygen"
    "HQC-256      encap"
    "HQC-256      decap"
    # -------- Assinaturas --------
    # ML-DSA (FIPS 204)
    "ML-DSA-44  keygen"
    "ML-DSA-44  sign"
    "ML-DSA-44  verify"
    "ML-DSA-65  keygen"
    "ML-DSA-65  sign"
    "ML-DSA-65  verify"
    "ML-DSA-87  keygen"
    "ML-DSA-87  sign"
    "ML-DSA-87  verify"
    # FN-DSA (FIPS 206) — liboqs: "Falcon-512" / "Falcon-1024"
    "Falcon-512   keygen"
    "Falcon-512   sign"
    "Falcon-512   verify"
    "Falcon-1024  keygen"
    "Falcon-1024  sign"
    "Falcon-1024  verify"
    # SLH-DSA-SHAKE (FIPS 205) — liboqs: "SPHINCS+-SHAKE-*-simple"
    "SPHINCS+-SHAKE-128s-simple  keygen"
    "SPHINCS+-SHAKE-128s-simple  sign"
    "SPHINCS+-SHAKE-128s-simple  verify"
    "SPHINCS+-SHAKE-128f-simple  keygen"
    "SPHINCS+-SHAKE-128f-simple  sign"
    "SPHINCS+-SHAKE-128f-simple  verify"
    "SPHINCS+-SHAKE-192s-simple  keygen"
    "SPHINCS+-SHAKE-192s-simple  sign"
    "SPHINCS+-SHAKE-192s-simple  verify"
    "SPHINCS+-SHAKE-192f-simple  keygen"
    "SPHINCS+-SHAKE-192f-simple  sign"
    "SPHINCS+-SHAKE-192f-simple  verify"
    "SPHINCS+-SHAKE-256s-simple  keygen"
    "SPHINCS+-SHAKE-256s-simple  sign"
    "SPHINCS+-SHAKE-256s-simple  verify"
    "SPHINCS+-SHAKE-256f-simple  keygen"
    "SPHINCS+-SHAKE-256f-simple  sign"
    "SPHINCS+-SHAKE-256f-simple  verify"
)

TOTAL=${#ENTRIES[@]}
CURRENT=0
FAILED=0

echo "============================================================"
echo "  Valgrind massif — ${TOTAL} combinações algoritmo×operação"
echo "  Binário : $BINARY"
echo "  Saída   : $OUT_DIR"
echo "============================================================"
echo ""

for ENTRY in "${ENTRIES[@]}"; do
    # Separa algoritmo e operação (campos separados por espaço)
    read -r ALG OP <<< "$ENTRY"
    CURRENT=$((CURRENT + 1))

    # Nome de arquivo seguro (sem espaços ou caracteres especiais)
    SAFE_NAME="${ALG//[^a-zA-Z0-9_-]/_}_${OP}"
    MASSIF_FILE="$OUT_DIR/${SAFE_NAME}.massif"

    printf "[%2d/%d] %-42s %-8s  " "$CURRENT" "$TOTAL" "$ALG" "$OP"

    # Roda o valgrind
    # --pages-as-heap=yes  → inclui TODA memória do processo (heap + mmap)
    # --heap=yes           → rastreia heap (padrão, explícito)
    # --massif-out-file    → caminho do arquivo de saída
    # --threshold=0.1      → inclui alocações > 0,1% do pico
    # --detailed-freq=1    → snapshot detalhado a cada N snapshots normais
    if valgrind \
        --tool=massif \
        --heap=yes \
        --pages-as-heap=no \
        --massif-out-file="$MASSIF_FILE" \
        --threshold=0.1 \
        --detailed-freq=10 \
        "$BINARY" "$ALG" "$OP" \
        2>/dev/null; then
        echo "OK"
    else
        EXIT_CODE=$?
        if [ "$EXIT_CODE" -eq 2 ]; then
            echo "PULADO (algoritmo não disponível)"
            rm -f "$MASSIF_FILE"
        else
            echo "FALHA (exit=$EXIT_CODE)"
            FAILED=$((FAILED + 1))
        fi
    fi
done

echo ""
echo "------------------------------------------------------------"
echo "  Concluído: $CURRENT total  |  $FAILED falhas"
echo "------------------------------------------------------------"
echo ""

# ---- Gera CSV usando o parser Python ----
if command -v python3 &>/dev/null && [ -f "$PARSE_SCRIPT" ]; then
    echo "Gerando CSV de resultados..."
    python3 "$PARSE_SCRIPT" "$OUT_DIR" "$CSV_OUT"
    echo "CSV salvo em: $CSV_OUT"
else
    echo "AVISO: python3 ou parse_massif.py não encontrado."
    echo "       Execute manualmente:"
    echo "       python3 $PARSE_SCRIPT $OUT_DIR $CSV_OUT"
fi

echo ""
echo "Para visualizar um algoritmo individualmente:"
echo "  ms_print $OUT_DIR/ML-KEM-512_decap.massif | head -40"
echo ""
