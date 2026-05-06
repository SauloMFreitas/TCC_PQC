"""
gerar_graficos.py
-----------------
Gera os 3 gráficos da seção de resultados do TCC em formato PDF
pronto para inclusão no LaTeX via \\includegraphics.

Uso:
    python3 gerar_graficos.py

Saída:
    graficos/g1_kem_log.pdf
    graficos/g2_sig_log.pdf
    graficos/g3_escala_sign.pdf

Dependências:
    pip install matplotlib pandas numpy
"""

import os
import glob
import numpy as np
import pandas as pd
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
from matplotlib.lines import Line2D

# --- configuração geral SBC-friendly ---
matplotlib.rcParams.update({
    "text.usetex": False,          # True se tiver LaTeX instalado (melhor tipografia)
    "font.family": "serif",
    "font.size": 9,
    "axes.titlesize": 9,
    "axes.labelsize": 9,
    "xtick.labelsize": 8,
    "ytick.labelsize": 8,
    "legend.fontsize": 8,
    "figure.dpi": 300,
    "savefig.dpi": 300,
    "savefig.bbox": "tight",
    "savefig.pad_inches": 0.05,
    "axes.spines.top": False,
    "axes.spines.right": False,
    "axes.grid": True,
    "grid.alpha": 0.3,
    "grid.linewidth": 0.5,
})

RESULTS_DIR = "../code/build/results"
OUT_DIR     = "graficos"
os.makedirs(OUT_DIR, exist_ok=True)

# paleta acessível (adequada para impressão em P&B também)
C_BLUE   = "#185FA5"
C_TEAL   = "#1D9E75"
C_CORAL  = "#D85A30"
C_AMBER  = "#BA7517"
C_PURPLE = "#6B4FA0"

# ======================================================================
# Carrega dados
# ======================================================================

kem = pd.read_csv(f"{RESULTS_DIR}/O0/kem.csv")
sig = pd.read_csv(f"{RESULTS_DIR}/O0/sig.csv")

scale_frames = []
for path in sorted(glob.glob(f"{RESULTS_DIR}/*/sig_scale.csv")):
    cfg = path.split("/")[-2]
    df  = pd.read_csv(path)
    df["config"] = cfg
    scale_frames.append(df)
scale_all = pd.concat(scale_frames, ignore_index=True)
scale_o0  = scale_all[scale_all["config"] == "O0"].sort_values(
    ["algoritmo_nist", "msg_bytes"]
)

# ======================================================================
# G1 — KEM: comparação em escala logarítmica
# ======================================================================

def plot_g1():
    fig, ax = plt.subplots(figsize=(7.2, 3.4))

    algs   = kem["algoritmo_nist"].tolist()
    x      = np.arange(len(algs))
    width  = 0.26

    kg = kem["keygen_mean_us"].values
    en = kem["encaps_mean_us"].values
    dc = kem["decaps_mean_us"].values

    kg_sd = kem["keygen_stddev_us"].values
    en_sd = kem["encaps_stddev_us"].values
    dc_sd = kem["decaps_stddev_us"].values

    kw_err = dict(elinewidth=0.8, capsize=2, capthick=0.8, ecolor="#444")

    b1 = ax.bar(x - width, kg, width, label="KeyGen",
                color=C_BLUE,   alpha=0.88, yerr=kg_sd, error_kw=kw_err)
    b2 = ax.bar(x,          en, width, label="Encaps",
                color=C_TEAL,  alpha=0.88, yerr=en_sd, error_kw=kw_err)
    b3 = ax.bar(x + width,  dc, width, label="Decaps",
                color=C_CORAL, alpha=0.88, yerr=dc_sd, error_kw=kw_err)

    ax.set_yscale("log")
    ax.set_xticks(x)
    ax.set_xticklabels(algs, rotation=30, ha="right")
    ax.set_ylabel("Tempo ($\\mu$s) — escala log")
    ax.set_xlabel("Algoritmo")

    # linha divisória entre famílias
    ax.axvline(x=2.5, color="gray", linewidth=0.8, linestyle="--", alpha=0.6)
    ax.text(1.2, 5, "ML-KEM", fontsize=7.5, color="gray", ha="center")
    ax.text(4.2, 5, "HQC",    fontsize=7.5, color="gray", ha="center")

    # anotação de fator
    ax.annotate("", xy=(5 + width, dc[-1]), xytext=(2 + width, dc[2]),
                arrowprops=dict(arrowstyle="->", color="gray",
                                lw=0.8, connectionstyle="arc3,rad=0.2"))

    ax.legend(loc="upper left", framealpha=0.7)
    ax.yaxis.set_major_formatter(
        ticker.FuncFormatter(lambda v, _: f"{v/1000:.0f}ms" if v >= 1000 else f"{v:.0f}µs")
    )

    fig.tight_layout()
    path = f"{OUT_DIR}/g1_kem_log.pdf"
    fig.savefig(path)
    plt.close(fig)
    print(f"  Salvo: {path}")


