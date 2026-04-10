"""
Plot LDBC SNB SF3 benchmark results for Flexograph vs NeuG.
Produces one PNG per query group + one special BI figure.
Usage: python plot_results.py  (run from any directory)
"""

import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

# ── Paths ─────────────────────────────────────────────────────────────────────
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR   = SCRIPT_DIR
OUT_DIR    = os.path.join(SCRIPT_DIR, "plots")
os.makedirs(OUT_DIR, exist_ok=True)

# ── Load CSVs ─────────────────────────────────────────────────────────────────
file_map = {
    "adj+col":  "fg_adj_col_sf3.csv",
    "adj+emb":  "fg_adj_emb_sf3.csv",
    "ekey+col": "fg_ekey_col_sf3.csv",
    "ekey+emb": "fg_ekey_emb_sf3.csv",
    "NeuG":     "neug_sf3.csv",
}

frames = []
for system, fname in file_map.items():
    df = pd.read_csv(os.path.join(DATA_DIR, fname))
    df["system"] = system
    frames.append(df)

all_df = pd.concat(frames, ignore_index=True)

# Normalise NeuG's w3 name to match FG naming
all_df["experiment"] = all_df["experiment"].replace(
    "w3_insert_post_hasCreator", "w3_insert_post_with_creator"
)

# Short display labels
label_map = {
    "w1_insert_person":            "w1",
    "w2_insert_knows":             "w2",
    "w3_insert_post_with_creator": "w3",
    "x4_insert_likes":             "x4",
    "r1": "r1", "r2": "r2", "r3": "r3",
    "x1": "x1", "x2": "x2", "x3": "x3", "x5": "x5",
    "a1": "a1", "a2": "a2", "a3": "a3",
    "ic3": "ic3", "ic5": "ic5", "ic7": "ic7", "ic8": "ic8", "ic9": "ic9",
    "bi1": "bi1", "bi2": "bi2", "bi12": "bi12",
}

# Drop cached variants, parallel variants, and fast_v2
EXCLUDE = ["_cached", "_parallel", "_fast_v2"]
mask_clean = ~all_df["experiment"].apply(
    lambda x: any(x.endswith(p) for p in EXCLUDE)
)
df_clean = all_df[mask_clean].copy()
df_clean["label"] = df_clean["experiment"].map(label_map)
df_clean = df_clean[df_clean["label"].notna()].copy()

# ── Style ──────────────────────────────────────────────────────────────────────
plt.rcParams.update({
    "font.family":   "sans-serif",
    "font.size":     10,
    "axes.spines.top":   False,
    "axes.spines.right": False,
})

SYSTEMS = ["adj+col", "adj+emb", "ekey+col", "ekey+emb", "NeuG"]
COLORS  = {
    "adj+col":  "#1565C0",   # dark blue
    "adj+emb":  "#42A5F5",   # light blue
    "ekey+col": "#BF360C",   # dark red-orange
    "ekey+emb": "#FFA726",   # amber
    "NeuG":     "#2E7D32",   # dark green
}

BAR_WIDTH = 0.15   # per system
GROUP_PAD = 0.10   # extra space between query groups (unused here, kept for reference)


# ── Helper: grouped bar chart ─────────────────────────────────────────────────
def plot_group(queries: list[str], title: str, fname: str,
               figsize=(9, 4), log_scale=True) -> None:
    n_s = len(SYSTEMS)
    n_q = len(queries)
    x   = np.arange(n_q)

    # Build pivot: index=label, columns=system, values=p50_ms
    subset  = df_clean[df_clean["label"].isin(queries)]
    pivot   = subset.pivot_table(index="label", columns="system",
                                 values="p50_ms", aggfunc="first")
    pivot   = pivot.reindex(queries)          # preserve desired order

    fig, ax = plt.subplots(figsize=figsize)

    offsets = np.linspace(-(n_s - 1) / 2, (n_s - 1) / 2, n_s) * BAR_WIDTH
    for sys, offset in zip(SYSTEMS, offsets):
        if sys not in pivot.columns:
            continue
        vals = pivot[sys].values.astype(float)
        bars = ax.bar(x + offset, vals, width=BAR_WIDTH,
                      color=COLORS[sys], label=sys,
                      edgecolor="white", linewidth=0.4, zorder=3)

    ax.set_xticks(x)
    ax.set_xticklabels(queries, fontsize=10)
    ax.set_ylabel("p50 latency (ms)")
    ax.set_title(title, fontweight="bold")
    if log_scale:
        ax.set_yscale("log")
        ax.yaxis.set_minor_formatter(ticker.NullFormatter())
    ax.legend(fontsize=8, framealpha=0.8)
    ax.grid(axis="y", alpha=0.25, zorder=0)
    ax.set_axisbelow(True)

    plt.tight_layout()
    path = os.path.join(OUT_DIR, fname)
    plt.savefig(path, dpi=150, bbox_inches="tight")
    plt.close()
    print(f"  saved {path}")


