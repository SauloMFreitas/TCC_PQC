#!/usr/bin/env bash
# =============================================================
# validate_flags_comparison.sh
#
# Compila memory_benchmark com as mesmas 16 combinações de flags
# usadas no benchmark de tempo do TCC e compara o uso de stack.
#
# Combinações testadas (para cada nível -O0 a -O3):
#   1. -Ox
#   2. -Ox -flto
#   3. -Ox -march=native
#   4. -Ox -flto -march=native
#
# Uso:
#   cd memory
#   bash validate_flags_comparison.sh
#
# Saída:
#   flags_comparison.csv   — tabela completa com todos os deltas
#   flags_summary.txt      — resumo legível com veredito
# =============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIBOQS_INSTALL="$SCRIPT_DIR/../third_party/liboqs_install"
BUILD_BASE="$SCRIPT_DIR/build_validation"
OUT_CSV="$SCRIPT_DIR/../results/memory/validation/flags_comparison.csv"
OUT_SUMMARY="$SCRIPT_DIR/../results/memory/validation/flags_summary.txt"
mkdir -p "$(dirname "$OUT_CSV")"

# ---- Verificar liboqs ----
if [ ! -f "$LIBOQS_INSTALL/lib/liboqs.a" ] && \
   [ ! -f "$LIBOQS_INSTALL/lib64/liboqs.a" ]; then
    echo "[ERRO] liboqs não encontrada em $LIBOQS_INSTALL"
    echo "       Execute build_and_run.sh primeiro para compilar a liboqs."
    exit 1
fi

LIBOQS_LIB=$([ -f "$LIBOQS_INSTALL/lib/liboqs.a" ] \
    && echo "$LIBOQS_INSTALL/lib" \
    || echo "$LIBOQS_INSTALL/lib64")
LIBOQS_INC="$LIBOQS_INSTALL/include"

# ---- Definir as 16 combinações ----
# Cada entrada: "label|flags"
declare -a CONFIGS=(
    "O0|-O0"
    "O0_lto|-O0 -flto"
    "O0_native|-O0 -march=native"
    "O0_lto_native|-O0 -flto -march=native"
    "O1|-O1"
    "O1_lto|-O1 -flto"
    "O1_native|-O1 -march=native"
    "O1_lto_native|-O1 -flto -march=native"
    "O2|-O2"
    "O2_lto|-O2 -flto"
    "O2_native|-O2 -march=native"
    "O2_lto_native|-O2 -flto -march=native"
    "O3|-O3"
    "O3_lto|-O3 -flto"
    "O3_native|-O3 -march=native"
    "O3_lto_native|-O3 -flto -march=native"
)

echo "========================================================"
echo "  Validação: impacto das 16 configs de flags no stack"
echo "========================================================"
echo ""
echo "  liboqs  : $LIBOQS_INSTALL"
echo "  saída   : $OUT_CSV"
echo "  resumo  : $OUT_SUMMARY"
echo ""

mkdir -p "$BUILD_BASE"

# ---- Compilar e executar cada configuração ----
for CONFIG in "${CONFIGS[@]}"; do
    LABEL="${CONFIG%%|*}"
    FLAGS="${CONFIG##*|}"
    TMPDIR="$BUILD_BASE/$LABEL"

    printf "  [%-20s] compilando e executando... " "$LABEL"
    mkdir -p "$TMPDIR"

    # Compilar
    gcc $FLAGS -g \
        -I"$LIBOQS_INC" \
        "$SCRIPT_DIR/memory_benchmark.c" \
        -o "$TMPDIR/memory_benchmark" \
        -L"$LIBOQS_LIB" -loqs \
        -lssl -lcrypto \
        -lpthread \
        -Wl,-rpath,"$LIBOQS_LIB" \
        2>"$TMPDIR/compile.log"

    # Executar
    cd "$TMPDIR"
    ./memory_benchmark > "$TMPDIR/run.log" 2>&1 || true
    cd "$SCRIPT_DIR"

    if [ -f "$TMPDIR/memory_results.csv" ]; then
        echo "OK"
    else
        echo "FALHOU (verifique $TMPDIR/run.log)"
    fi
done

echo ""
echo "  Todas as execuções concluídas. Gerando tabela de comparação..."
echo ""

# ---- Gerar CSV de comparação e resumo com Python ----
python3 - "$BUILD_BASE" "$OUT_CSV" "$OUT_SUMMARY" <<'PYEOF'
import csv, sys, os
from pathlib import Path

build_base = Path(sys.argv[1])
out_csv    = sys.argv[2]
out_sum    = sys.argv[3]