# ======================================================================
# G2 — Assinaturas: comparação em escala logarítmica
# ======================================================================

def plot_g2():
    fig, ax = plt.subplots(figsize=(7.2, 4.0))

    algs_short = {
        "ML-DSA-44":            "ML-DSA\n44",
        "ML-DSA-65":            "ML-DSA\n65",
        "ML-DSA-87":            "ML-DSA\n87",
        "FN-DSA-512":           "FN-DSA\n512",
        "FN-DSA-1024":          "FN-DSA\n1024",
        "SLH-DSA-SHAKE-128s":  "SLH\n128s",
        "SLH-DSA-SHAKE-128f":  "SLH\n128f",
        "SLH-DSA-SHAKE-192s":  "SLH\n192s",
        "SLH-DSA-SHAKE-192f":  "SLH\n192f",
        "SLH-DSA-SHAKE-256s":  "SLH\n256s",
        "SLH-DSA-SHAKE-256f":  "SLH\n256f",
    }
    labels = list(algs_short.values())
    x      = np.arange(len(labels))
    width  = 0.26

    kg = sig["keygen_mean_us"].values
    sg = sig["sign_mean_us"].values
    vf = sig["verify_mean_us"].values

    kg_sd = sig["keygen_stddev_us"].values
    sg_sd = sig["sign_stddev_us"].values
    vf_sd = sig["verify_stddev_us"].values

    kw_err = dict(elinewidth=0.7, capsize=1.8, capthick=0.7, ecolor="#444")

    ax.bar(x - width, kg, width, label="KeyGen",
           color=C_BLUE,   alpha=0.88, yerr=kg_sd, error_kw=kw_err)
    ax.bar(x,          sg, width, label="Sign",
           color=C_TEAL,  alpha=0.88, yerr=sg_sd, error_kw=kw_err)
    ax.bar(x + width,  vf, width, label="Verify",
           color=C_CORAL, alpha=0.88, yerr=vf_sd, error_kw=kw_err)

    ax.set_yscale("log")
    ax.set_xticks(x)
    ax.set_xticklabels(labels, fontsize=7.5)
    ax.set_ylabel("Tempo ($\\mu$s) — escala log")
    ax.set_xlabel("Algoritmo")

    # divisórias de família
    for xv, lbl in [(2.5, ""), (4.5, "")]:
        ax.axvline(x=xv, color="gray", linewidth=0.7, linestyle="--", alpha=0.5)

    for xc, txt in [(1.0, "ML-DSA"), (3.5, "FN-DSA"), (7.5, "SLH-DSA")]:
        ax.text(xc, 1.5, txt, fontsize=7, color="gray", ha="center")

    ax.yaxis.set_major_formatter(
        ticker.FuncFormatter(lambda v, _:
            f"{v/1e6:.0f}s"  if v >= 1e6 else
            f"{v/1000:.0f}ms" if v >= 1000 else
            f"{v:.0f}µs")
    )
    ax.legend(loc="upper left", framealpha=0.7)

    fig.tight_layout()
    path = f"{OUT_DIR}/g2_sig_log.pdf"
    fig.savefig(path)
    plt.close(fig)
    print(f"  Salvo: {path}")


# ======================================================================
# G3 — Escalabilidade: Sign vs tamanho de mensagem
# ======================================================================

