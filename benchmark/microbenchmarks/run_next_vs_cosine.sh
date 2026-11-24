#!/usr/bin/env bash
# run_next_vs_cosine.sh — run next_vs_cosine on each graph's active DB and log
# everything under the ML drive.
#
# The DB holds whatever embedding was loaded last (add_kron_emb.sh overwrites);
# next_vs_cosine probes that dim itself, so load first, then run this.
#
# Usage (inside the flexograph-perf container):
#   ./run_next_vs_cosine.sh                      # s12 s16 s17 s19 s20, adj
#   ./run_next_vs_cosine.sh s12_e8 s20_e8        # chosen graphs
#
# Env vars (all optional):
#   BIN       next_vs_cosine binary  (default: sibling next_vs_cosine)
#   GT        adj | split_ekey       (default: adj)
#   DIMS      extra cosine dims, e.g. "64,128" (default: none -> DB dim only)
#   ROUNDS    rounds, median taken   (default: 3)
#   CALLS     calls per op per round (default: 2000000)
#   WT_CACHE  WiredTiger cache size, e.g. 16MB (default: WiredTiger's own);
#             logs/CSV record it, log names get _wt<size>
#   EXPECT_DIM  embedding dim the DBs should hold; a run whose DB holds a
#             different dim is marked DIM MISMATCH and fails (default: unchecked)
#   DB_ROOT   kron DB root           (default: /drives/ssd2/flexograph_rd/kron)
#   LOG_ROOT  output folder          (default: see below; README.md explains it)

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN=${BIN:-$SCRIPT_DIR/next_vs_cosine}
GT=${GT:-adj}
DIMS=${DIMS:-}
ROUNDS=${ROUNDS:-3}
CALLS=${CALLS:-2000000}
WT_CACHE=${WT_CACHE:-}
EXPECT_DIM=${EXPECT_DIM:-}
DB_ROOT=${DB_ROOT:-/drives/ssd2/flexograph_rd/kron}
FAILS=0
LOG_ROOT=${LOG_ROOT:-/drives/ml-pytorch-data/linkpred/flexography/microbench/next_vs_cosine}

GRAPHS=("$@")
[ ${#GRAPHS[@]} -eq 0 ] && GRAPHS=(s12_e8 s16_e8 s17_e8 s19_e8 s20_e8)

[ -x "$BIN" ] || { echo "ERROR: binary not executable: $BIN (set BIN=...)" >&2; exit 2; }

mkdir -p "$LOG_ROOT/logs"
CSV="$LOG_ROOT/results.csv"
EXTRA=()
[ -n "$DIMS" ] && EXTRA+=(--dims="$DIMS")
[ -n "$WT_CACHE" ] && EXTRA+=(--wt-cache="$WT_CACHE")
WT_TAG=""
[ -n "$WT_CACHE" ] && WT_TAG="_wt${WT_CACHE}"

for g in "${GRAPHS[@]}"; do
    if [ "$GT" = "adj" ]; then DB_NAME="adj_r_$g"; else DB_NAME="split_ekey_r_$g"; fi
    if [ ! -d "$DB_ROOT/$g/$DB_NAME" ]; then
        echo "[SKIP] $g: DB not found: $DB_ROOT/$g/$DB_NAME"
        continue
    fi
    ts=$(date +%Y%m%d_%H%M%S)
    tmp="$LOG_ROOT/logs/${g}_${GT}_${ts}.log"
    echo "=== $g ($GT) ==="
    "$BIN" -p "$DB_ROOT/$g" -m "$DB_NAME" -g "$GT" -r --props \
        --rounds="$ROUNDS" --calls="$CALLS" --out="$CSV" "${EXTRA[@]}" > "$tmp" 2>&1
    rc=$?
    # Name the log by the dim the DB actually held.
    dim=$(grep -oE 'db_dim=[0-9]+' "$tmp" | head -1 | cut -d= -f2)
    log="$LOG_ROOT/logs/${g}_${GT}_dim${dim:-unknown}${WT_TAG}_${ts}.log"
    mv "$tmp" "$log"
    if [ $rc -ne 0 ]; then
        echo "  FAIL rc=$rc — see $log"
        FAILS=$((FAILS + 1))
        continue
    fi
    if [ -n "$EXPECT_DIM" ] && [ "$dim" != "$EXPECT_DIM" ]; then
        echo "  DIM MISMATCH: expected dim $EXPECT_DIM, DB held ${dim:-unknown} — see $log"
        FAILS=$((FAILS + 1))
    fi
    grep -A40 '^graph=' "$log" | sed 's/^/  /'
    echo "  log: $log"
done
echo ""
echo "results (appended): $CSV"
[ $FAILS -eq 0 ] || { echo "$FAILS run(s) failed or had the wrong dim"; exit 1; }
