#!/usr/bin/env python3
"""
parse_massif.py

Lê todos os arquivos .massif gerados pelo run_massif_all.sh e extrai
o pico de heap (mem_heap_B) de cada um, gerando um CSV consolidado.

Uso:
    python3 parse_massif.py <diretório_massif_out> <saída.csv>

Exemplo:
    python3 parse_massif.py build/massif_out build/massif_results_heap.csv

Formato do CSV gerado:
    algoritmo, operacao, heap_peak_bytes, heap_peak_kb, heap_peak_mb
"""

import sys
import os
import re
import csv
from pathlib import Path


def parse_massif_file(filepath: Path) -> dict:
    """
    Analisa um arquivo massif e retorna o pico de mem_heap_B.

    O formato massif tem snapshots assim:
        snapshot=N
        #-----------
        time=N
        mem_heap_B=N
        mem_heap_extra_B=N
        mem_stacks_B=N

    O pico é o snapshot com maior mem_heap_B.
    """
    peak_heap = 0
    peak_total = 0  # heap + extra + stacks
    current_heap = 0
    current_heap_extra = 0
    current_stacks = 0

    try:
        with open(filepath, "r") as f:
            for line in f:
                line = line.strip()
                if line.startswith("mem_heap_B="):
                    current_heap = int(line.split("=", 1)[1])
                elif line.startswith("mem_heap_extra_B="):
                    current_heap_extra = int(line.split("=", 1)[1])
                elif line.startswith("mem_stacks_B="):
                    current_stacks = int(line.split("=", 1)[1])
                    # Fim de um bloco de snapshot — atualiza pico
                    total = current_heap + current_heap_extra + current_stacks
                    if current_heap > peak_heap:
                        peak_heap = current_heap
                    if total > peak_total:
                        peak_total = total
    except Exception as e:
        print(f"  AVISO: não foi possível ler {filepath}: {e}", file=sys.stderr)
        return None

    return {
        "heap_peak_bytes":       peak_heap,
        "heap_peak_kb":          round(peak_heap / 1024, 2),
        "heap_peak_mb":          round(peak_heap / 1024 / 1024, 4),
        "total_peak_bytes":      peak_total,
        "total_peak_kb":         round(peak_total / 1024, 2),
    }


def filename_to_alg_op(stem: str):
    """
    Converte o nome do arquivo de volta para (algoritmo, operação).

    Exemplos:
      ML-KEM-512_keygen          → ("ML-KEM-512",  "keygen")
      ML-DSA-44_sign             → ("ML-DSA-44",   "sign")
      SPHINCS__SHA2-128f-simple_verify → ("SPHINCS+-SHA2-128f-simple", "verify")
    """
    # Operações conhecidas
    ops = ["keygen", "encap", "decap", "sign", "verify"]

    for op in ops:
        if stem.endswith(f"_{op}"):
            raw_alg = stem[: -(len(op) + 1)]
            # Restaura SPHINCS+ (o '+' vira '__' pelo safe_name no script)
            alg = raw_alg.replace("SPHINCS__", "SPHINCS+-")
            return alg, op

    # Fallback: último segmento é a operação
    parts = stem.rsplit("_", 1)
    if len(parts) == 2:
        return parts[0], parts[1]
    return stem, "unknown"


def main():
    if len(sys.argv) < 3:
        print(f"Uso: {sys.argv[0]} <massif_out_dir> <output.csv>")
        sys.exit(1)

    massif_dir = Path(sys.argv[1])
    out_csv    = Path(sys.argv[2])

    if not massif_dir.is_dir():
        print(f"ERRO: diretório não encontrado: {massif_dir}")
        sys.exit(1)

    files = sorted(massif_dir.glob("*.massif"))
    if not files:
        print(f"AVISO: nenhum arquivo .massif encontrado em {massif_dir}")
        sys.exit(0)

    rows = []
    print(f"Processando {len(files)} arquivo(s) massif...")

    for f in files:
        alg, op = filename_to_alg_op(f.stem)
        result = parse_massif_file(f)
        if result is None:
            continue

        row = {
            "algoritmo":        alg,
            "operacao":         op,
            "heap_peak_bytes":  result["heap_peak_bytes"],
            "heap_peak_kb":     result["heap_peak_kb"],
            "heap_peak_mb":     result["heap_peak_mb"],
            "total_peak_bytes": result["total_peak_bytes"],
            "total_peak_kb":    result["total_peak_kb"],
        }
        rows.append(row)
        print(f"  {alg:42s} {op:8s}  heap_peak={result['heap_peak_kb']:8.2f} KB"
              f"  total={result['total_peak_kb']:8.2f} KB")

    if not rows:
        print("Nenhum dado extraído.")
        sys.exit(1)

    # Ordena: algoritmo então operação
    op_order = {"keygen": 0, "encap": 1, "decap": 2, "sign": 1, "verify": 2}
    rows.sort(key=lambda r: (r["algoritmo"], op_order.get(r["operacao"], 9)))

    fieldnames = ["algoritmo", "operacao",
                  "heap_peak_bytes", "heap_peak_kb", "heap_peak_mb",
                  "total_peak_bytes", "total_peak_kb"]

    with open(out_csv, "w", newline="") as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    print(f"\nCSV salvo em: {out_csv}")
    print(f"Total de entradas: {len(rows)}")

    # ---- Resumo no terminal ----
    print("\n--- Top 5 maiores picos de heap ---")
    top5 = sorted(rows, key=lambda r: r["heap_peak_bytes"], reverse=True)[:5]
    for r in top5:
        print(f"  {r['algoritmo']:42s} {r['operacao']:8s}  {r['heap_peak_kb']:8.2f} KB")

    print("\n--- Top 5 menores picos de heap ---")
    bot5 = sorted(rows, key=lambda r: r["heap_peak_bytes"])[:5]
    for r in bot5:
        print(f"  {r['algoritmo']:42s} {r['operacao']:8s}  {r['heap_peak_kb']:8.2f} KB")


if __name__ == "__main__":
    main()