def plot_g3():
    import glob as _glob

    scale_frames = []
    for path in sorted(_glob.glob(f"{RESULTS_DIR}/*/sig_scale.csv")):
        cfg = path.split("/")[-2]; df = pd.read_csv(path); df["config"] = cfg
        scale_frames.append(df)
    scale_all = pd.concat(scale_frames, ignore_index=True)
    scale_o0  = scale_all[scale_all["config"] == "O0"].sort_values(
        ["algoritmo_nist", "msg_bytes"]
    )

    msg_bytes = sorted(scale_o0["msg_bytes"].unique())
    x_ticks   = [1, 4, 16, 64, 256, 1024, 4096, 16384, 65536, 262144, 1048576]
    x_labels  = ["1B","4B","16B","64B","256B","1KB","4KB","16KB","64KB","256KB","1MB"]

    def get_sign(alg):
        d = scale_o0[scale_o0["algoritmo_nist"] == alg].sort_values("msg_bytes")
        return d["sign_mean_us"].values

    def setup_ax(ax, ylabel, title):
        ax.set_xscale("log", base=2)
        ax.set_xticks(x_ticks)
        ax.set_xticklabels(x_labels, rotation=30, ha="right", fontsize=7.5)
        ax.set_ylabel(ylabel)
        ax.set_xlabel("Tamanho da mensagem")
        ax.set_title(title, fontsize=9)
        ax.legend(fontsize=7.5, framealpha=0.75, ncol=2, loc="upper left")
        ax.yaxis.set_major_formatter(
            ticker.FuncFormatter(lambda v, _:
                f"{v/1e6:.2f}s"   if v >= 1e6  else
                f"{v/1000:.0f}ms" if v >= 1000 else
                f"{v:.0f}µs"))

    # --- G3a: ML-DSA (3 variantes) + FN-DSA (2 variantes) ---
    fig_a, ax_a = plt.subplots(figsize=(7.2, 3.6))
    for alg, color, ls, marker in [
        ("ML-DSA-44",   C_BLUE,   "-",   "o"),
        ("ML-DSA-65",   C_TEAL,   "--",  "s"),
        ("ML-DSA-87",   C_CORAL,  "-.",  "^"),
        ("FN-DSA-512",  C_AMBER,  ":",   "D"),
        ("FN-DSA-1024", C_PURPLE, "--",  "P"),
    ]:
        ax_a.plot(msg_bytes, get_sign(alg), color=color, linestyle=ls,
                  marker=marker, markersize=3.5, linewidth=1.4,
                  markevery=4, label=alg)
    setup_ax(ax_a, "Tempo de assinatura (µs)",
             "Escalabilidade de Sign — ML-DSA e FN-DSA (−O0)")
    fig_a.tight_layout()
    fig_a.savefig(f"{OUT_DIR}/g3a_escala_sign_lattice.pdf"); plt.close(fig_a)
    print(f"  Salvo: {OUT_DIR}/g3a_escala_sign_lattice.pdf")

    # --- G3b: SLH-DSA variantes f (128f / 192f / 256f) ---
    fig_b, ax_b = plt.subplots(figsize=(7.2, 3.6))
    for alg, color, ls, marker in [
        ("SLH-DSA-SHAKE-128f", C_BLUE,  "-",   "o"),
        ("SLH-DSA-SHAKE-192f", C_TEAL,  "--",  "s"),
        ("SLH-DSA-SHAKE-256f", C_CORAL, "-.",  "^"),
    ]:
        ax_b.plot(msg_bytes, get_sign(alg), color=color, linestyle=ls,
                  marker=marker, markersize=3.5, linewidth=1.4,
                  markevery=4, label=alg)
    setup_ax(ax_b, "Tempo de assinatura (µs)",
             "Escalabilidade de Sign — SLH-DSA variantes f (−O0)")
    fig_b.tight_layout()
    fig_b.savefig(f"{OUT_DIR}/g3b_escala_sign_slh_f.pdf"); plt.close(fig_b)
    print(f"  Salvo: {OUT_DIR}/g3b_escala_sign_slh_f.pdf")

    # --- G3c: SLH-DSA variantes s (128s / 192s / 256s) ---
    fig_c, ax_c = plt.subplots(figsize=(7.2, 3.6))
    for alg, color, ls, marker in [
        ("SLH-DSA-SHAKE-128s", C_BLUE,  "-",   "o"),
        ("SLH-DSA-SHAKE-192s", C_TEAL,  "--",  "s"),
        ("SLH-DSA-SHAKE-256s", C_CORAL, "-.",  "^"),
    ]:
        ax_c.plot(msg_bytes, get_sign(alg), color=color, linestyle=ls,
                  marker=marker, markersize=3.5, linewidth=1.4,
                  markevery=4, label=alg)
    setup_ax(ax_c, "Tempo de assinatura (µs)",
             "Escalabilidade de Sign — SLH-DSA variantes s (−O0)")
    fig_c.tight_layout()
    fig_c.savefig(f"{OUT_DIR}/g3c_escala_sign_slh_s.pdf"); plt.close(fig_c)
    print(f"  Salvo: {OUT_DIR}/g3c_escala_sign_slh_s.pdf")


