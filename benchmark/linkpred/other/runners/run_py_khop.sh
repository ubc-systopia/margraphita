#!/usr/bin/env bash
# run_py_khop.sh — Python (PyG / NetworkX) k-hop 2 baselines on the kron graphs,
# run OUTSIDE the flexograph-perf container (WSL host), reading the same raw
# edge lists the DBs were ingested from.
#
# Each (graph, mode) is its own process, so peak memory is per mode and an
# out-of-memory kill only loses that one run. Per-node modes (khop_subgraph,
# networkx) are pure-Python loops and are skipped above SLOW_MAX.
#
# Usage (WSL host, after creating the venv, see py_khop_baseline.py):
#   ./run_py_khop.sh                                  # s12_e8 s16_e8
#   GRAPHS="s12_e8 s16_e8 s17_e8 s19_e8" ./run_py_khop.sh
#
# Env vars (all optional):
#   GRAPHS      kron graphs                 (default: "s12_e8 s16_e8")
#   MODES       py_khop_baseline modes      (default: "twohop khop_subgraph fanout networkx")
#   SLOW_MAX    largest graph for per-node modes (default: s17_e8)
#   ROUNDS      repeats per (graph, mode)   (default: 1)
#   TIMEOUT     seconds per run             (default: 3600)
#   PY          python with torch / PyG / networkx (default: ~/pyg/bin/python)
#   EDGES_ROOT  raw edge lists              (default: /mnt/c/FlexoGraphData/hdd_main/kron_graphs)
#   OUT_ROOT    results parent              (default: benchmark/linkpred/output)
#
# Output: OUT_ROOT/py_khop_<timestamp>/ with py_khop.csv, logs/, SUMMARY.md.
# OUT_ROOT is benchmark/linkpred/output; the container sees it as /FlexoGraph/pull-property/margraphita/benchmark/linkpred/output.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GRAPHS=${GRAPHS:-"s12_e8 s16_e8"}
MODES=${MODES:-"twohop khop_subgraph fanout networkx"}
SLOW_MAX=${SLOW_MAX:-s17_e8}
ROUNDS=${ROUNDS:-1}
TIMEOUT=${TIMEOUT:-3600}
PY=${PY:-$HOME/pyg/bin/python}
EDGES_ROOT=${EDGES_ROOT:-/mnt/c/FlexoGraphData/hdd_main/kron_graphs}
OUT_ROOT=${OUT_ROOT:-/home/larsav/flexography-clone-structural/pull-property/margraphita/benchmark/linkpred/output}   # WSL path of benchmark/linkpred/output
BASELINE="$SCRIPT_DIR/../py_khop_baseline.py"

[ -x "$PY" ] || { echo "ERROR: python not found: $PY (set PY=...)" >&2; exit 2; }
"$PY" -c "import torch, torch_geometric, networkx" 2>/dev/null \
    || { echo "ERROR: $PY lacks torch / torch_geometric / networkx" >&2; exit 2; }

scale() { echo "$1" | sed -E 's/^s([0-9]+)_.*/\1/'; }
OUT=$OUT_ROOT/py_khop_$(date +%Y%m%d_%H%M%S)
mkdir -p "$OUT/logs"
echo "graphs=$GRAPHS modes=$MODES rounds=$ROUNDS -> $OUT"

for g in $GRAPHS; do
    edges=$EDGES_ROOT/$g/graph_$g
    [ -f "$edges" ] || { echo "  SKIP $g: no edge list $edges"; continue; }
    for m in $MODES; do
        if { [ "$m" = khop_subgraph ] || [ "$m" = networkx ]; } \
           && [ "$(scale "$g")" -gt "$(scale "$SLOW_MAX")" ]; then
            echo "  SKIP $g $m (per-node mode above SLOW_MAX=$SLOW_MAX)"; continue
        fi
        for r in $(seq 1 "$ROUNDS"); do
            log=$OUT/logs/${g}_${m}_r$r.log
            timeout "$TIMEOUT" "$PY" "$BASELINE" --edges "$edges" --mode "$m" \
                --out "$OUT/py_khop.csv" > "$log" 2>&1
            rc=$?
            line=$(grep '^PyKhop ' "$log" | tail -1)
            if [ $rc -ne 0 ] || [ -z "$line" ]; then
                why="rc=$rc"; [ $rc -eq 124 ] && why="timeout ${TIMEOUT}s"; [ $rc -eq 137 ] && why="killed (out of memory?)"
                echo "  FAIL $g $m: $why — see $log"
            else
                echo "  $g $m: $(echo "$line" | grep -oE 'pairs_scored=[0-9]+|true_edges=[0-9]+|time_ms=[0-9.]+|peak_rss_mb=[0-9]+' | tr '\n' ' ')"
            fi
        done
    done
done

[ -f "$OUT/py_khop.csv" ] || { echo "no successful runs"; exit 1; }
"$PY" - "$OUT/py_khop.csv" "$OUT/SUMMARY.md" <<'PY'
import csv, sys, statistics as st
rows = list(csv.DictReader(open(sys.argv[1])))
# C++ reference counts (khop2, v > u) where known
expect = {"graph_s12_e8": (1077491, 32447)}
groups = {}
for r in rows:
    groups.setdefault((r["graph"], r["mode"], r["cand"]), []).append(r)
L = ["# Python k-hop 2 baselines (PyG / NetworkX)\n",
     "WSL host, outside the container. Median over rounds. `peak_rss_mb` is the process peak.\n",
     "| graph | mode | candidates | pairs | existing edges | P@10 | time_ms | load_ms | peak_rss_mb | matches C++ |",
     "|---|---|---|---|---|---|---|---|---|---|"]
for (g, m, c), rs in sorted(groups.items()):
    p, e = rs[0]["pairs_scored"], rs[0]["true_edges"]
    ok = "–"
    if g in expect and c == "khop2":
        ok = "yes" if (int(p), int(e)) == expect[g] else f"**no** (C++ {expect[g][0]:,} / {expect[g][1]:,})"
    L.append(f"| {g.replace('graph_', '')} | {m} | {c} | {int(p):,} | {int(e):,} | {rs[0]['p_at_k']} | "
             f"{st.median(float(x['time_ms']) for x in rs):,.1f} | {st.median(float(x['load_ms']) for x in rs):,.1f} | "
             f"{max(int(x['peak_rss_mb']) for x in rs):,} | {ok} |")
open(sys.argv[2], "w").write("\n".join(L) + "\n")
print(open(sys.argv[2]).read())
PY
echo "results: $OUT"
