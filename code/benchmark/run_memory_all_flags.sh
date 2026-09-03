#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIBOQS_INSTALL="$SCRIPT_DIR/../libs/liboqs_install"
OUT_ROOT="$SCRIPT_DIR/memory_results_flags"
RUN_MASSIF="${RUN_MASSIF:-1}"   # RUN_MASSIF=0 pula o massif (só stack)

# As 16 combinações: nome -> flags
declare -A CONFIGS=(
  [O0]="-O0"
  [O0_native]="-O0 -march=native"
  [O0_lto]="-O0 -flto"
  [O0_native_lto]="-O0 -march=native -flto"
  [O1]="-O1"
  [O1_native]="-O1 -march=native"
  [O1_lto]="-O1 -flto"
  [O1_native_lto]="-O1 -march=native -flto"
  [O2]="-O2"
  [O2_native]="-O2 -march=native"
  [O2_lto]="-O2 -flto"
  [O2_native_lto]="-O2 -march=native -flto"
  [O3]="-O3"
  [O3_native]="-O3 -march=native"
  [O3_lto]="-O3 -flto"
  [O3_native_lto]="-O3 -march=native -flto"
)

# Ordem determinística
ORDER=(O0 O0_native O0_lto O0_native_lto
       O1 O1_native O1_lto O1_native_lto
       O2 O2_native O2_lto O2_native_lto
       O3 O3_native O3_lto O3_native_lto)

mkdir -p "$OUT_ROOT"
echo ">>> Resultados de memória por flag em: $OUT_ROOT"
echo ">>> massif: $([ "$RUN_MASSIF" = 1 ] && echo ATIVADO || echo desativado)"

for cfg in "${ORDER[@]}"; do
  flags="${CONFIGS[$cfg]}"
  build_dir="$SCRIPT_DIR/build_mem_$cfg"
  out_dir="$OUT_ROOT/$cfg"
  mkdir -p "$out_dir"

  echo ""
  echo "==================================================="
  echo ">>> [$cfg]  flags: $flags"
  echo "==================================================="

  # Recompila o harness com estas flags.
  # -DEXTRA_MEM_FLAGS é anexado por cima do -O0 base do CMakeLists;
  # como vem depois, o -Ox daqui prevalece.
  cmake -S "$SCRIPT_DIR" -B "$build_dir" \
      -DCMAKE_PREFIX_PATH="$LIBOQS_INSTALL" \
      -DCMAKE_BUILD_TYPE="" \
      -DCMAKE_C_FLAGS="$flags -g -Wall" >/dev/null
  cmake --build "$build_dir" --parallel "$(nproc)" >/dev/null

  # Stack/heap/RSS
  ( cd "$build_dir" && ./memory_benchmark )
  [ -f "$build_dir/memory_results.csv" ] && \
      cp "$build_dir/memory_results.csv" "$out_dir/memory_results.csv"

  # Massif (heap), se habilitado e se o runner existir
  if [ "$RUN_MASSIF" = 1 ] && [ -f "$SCRIPT_DIR/run_massif_all.sh" ]; then
    ( cd "$build_dir" && bash "$SCRIPT_DIR/run_massif_all.sh" )
    [ -f "$build_dir/massif_results_heap.csv" ] && \
        cp "$build_dir/massif_results_heap.csv" "$out_dir/massif_results_heap.csv"
  fi

  echo ">>> [$cfg] concluído -> $out_dir"
done

echo ""
echo ">>> Todas as 16 configurações concluídas."
echo ">>> Estrutura final:"
find "$OUT_ROOT" -name '*.csv' | sort