# ======================================================================
# Executa
# ======================================================================

# ======================================================================
# G4 — Impacto das flags de compilação (ganho % vs O0)
# ======================================================================

def plot_g4():
    import glob as _glob

    kem_frames, sig_frames = [], []
    for path in sorted(_glob.glob(f"{RESULTS_DIR}/*/kem.csv")):
        cfg = path.split("/")[-2]; df = pd.read_csv(path); df["config"] = cfg
        kem_frames.append(df)
    for path in sorted(_glob.glob(f"{RESULTS_DIR}/*/sig.csv")):
        cfg = path.split("/")[-2]; df = pd.read_csv(path); df["config"] = cfg
        sig_frames.append(df)

    kem_all = pd.concat(kem_frames, ignore_index=True)
    sig_all = pd.concat(sig_frames, ignore_index=True)

    cfg_order = [
        "O0","O0_lto","O0_native","O0_native_lto",
        "O1","O1_lto","O1_native","O1_native_lto",
        "O2","O2_lto","O2_native","O2_native_lto",
        "O3","O3_lto","O3_native","O3_native_lto",
    ]
    cfg_labels = [
        "O0","O0\nlto","O0\nnat","O0\nnat+lto",
        "O1","O1\nlto","O1\nnat","O1\nnat+lto",
        "O2","O2\nlto","O2\nnat","O2\nnat+lto",
        "O3","O3\nlto","O3\nnat","O3\nnat+lto",
    ]

    def speedups(df, alg, op):
        vals = [df[(df.algoritmo_nist==alg)&(df.config==c)][op].values[0]
                for c in cfg_order]
        base = vals[0]
        return [base / v for v in vals]

    cfg_order = [
        "O0","O0_lto","O0_native","O0_native_lto",
        "O1","O1_lto","O1_native","O1_native_lto",
        "O2","O2_lto","O2_native","O2_native_lto",
        "O3","O3_lto","O3_native","O3_native_lto",
    ]
    cfg_labels = [
        "O0","O0\n+lto","O0\n+nat","O0\n+n+l",
        "O1","O1\n+lto","O1\n+nat","O1\n+n+l",
        "O2","O2\n+lto","O2\n+nat","O2\n+n+l",
        "O3","O3\n+lto","O3\n+nat","O3\n+n+l",
    ]

    level_colors = (
        ["#AAAAAA","#BBBBBB","#CCCCCC","#DDDDDD"] +
        ["#5B8FD4","#7AA3DE","#99B7E8","#B8CBF2"] +
        ["#2E9E6B","#4DB882","#6CD299","#8BECB0"] +
        ["#D87D2A","#E69344","#F4A95E","#FFBF78"]
    )

    def get_speedup(df, alg, op):
        vals = np.array([
            df[(df.algoritmo_nist==alg)&(df.config==c)][op].values[0]
            for c in cfg_order
        ])
        return vals[0] / vals

    def plot_single(alg, df, ops, op_cols, unit, filename):
        fig, ax = plt.subplots(figsize=(7.2, 3.2))
        x = np.arange(len(cfg_order))
        w = 0.26
        offsets = [-w, 0, w]

        for (op_label, op_col), offset in zip(zip(ops, op_cols), offsets):
            sp = get_speedup(df, alg, op_col)
            ax.bar(x + offset, sp, w,
                   color=level_colors, edgecolor="white",
                   linewidth=0.3, label=op_label,
                   alpha=[1.0, 0.72, 0.48][offsets.index(offset)])

        ax.axhline(1.0, color="black", linewidth=0.8,
                   linestyle="--", alpha=0.45, zorder=0)

        for xv in [3.5, 7.5, 11.5]:
            ax.axvline(xv, color="gray", linewidth=0.6,
                       linestyle=":", alpha=0.4, zorder=0)

        for xc, txt in [(1.5,"O0"),(5.5,"O1"),(9.5,"O2"),(13.5,"O3")]:
            ax.text(xc, ax.get_ylim()[1] * 0.995 if ax.get_ylim()[1] > 1.15 else 1.155,
                    txt, ha="center", fontsize=7.5, color="gray",
                    bbox=dict(boxstyle="round,pad=0.2", fc="white", ec="none"))

        ax.set_xticks(x)
        ax.set_xticklabels(cfg_labels, fontsize=7)
        ax.set_ylabel(f"Speedup vs. \\texttt{{-O0}}")
        ax.set_xlabel("Configuração de compilação")
        ax.set_ylim(0.82, 1.20)
        ax.yaxis.set_major_formatter(
            ticker.FuncFormatter(lambda v, _: f"{v:.2f}×"))
        ax.set_title(f"Speedup por configuração de compilação — {alg} ({unit})",
                     fontsize=9)

        # legenda operações
        from matplotlib.patches import Patch
        op_handles = [Patch(fc="#888888", alpha=[1.0,0.72,0.48][i], label=ops[i])
                      for i in range(len(ops))]
        # legenda nível O
        level_handles = [
            Patch(fc="#AAAAAA", label="O0"),
            Patch(fc="#5B8FD4", label="O1"),
            Patch(fc="#2E9E6B", label="O2"),
            Patch(fc="#D87D2A", label="O3"),
        ]
        l1 = ax.legend(handles=op_handles, loc="upper left",
                       fontsize=7, framealpha=0.7, title="Operação",
                       title_fontsize=7)
        ax.add_artist(l1)
        ax.legend(handles=level_handles, loc="upper right",
                  fontsize=7, framealpha=0.7, title="Nível -O",
                  title_fontsize=7, ncol=2)

        fig.tight_layout()
        fig.savefig(filename, bbox_inches="tight")
        plt.close(fig)
        print(f"  Salvo: {filename}")

    # KEM
    for alg in ["ML-KEM-512","ML-KEM-768","ML-KEM-1024",
                "HQC-128","HQC-192","HQC-256"]:
        safe = alg.replace("-","_").lower()
        plot_single(alg, kem_all,
                    ["KeyGen","Encaps","Decaps"],
                    ["keygen_mean_us","encaps_mean_us","decaps_mean_us"],
                    "µs", f"{OUT_DIR}/g4_{safe}.pdf")

    # SIG
    for alg in ["ML-DSA-44","ML-DSA-65","ML-DSA-87",
                "FN-DSA-512","FN-DSA-1024"]:
        safe = alg.replace("-","_").lower()
        plot_single(alg, sig_all,
                    ["KeyGen","Sign","Verify"],
                    ["keygen_mean_us","sign_mean_us","verify_mean_us"],
                    "µs", f"{OUT_DIR}/g4_{safe}.pdf")

    for alg in ["SLH-DSA-SHAKE-128s","SLH-DSA-SHAKE-128f",
                "SLH-DSA-SHAKE-192s","SLH-DSA-SHAKE-192f",
                "SLH-DSA-SHAKE-256s","SLH-DSA-SHAKE-256f"]:
        safe = alg.replace("-","_").replace("(","").replace(")","").lower()
        plot_single(alg, sig_all,
                    ["KeyGen","Sign","Verify"],
                    ["keygen_mean_us","sign_mean_us","verify_mean_us"],
                    "ms", f"{OUT_DIR}/g4_{safe}.pdf")

    return


