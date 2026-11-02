"""
aggregate_results.py
--------------------
Merges the per-configuration CSVs into the *_all_flags.csv tables used by
the plotting script and the paper.

    results/timing/<cfg>/{kem,sig,sig_scale}.csv
        -> results/timing/{kem,sig,sig_scale}_all_flags.csv
    results/memory/<cfg>/{memory_results,massif_results_heap}.csv
        -> results/memory/{stack_rss,heap_peak}_all_flags.csv

Usage:
    python3 scripts/aggregate_results.py
"""

import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TIMING = ROOT / "results" / "timing"
MEMORY = ROOT / "results" / "memory"

CONFIGS = [f"O{o}{s}" for o in range(4) for s in ("", "_lto", "_native", "_native_lto")]

JOBS = [
    (TIMING, "kem.csv", "kem_all_flags.csv"),
    (TIMING, "sig.csv", "sig_all_flags.csv"),
    (TIMING, "sig_scale.csv", "sig_scale_all_flags.csv"),
    (MEMORY, "memory_results.csv", "stack_rss_all_flags.csv"),
    (MEMORY, "massif_results_heap.csv", "heap_peak_all_flags.csv"),
]


def aggregate(base: Path, src_name: str, out_name: str) -> None:
    header, rows = None, []
    for cfg in CONFIGS:
        path = base / cfg / src_name
        if not path.exists():
            print(f"  [skip] {path.relative_to(ROOT)} not found")
            continue
        with open(path, newline="") as f:
            reader = csv.reader(f)
            cols = next(reader)
            header = header or cols
            meta = [cfg, cfg[:2], int("native" in cfg), int("lto" in cfg)]
            rows += [meta + r for r in reader if r]
    if header is None:
        return
    out = base / out_name
    with open(out, "w", newline="") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(["config", "opt_level", "march_native", "lto"] + header)
        w.writerows(rows)
    print(f"  {out.relative_to(ROOT)}  ({len(rows)} rows)")


if __name__ == "__main__":
    for job in JOBS:
        aggregate(*job)
