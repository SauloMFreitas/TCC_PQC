#!/usr/bin/env bash
# =============================================================
# build_and_run.sh
# Compila a liboqs (se necessário), compila o benchmark e executa.
# =============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIBOQS_SRC="$SCRIPT_DIR/../third_party/liboqs"
LIBOQS_BUILD="$LIBOQS_SRC/build"
LIBOQS_INSTALL="$SCRIPT_DIR/../third_party/liboqs_install"
BENCH_BUILD="$SCRIPT_DIR/build"

# ---- 1. Compilar e instalar liboqs (pula se já existir) ----
if [ ! -f "$LIBOQS_INSTALL/lib/liboqs.a" ] && \
   [ ! -f "$LIBOQS_INSTALL/lib64/liboqs.a" ]; then

    echo ">>> Compilando liboqs..."
    mkdir -p "$LIBOQS_BUILD"
    cmake -S "$LIBOQS_SRC" -B "$LIBOQS_BUILD" \
        -DCMAKE_INSTALL_PREFIX="$LIBOQS_INSTALL" \
        -DBUILD_SHARED_LIBS=OFF \
        -DOQS_BUILD_ONLY_LIB=ON \
        -DCMAKE_BUILD_TYPE=Release

    cmake --build "$LIBOQS_BUILD" --parallel "$(nproc)"
    cmake --install "$LIBOQS_BUILD"
    echo ">>> liboqs instalada em: $LIBOQS_INSTALL"
else
    echo ">>> liboqs já compilada em: $LIBOQS_INSTALL"
fi

# ---- 2. Compilar o benchmark ----
echo ""
echo ">>> Compilando memory_benchmark (sem otimizações -O0)..."
mkdir -p "$BENCH_BUILD"
cmake -S "$SCRIPT_DIR" -B "$BENCH_BUILD" \
    -DCMAKE_PREFIX_PATH="$LIBOQS_INSTALL" \
    -DCMAKE_BUILD_TYPE=Debug

cmake --build "$BENCH_BUILD" --parallel "$(nproc)"

# ---- 3. Executar ----
echo ""
echo ">>> Executando benchmark..."
echo ""
cd "$BENCH_BUILD"
./memory_benchmark

# ---- 4. Copiar CSV de volta para a pasta benchmark ----
if [ -f "$BENCH_BUILD/memory_results.csv" ]; then
    mkdir -p "$SCRIPT_DIR/../results/memory/O0_debug"
    cp "$BENCH_BUILD/memory_results.csv" "$SCRIPT_DIR/../results/memory/O0_debug/memory_results.csv"
    echo ""
    echo ">>> CSV copiado para: results/memory/O0_debug/memory_results.csv"
fi