# ======================================================================
# G5 — Escalabilidade: Verify vs tamanho de mensagem
# ======================================================================

def plot_g5():
    import glob as _glob

    scale_frames = []
    for path in sorted(_glob.glob(f"{RESULTS_DIR}/*/sig_scale.csv")):
        cfg = path.split("/")[-2]; df = pd.read_csv(path); df["config"] = cfg
        scale_frames.append(df)
    scale_all = pd.concat(scale_frames, ignore_index=True)
    scale_o0  = scale_all[scale_all["config"] == "O0"].sort_values(
        ["algoritmo_nist", "msg_bytes"]
    )

    msg_bytes = sorted(scale_o0["msg_bytes"].unique())
    x_ticks   = [1, 4, 16, 64, 256, 1024, 4096, 16384, 65536, 262144, 1048576]
    x_labels  = ["1B","4B","16B","64B","256B","1KB","4KB","16KB","64KB","256KB","1MB"]
    kb        = np.array([1024, 1048576])

    def get_verify(alg):
        d = scale_o0[scale_o0["algoritmo_nist"] == alg].sort_values("msg_bytes")
        return d["verify_mean_us"].values

    def setup_ax(ax, ylabel, title):
        ax.set_xscale("log", base=2)
        ax.set_xticks(x_ticks)
        ax.set_xticklabels(x_labels, rotation=30, ha="right", fontsize=7.5)
        ax.set_ylabel(ylabel)
        ax.set_xlabel("Tamanho da mensagem")
        ax.set_title(title, fontsize=9)
        ax.yaxis.set_major_formatter(
            ticker.FuncFormatter(lambda v, _:
                f"{v/1000:.0f}ms" if v >= 1000 else f"{v:.0f}µs"))
        ax.legend(fontsize=7.5, framealpha=0.75, ncol=2, loc="upper left")

    # --- G5a: ML-DSA (3) + FN-DSA (2) ---
    fig_a, ax_a = plt.subplots(figsize=(7.2, 3.6))
    for alg, color, ls, marker in [
        ("ML-DSA-44",   C_BLUE,   "-",   "o"),
        ("ML-DSA-65",   C_TEAL,   "--",  "s"),
        ("ML-DSA-87",   C_CORAL,  "-.",  "^"),
        ("FN-DSA-512",  C_AMBER,  ":",   "D"),
        ("FN-DSA-1024", C_PURPLE, "--",  "P"),
    ]:
        ax_a.plot(msg_bytes, get_verify(alg), color=color, linestyle=ls,
                  marker=marker, markersize=3.5, linewidth=1.4,
                  markevery=4, label=alg)
    setup_ax(ax_a, "Tempo de verificação (µs)",
             "Escalabilidade de Verify — ML-DSA e FN-DSA (−O0)")
    fig_a.tight_layout()
    fig_a.savefig(f"{OUT_DIR}/g5a_escala_verify_lattice.pdf"); plt.close(fig_a)
    print(f"  Salvo: {OUT_DIR}/g5a_escala_verify_lattice.pdf")

    # --- G5b: SLH-DSA todos os 6 ---
    fig_b, ax_b = plt.subplots(figsize=(7.2, 3.6))
    slh_styles = [
        ("SLH-DSA-SHAKE-128s", C_BLUE,   "-",   "o"),
        ("SLH-DSA-SHAKE-128f", C_BLUE,   "--",  "o"),
        ("SLH-DSA-SHAKE-192s", C_TEAL,   "-",   "s"),
        ("SLH-DSA-SHAKE-192f", C_TEAL,   "--",  "s"),
        ("SLH-DSA-SHAKE-256s", C_CORAL,  "-",   "^"),
        ("SLH-DSA-SHAKE-256f", C_CORAL,  "--",  "^"),
    ]
    for alg, color, ls, marker in slh_styles:
        label = alg.replace("SLH-DSA-SHAKE-", "SLH-")
        ax_b.plot(msg_bytes, get_verify(alg), color=color, linestyle=ls,
                  marker=marker, markersize=3.5, linewidth=1.4,
                  markevery=4, label=label, alpha=0.9 if "s" in alg[-1] else 0.6)
    setup_ax(ax_b, "Tempo de verificação (µs)",
             "Escalabilidade de Verify — SLH-DSA todas as variantes (−O0)")
    fig_b.tight_layout()
    fig_b.savefig(f"{OUT_DIR}/g5b_escala_verify_slh.pdf"); plt.close(fig_b)
    print(f"  Salvo: {OUT_DIR}/g5b_escala_verify_slh.pdf")