# ── Regular query-group plots ─────────────────────────────────────────────────
QUERY_GROUPS = [
    (["r1", "r2", "r3"],               "Simple Reads (r1–r3)",          "reads.png"),
    (["w1", "w2", "w3", "x4"],         "Writes (w1–w3, x4)",            "writes.png"),
    (["x1", "x2", "x3", "x5"],         "Cross-type Reads (x1–x5)",      "cross_reads.png"),
    (["a1", "a2", "a3"],               "Aggregates (a1–a3)",             "aggregates.png"),
    (["ic3", "ic5", "ic7", "ic8", "ic9"], "IC Queries",                 "ic_queries.png"),
]

print("Generating standard bar plots …")
for queries, title, fname in QUERY_GROUPS:
    plot_group(queries, title, fname)


# ── BI figure: bars (1T + 28T) + NeuG line ───────────────────────────────────
print("Generating BI figure …")

BI_QUERIES = ["bi1", "bi2", "bi12"]

# Single-thread BI data (from df_clean, COLUMNAR configs only — emb has no BI)
bi_single = (
    df_clean[df_clean["label"].isin(BI_QUERIES) &
             df_clean["system"].isin(["adj+col", "ekey+col"])]
    .pivot_table(index="label", columns="system", values="p50_ms", aggfunc="first")
    .reindex(BI_QUERIES)
)

# Parallel BI data
df_par = all_df[all_df["experiment"].isin(["bi1_parallel", "bi12_parallel"])].copy()
df_par["label"] = df_par["experiment"].str.replace("_parallel", "", regex=False)
bi_par = (
    df_par[df_par["system"].isin(["adj+col", "ekey+col"])]
    .pivot_table(index="label", columns="system", values="p50_ms", aggfunc="first")
    .reindex(BI_QUERIES)   # bi2 row will be all-NaN (no parallel run)
)

# NeuG reference values
neug_bi = (
    df_clean[df_clean["system"] == "NeuG"]
    .set_index("label")["p50_ms"]
    .reindex(BI_QUERIES)
)

# Bar spec: (source_df, system_col, display_label, color, x_offset_factor)
#   offset_factor is relative to BAR_WIDTH; 4 bars per query group
BAR_W_BI = 0.18
BI_BARS = [
    (bi_single, "adj+col",  "adj+col  1T",   "#1565C0", -1.5),
    (bi_single, "ekey+col", "ekey+col 1T",   "#BF360C", -0.5),
    (bi_par,    "adj+col",  "adj+col  28T",  "#64B5F6",  0.5),
    (bi_par,    "ekey+col", "ekey+col 28T",  "#EF9A9A",  1.5),
]

x = np.arange(len(BI_QUERIES))

fig, ax = plt.subplots(figsize=(8, 4.5))

for src, sys, lbl, color, pos in BI_BARS:
    if sys not in src.columns:
        continue
    vals = src[sys].values.astype(float)   # NaN → no bar rendered
    ax.bar(x + pos * BAR_W_BI, vals, width=BAR_W_BI,
           color=color, label=lbl,
           edgecolor="white", linewidth=0.4, zorder=3)

# NeuG as a line with point markers at each query position
ax.plot(x, neug_bi.values, "D--",
        color=COLORS["NeuG"], linewidth=2, markersize=8,
        label="NeuG", zorder=5)

# Annotate NeuG values
for xi, (q, v) in enumerate(zip(BI_QUERIES, neug_bi.values)):
    if not np.isnan(v):
        ax.annotate(f"{v:.0f} ms", xy=(xi, v),
                    xytext=(0, 10), textcoords="offset points",
                    ha="center", fontsize=8, color=COLORS["NeuG"])

ax.set_xticks(x)
# Put the "no parallel run" note as a second line under the bi2 tick label
xticklabels = [q if q != "bi2" else "bi2\n(no parallel run)" for q in BI_QUERIES]
ax.set_xticklabels(xticklabels, fontsize=11)
# Style the "(no parallel run)" sub-line in gray italic via tick label properties
for lbl in ax.get_xticklabels():
    if "\n" in lbl.get_text():
        lbl.set_color("gray")
        lbl.set_fontstyle("italic")
        lbl.set_fontsize(8)
ax.set_ylabel("p50 latency (ms)")
ax.set_title("BI Queries — single-thread vs 28-thread vs NeuG", fontweight="bold")
ax.set_yscale("log")
ax.yaxis.set_minor_formatter(ticker.NullFormatter())
ax.legend(fontsize=8, framealpha=0.8, ncol=2)
ax.grid(axis="y", alpha=0.25, zorder=0)
ax.set_axisbelow(True)

plt.tight_layout()
bi_path = os.path.join(OUT_DIR, "bi_queries.png")
plt.savefig(bi_path, dpi=150, bbox_inches="tight")
plt.close()
print(f"  saved {bi_path}")

print("Done.")