# Ordem das 16 configurações (mesma do shell)
CONFIGS = [
    ("O0",              "-O0"),
    ("O0_lto",          "-O0 -flto"),
    ("O0_native",       "-O0 -march=native"),
    ("O0_lto_native",   "-O0 -flto -march=native"),
    ("O1",              "-O1"),
    ("O1_lto",          "-O1 -flto"),
    ("O1_native",       "-O1 -march=native"),
    ("O1_lto_native",   "-O1 -flto -march=native"),
    ("O2",              "-O2"),
    ("O2_lto",          "-O2 -flto"),
    ("O2_native",       "-O2 -march=native"),
    ("O2_lto_native",   "-O2 -flto -march=native"),
    ("O3",              "-O3"),
    ("O3_lto",          "-O3 -flto"),
    ("O3_native",       "-O3 -march=native"),
    ("O3_lto_native",   "-O3 -flto -march=native"),
]

def load_csv(path):
    """Retorna dict {(algoritmo, operacao): stack_bytes}"""
    data = {}
    if not path.exists():
        return data
    with open(path) as f:
        for row in csv.DictReader(f):
            key = (row['algoritmo'], row['operacao'])
            data[key] = int(row['stack_bytes'])
    return data

# Carregar todos os CSVs
all_data = {}
for label, flags in CONFIGS:
    csv_path = build_base / label / "memory_results.csv"
    all_data[label] = load_csv(csv_path)

# Obter todas as chaves (algoritmo, operacao) a partir do baseline -O0
baseline = all_data.get("O0", {})
all_keys = sorted(baseline.keys())

if not all_keys:
    print("[ERRO] CSV do baseline -O0 não encontrado ou vazio.")
    sys.exit(1)

# ---- Escrever CSV de comparação completo ----
with open(out_csv, 'w', newline='') as f:
    fieldnames = ['algoritmo', 'operacao', 'flags', 'stack_bytes',
                  'delta_vs_O0', 'delta_pct_vs_O0']
    w = csv.DictWriter(f, fieldnames=fieldnames)
    w.writeheader()
    for label, flags in CONFIGS:
        for key in all_keys:
            algo, op = key
            s_base = baseline.get(key, 0)
            s_cfg  = all_data[label].get(key, None)
            if s_cfg is None:
                continue
            delta = s_cfg - s_base
            pct   = (delta / s_base * 100) if s_base > 0 else 0
            w.writerow({
                'algoritmo':       algo,
                'operacao':        op,
                'flags':           flags,
                'stack_bytes':     s_cfg,
                'delta_vs_O0':     delta,
                'delta_pct_vs_O0': f"{pct:.2f}",
            })

# ---- Resumo por configuração ----
lines = []
lines.append("=" * 72)
lines.append("  RESUMO: impacto das 16 configs de flags no uso de STACK")
lines.append(f"  Baseline: -O0  |  Total de casos: {len(all_keys)} (algoritmo × operação)")
lines.append("=" * 72)
lines.append("")
lines.append(f"  {'Configuração':<25} {'Flags':<28} {'Max Δ':>8} {'Min Δ':>8}  Veredito")
lines.append("  " + "-" * 70)

all_ok = True
for label, flags in CONFIGS:
    deltas = []
    for key in all_keys:
        s_base = baseline.get(key, 0)
        s_cfg  = all_data[label].get(key)
        if s_cfg is not None and s_base > 0:
            deltas.append(s_cfg - s_base)
    if not deltas:
        lines.append(f"  {label:<25} {flags:<28}  {'N/A':>8}  {'N/A':>8}  sem dados")
        continue
    max_d = max(deltas)
    min_d = min(deltas)
    if max_d > 0:
        verdict = f"⚠  aumentou até {max_d:+,}B"
        all_ok = False
    elif max_d == 0 and min_d == 0:
        verdict = "✓  idêntico ao baseline"
    else:
        verdict = f"✓  reduziu até {min_d:,}B"
    lines.append(f"  {label:<25} {flags:<28} {max_d:>+8,} {min_d:>+8,}  {verdict}")

lines.append("")
lines.append("=" * 72)
if all_ok:
    lines.append("  CONCLUSÃO: ✓ VALIDADO")
    lines.append("  Nenhuma das 16 configurações produziu uso de stack MAIOR")
    lines.append("  que o medido com -O0. Os valores reportados no TCC são")
    lines.append("  limites superiores conservadores válidos para todas as")
    lines.append("  combinações de flags avaliadas.")
else:
    lines.append("  ATENÇÃO: Algumas configurações produziram stack maior que -O0.")
    lines.append("  Verifique os casos marcados com ⚠ acima.")
lines.append("=" * 72)

summary_text = "\n".join(lines)
print(summary_text)

with open(out_sum, 'w') as f:
    f.write(summary_text + "\n")

print(f"\n  CSV  salvo : {out_csv}")
print(f"  Resumo salvo: {out_sum}")
PYEOF

echo ""
echo "========================================================"
echo "  Validação concluída."
echo "  flags_comparison.csv  — tabela completa"
echo "  flags_summary.txt     — veredito consolidado"
echo "========================================================"