# adiciona chamadas ao bloco principal

# ======================================================================
# G6 — Heatmap de tempo de execução por flag (normalizado por linha)
# ======================================================================

def plot_g6():
    import glob as _glob

    kem_frames, sig_frames = [], []
    for path in sorted(_glob.glob(f"{RESULTS_DIR}/*/kem.csv")):
        cfg = path.split("/")[-2]; df = pd.read_csv(path); df["config"] = cfg
        kem_frames.append(df)
    for path in sorted(_glob.glob(f"{RESULTS_DIR}/*/sig.csv")):
        cfg = path.split("/")[-2]; df = pd.read_csv(path); df["config"] = cfg
        sig_frames.append(df)

    kem_all = pd.concat(kem_frames, ignore_index=True)
    sig_all = pd.concat(sig_frames, ignore_index=True)

    cfg_order = [
        "O0","O0_lto","O0_native","O0_native_lto",
        "O1","O1_lto","O1_native","O1_native_lto",
        "O2","O2_lto","O2_native","O2_native_lto",
        "O3","O3_lto","O3_native","O3_native_lto",
    ]
    cfg_labels = [
        "O0","O0\n+lto","O0\n+nat","O0\n+nat\n+lto",
        "O1","O1\n+lto","O1\n+nat","O1\n+nat\n+lto",
        "O2","O2\n+lto","O2\n+nat","O2\n+nat\n+lto",
        "O3","O3\n+lto","O3\n+nat","O3\n+nat\n+lto",
    ]

    # --- KEM ---
    kem_rows = [
        ("ML-KEM-512",  kem_all, "decaps_mean_us",  "ML-KEM-512\n(Decaps, µs)"),
        ("ML-KEM-768",  kem_all, "decaps_mean_us",  "ML-KEM-768\n(Decaps, µs)"),
        ("ML-KEM-1024", kem_all, "decaps_mean_us",  "ML-KEM-1024\n(Decaps, µs)"),
        ("HQC-128",     kem_all, "decaps_mean_us",  "HQC-128\n(Decaps, ms)"),
        ("HQC-192",     kem_all, "decaps_mean_us",  "HQC-192\n(Decaps, ms)"),
        ("HQC-256",     kem_all, "decaps_mean_us",  "HQC-256\n(Decaps, ms)"),
    ]

    # --- SIG: ML-DSA + FN-DSA ---
    sig_rows = [
        ("ML-DSA-44",   sig_all, "sign_mean_us",   "ML-DSA-44\n(Sign, µs)"),
        ("ML-DSA-65",   sig_all, "sign_mean_us",   "ML-DSA-65\n(Sign, µs)"),
        ("ML-DSA-87",   sig_all, "sign_mean_us",   "ML-DSA-87\n(Sign, µs)"),
        ("FN-DSA-512",  sig_all, "keygen_mean_us", "FN-DSA-512\n(KeyGen, ms)"),
        ("FN-DSA-1024", sig_all, "keygen_mean_us", "FN-DSA-1024\n(KeyGen, ms)"),
    ]

    # --- SIG: SLH-DSA ---
    slh_rows = [
        ("SLH-DSA-SHAKE-128s", sig_all, "sign_mean_us",   "SLH-128s\n(Sign, ms)"),
        ("SLH-DSA-SHAKE-128f", sig_all, "sign_mean_us",   "SLH-128f\n(Sign, ms)"),
        ("SLH-DSA-SHAKE-192s", sig_all, "sign_mean_us",   "SLH-192s\n(Sign, ms)"),
        ("SLH-DSA-SHAKE-192f", sig_all, "sign_mean_us",   "SLH-192f\n(Sign, ms)"),
        ("SLH-DSA-SHAKE-256s", sig_all, "sign_mean_us",   "SLH-256s\n(Sign, ms)"),
        ("SLH-DSA-SHAKE-256f", sig_all, "sign_mean_us",   "SLH-256f\n(Sign, ms)"),
    ]

    def build_matrix(rows):
        matrix, ylabels, annotations = [], [], []
        for alg, df, op, lbl in rows:
            vals = np.array([
                df[(df.algoritmo_nist==alg)&(df.config==c)][op].values[0]
                for c in cfg_order
            ])
            # normalise row: 0 = fastest, 1 = slowest
            vmin, vmax = vals.min(), vals.max()
            norm = (vals - vmin) / (vmax - vmin + 1e-12)
            matrix.append(norm)
            ylabels.append(lbl)
            # annotation: actual value formatted
            scale = 1000 if vals.max() > 1000 else 1
            unit  = "ms" if scale == 1000 else "µs"
            annotations.append([f"{v/scale:.1f}" for v in vals])
        return np.array(matrix), ylabels, annotations

    def draw_heatmap(ax, matrix, ylabels, annotations, title):
        im = ax.imshow(matrix, aspect="auto", cmap="RdYlGn_r",
                       vmin=0, vmax=1, interpolation="nearest")

        ax.set_xticks(np.arange(len(cfg_order)))
        ax.set_xticklabels(cfg_labels, fontsize=6.5)
        ax.set_yticks(np.arange(len(ylabels)))
        ax.set_yticklabels(ylabels, fontsize=7.5)
        ax.set_title(title, fontsize=8.5, pad=6)

        # divisórias entre grupos O0/O1/O2/O3
        for xv in [3.5, 7.5, 11.5]:
            ax.axvline(xv, color="white", linewidth=1.2)

        # valores nas células
        for i, row_ann in enumerate(annotations):
            for j, txt in enumerate(row_ann):
                color = "white" if matrix[i, j] > 0.65 else "black"
                ax.text(j, i, txt, ha="center", va="center",
                        fontsize=5.8, color=color, fontweight="normal")

        return im

    # --- figura KEM ---
    mat_k, ylbl_k, ann_k = build_matrix(kem_rows)
    fig_k, ax_k = plt.subplots(figsize=(7.2, 2.8))
    draw_heatmap(ax_k, mat_k, ylbl_k, ann_k,
                 "KEM — tempo de execução por configuração de compilação\n"
                 "(verde = mais rápido, vermelho = mais lento; normalizado por linha)")
    fig_k.colorbar(
        plt.cm.ScalarMappable(cmap="RdYlGn_r"),
        ax=ax_k, orientation="vertical", fraction=0.02, pad=0.02,
        label="Relativo (0 = mín, 1 = máx por algoritmo)"
    )
    fig_k.tight_layout()
    path_k = f"{OUT_DIR}/g6a_heatmap_kem.pdf"
    fig_k.savefig(path_k)
    plt.close(fig_k)
    print(f"  Salvo: {path_k}")

    # --- figura SIG: ML-DSA + FN-DSA ---
    mat_s, ylbl_s, ann_s = build_matrix(sig_rows)
    fig_s, ax_s = plt.subplots(figsize=(7.2, 2.8))
    draw_heatmap(ax_s, mat_s, ylbl_s, ann_s,
                 "ML-DSA / FN-DSA — tempo de execução por configuração\n"
                 "(verde = mais rápido, vermelho = mais lento; normalizado por linha)")
    fig_s.colorbar(
        plt.cm.ScalarMappable(cmap="RdYlGn_r"),
        ax=ax_s, orientation="vertical", fraction=0.02, pad=0.02,
        label="Relativo (0 = mín, 1 = máx por algoritmo)"
    )
    fig_s.tight_layout()
    path_s = f"{OUT_DIR}/g6b_heatmap_sig.pdf"
    fig_s.savefig(path_s)
    plt.close(fig_s)
    print(f"  Salvo: {path_s}")

    # --- figura SIG: SLH-DSA ---
    mat_slh, ylbl_slh, ann_slh = build_matrix(slh_rows)
    fig_slh, ax_slh = plt.subplots(figsize=(7.2, 3.2))
    draw_heatmap(ax_slh, mat_slh, ylbl_slh, ann_slh,
                 "SLH-DSA-SHAKE — tempo de assinatura por configuração (ms)\n"
                 "(verde = mais rápido, vermelho = mais lento; normalizado por linha)")
    fig_slh.colorbar(
        plt.cm.ScalarMappable(cmap="RdYlGn_r"),
        ax=ax_slh, orientation="vertical", fraction=0.02, pad=0.02,
        label="Relativo (0 = mín, 1 = máx por algoritmo)"
    )
    fig_slh.tight_layout()
    path_slh = f"{OUT_DIR}/g6c_heatmap_slh.pdf"
    fig_slh.savefig(path_slh)
    plt.close(fig_slh)
    print(f"  Salvo: {path_slh}")


if __name__ == '__main__':
    print('Gerando gráficos...')
    plot_g1()
    plot_g2()
    plot_g3()
    plot_g4()
    plot_g5()
    plot_g6()
    print('Concluído. Arquivos em ./graficos/')