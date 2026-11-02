#!/usr/bin/env bash
set -euo pipefail

# ==========================================================
#  PQC Benchmark — full pipeline: install, build and run
#  Target: Linux Mint 22.x / Ubuntu 24.04 (bare metal)
#
#  Usage: scripts/run_all.sh [all|timing|kem|memory]
#    all     timing (scale + kem) + memory          (default)
#    timing  16 timing executables, modes scale + kem
#    kem     16 timing executables, mode kem only
#    memory  stack (O0 debug + 16 flags) and heap (massif)
# ==========================================================

MODE="${1:-all}"
case "$MODE" in all|timing|kem|memory) ;; *)
  echo "usage: $0 [all|timing|kem|memory]" >&2; exit 2 ;;
esac

# ---- Caminhos do projeto (relativos ao repositório) ----
PROJ="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LIBOQS_SRC="$PROJ/third_party/liboqs"
LIBOQS_BUILD="$LIBOQS_SRC/build"
LIBOQS_INSTALL="$PROJ/third_party/liboqs_install"
BENCH_BUILD="$PROJ/bench/build"
MEM_DIR="$PROJ/memory"
RESULTS="$PROJ/results"
LIBOQS_TAG="0.10.1"

# ---- Núcleo isolado para os benchmarks de tempo ----
PIN_CORE=2

# ---- Log ----
mkdir -p "$RESULTS/logs"
LOG="$RESULTS/logs/run_all_$(date +%Y%m%d_%H%M%S).log"
exec > >(tee -a "$LOG") 2>&1

log()  { printf '\n\033[1;36m==> %s\033[0m\n' "$*"; }
warn() { printf '\033[1;33m[aviso] %s\033[0m\n' "$*"; }
die()  { printf '\033[1;31m[erro] %s\033[0m\n' "$*" >&2; exit 1; }

# ==========================================================
#  Restauração do estado da CPU ao sair (trap)
# ==========================================================
ORIG_GOV=""; ORIG_TURBO_AMD=""; ORIG_TURBO_INTEL=""; ORIG_ASLR=""; ORIG_SMT=""

restore_state() {
  log "Restaurando estado da CPU"
  [ -n "$ORIG_GOV" ] && sudo cpupower frequency-set -g "$ORIG_GOV" >/dev/null 2>&1 || true
  if [ -n "$ORIG_TURBO_AMD" ] && [ -f /sys/devices/system/cpu/cpufreq/boost ]; then
    echo "$ORIG_TURBO_AMD" | sudo tee /sys/devices/system/cpu/cpufreq/boost >/dev/null || true
  fi
  if [ -n "$ORIG_TURBO_INTEL" ] && [ -f /sys/devices/system/cpu/intel_pstate/no_turbo ]; then
    echo "$ORIG_TURBO_INTEL" | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo >/dev/null || true
  fi
  [ -n "$ORIG_ASLR" ] && echo "$ORIG_ASLR" | sudo tee /proc/sys/kernel/randomize_va_space >/dev/null || true
  if [ -n "$ORIG_SMT" ] && [ -f /sys/devices/system/cpu/smt/control ]; then
    echo "$ORIG_SMT" | sudo tee /sys/devices/system/cpu/smt/control >/dev/null || true
  fi
}
trap restore_state EXIT

# ==========================================================
#  1) Dependências
# ==========================================================
install_deps() {
  log "Instalando dependências (apt)"
  sudo apt update
  sudo apt install -y \
    build-essential cmake ninja-build libssl-dev git valgrind python3 \
    linux-tools-common linux-tools-generic
  sudo apt install -y "linux-tools-$(uname -r)" 2>/dev/null \
    || warn "linux-tools-$(uname -r) indisponível; cpupower pode reclamar de versão"
}

# ==========================================================
#  2) liboqs (pula se já instalada + build presente)
# ==========================================================
build_liboqs() {
  local have_install="" have_build=""
  { [ -f "$LIBOQS_INSTALL/lib/liboqs.so" ] || [ -f "$LIBOQS_INSTALL/lib64/liboqs.so" ] \
    || [ -f "$LIBOQS_INSTALL/lib/liboqs.a" ]; } && have_install=1
  [ -d "$LIBOQS_BUILD" ] && have_build=1

  if [ -n "$have_install" ] && [ -n "$have_build" ]; then
    log "liboqs já instalada em $LIBOQS_INSTALL — pulando build"
    return 0
  fi

  log "Compilando e instalando liboqs $LIBOQS_TAG"
  if [ ! -e "$LIBOQS_SRC/CMakeLists.txt" ]; then
    git -C "$PROJ" submodule update --init third_party/liboqs 2>/dev/null \
      || git clone --depth 1 --branch "$LIBOQS_TAG" \
           https://github.com/open-quantum-safe/liboqs.git "$LIBOQS_SRC"
  fi
  cd "$LIBOQS_SRC"
  cmake -B build -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_INSTALL_PREFIX="$LIBOQS_INSTALL" \
    -DOQS_ENABLE_KEM_HQC=ON \
    -DOQS_ENABLE_KEM_ML_KEM=ON \
    -DOQS_ENABLE_SIG_ML_DSA=ON \
    -DOQS_ENABLE_SIG_SLH_DSA=ON \
    -DOQS_ENABLE_SIG_FALCON=ON \
    -DOPENSSL_ROOT_DIR=/usr \
    -DOPENSSL_CRYPTO_LIBRARY=/usr/lib/x86_64-linux-gnu/libcrypto.so \
    -DOPENSSL_SSL_LIBRARY=/usr/lib/x86_64-linux-gnu/libssl.so
  cmake --build build -j"$(nproc)"
  cmake --install build
}

# ==========================================================
#  3) Higiene de benchmark (governor / turbo / ASLR / SMT)
# ==========================================================
apply_hygiene() {
  log "Aplicando higiene de benchmark (requer sudo)"

  if command -v cpupower >/dev/null 2>&1; then
    ORIG_GOV="$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo '')"
    sudo cpupower frequency-set -g performance >/dev/null 2>&1 \
      && echo "  governor -> performance (era: ${ORIG_GOV:-?})" \
      || warn "não consegui fixar o governor"
  else
    warn "cpupower ausente; pulei o governor"
  fi

  if [ -f /sys/devices/system/cpu/cpufreq/boost ]; then            # AMD
    ORIG_TURBO_AMD="$(cat /sys/devices/system/cpu/cpufreq/boost)"
    echo 0 | sudo tee /sys/devices/system/cpu/cpufreq/boost >/dev/null \
      && echo "  turbo boost (AMD) -> off"
  elif [ -f /sys/devices/system/cpu/intel_pstate/no_turbo ]; then  # Intel
    ORIG_TURBO_INTEL="$(cat /sys/devices/system/cpu/intel_pstate/no_turbo)"
    echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo >/dev/null \
      && echo "  turbo boost (Intel) -> off"
  else
    warn "controle de turbo não encontrado"
  fi

  ORIG_ASLR="$(cat /proc/sys/kernel/randomize_va_space 2>/dev/null || echo '')"
  echo 0 | sudo tee /proc/sys/kernel/randomize_va_space >/dev/null \
    && echo "  ASLR -> off (era: ${ORIG_ASLR:-?})"

  if [ -f /sys/devices/system/cpu/smt/control ]; then
    local smt_now; smt_now="$(cat /sys/devices/system/cpu/smt/control)"
    if [ "$smt_now" = "on" ]; then
      ORIG_SMT="on"
      echo off | sudo tee /sys/devices/system/cpu/smt/control >/dev/null \
        && echo "  SMT -> off (era: on)"
    else
      warn "SMT em '$smt_now'; não alterei"
    fi
  fi
}

# ==========================================================
#  4) Benchmark de tempo (16 execs, modo scale)
# ==========================================================
run_timing() {
  log "Compilando benchmark de tempo (16 configurações)"
  mkdir -p "$BENCH_BUILD"; cd "$BENCH_BUILD"
  cmake .. -DOQS_DIR="$LIBOQS_BUILD"
  cmake --build . -j"$(nproc)"

  log "Rodando os 16 executáveis (modo scale, fixados no core $PIN_CORE)"
  local execs=(
    full_bench-O0 full_bench-O0_native full_bench-O0_lto full_bench-O0_native_lto
    full_bench-O1 full_bench-O1_native full_bench-O1_lto full_bench-O1_native_lto
    full_bench-O2 full_bench-O2_native full_bench-O2_lto full_bench-O2_native_lto
    full_bench-O3 full_bench-O3_native full_bench-O3_lto full_bench-O3_native_lto
  )
  for e in "${execs[@]}"; do
    [ -x "./$e" ] || { warn "executável ausente: $e (pulando)"; continue; }
    echo "====== $e ======"
    [ "$MODE" = kem ] || taskset -c "$PIN_CORE" "./$e" scale
    taskset -c "$PIN_CORE" "./$e" kem
  done

  log "Copiando CSVs de tempo para $RESULTS/timing/"
  mkdir -p "$RESULTS/timing"
  cp -r "$BENCH_BUILD/results/." "$RESULTS/timing/"
}

# ==========================================================
#  5) Benchmark de memória (stack + heap/massif)
# ==========================================================
run_memory() {
  log "Memória — stack, build de referência -O0 -g (build_and_run.sh)"
  bash "$MEM_DIR/build_and_run.sh"

  log "Memória — stack + heap (massif) nas 16 configurações de flags"
  bash "$MEM_DIR/run_memory_all_flags.sh"
}

# ==========================================================
#  Fluxo principal
# ==========================================================
main() {
  log "Início — projeto em $PROJ | modo: $MODE | log: $LOG"
  [ -d "$PROJ/bench" ] || die "não encontrei $PROJ/bench — confira o caminho do projeto"

  sudo -v || die "sudo necessário"
  ( while true; do sudo -n true; sleep 50; kill -0 "$$" 2>/dev/null || exit; done ) &

  install_deps
  build_liboqs
  apply_hygiene
  [ "$MODE" = memory ] || run_timing
  case "$MODE" in all|memory) run_memory ;; esac

  log "Agregando resultados (*_all_flags.csv)"
  python3 "$PROJ/scripts/aggregate_results.py"

  log "Concluído."
  log "Tempo:   $RESULTS/timing/"
  log "Memória: $RESULTS/memory/"
  log "Figuras: python3 scripts/plot_results.py"
}
main "$@" 
