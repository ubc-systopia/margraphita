#!/usr/bin/env bash
# run_microbench_sweep.sh — full microbenchmark sweep over embedding dims.
#
# For each dim in DIMS:
#   1. load FastRP <dim> into every kron DB (add_kron_emb.sh: adj + split_ekey;
#      each DB holds one embedding at a time, so this overwrites the last one).
#      Always loads, even if bench_params.json already says <dim>.
#   2. run_next_vs_cosine.sh        (single-thread costs)
#   3. run_fetch_random_parallel.sh (random fetch with 1..T threads)
# then reload RESTORE_DIM (default 16, what the linkpred runs expect).
#
# Each run reads the embeddings from the DB it opens (dim probed from the stored
# blobs, cosine scored on those same embeddings); runs are checked against the
# swept dim and any mismatch is reported as DIM MISMATCH and fails the sweep.
#
# Copied to build/<cfg>/benchmark/linkpred/ by CMake (with the two runners and
# READMEs), next to the binaries. Run from there:
#
# Usage (inside the flexograph-perf container, nothing else using the DBs):
#   ./run_microbench_sweep.sh                       # all graphs, dims 16 32 64 128
#   ./run_microbench_sweep.sh s12_e8 s20_e8         # chosen graphs
#   CLEAN=1 ./run_microbench_sweep.sh               # delete old results/logs first
#
# Env vars (all optional):
#   DIMS         dims to sweep, in order        (default: "16 32 64 128")
#   RESTORE_DIM  dim to reload at the end       (default: 16; "" = leave as is)
#   CLEAN        1 = delete results.csv + logs/ in both output folders first
#   LOAD_DIR     folder with add_kron_emb.sh + bench_params.json
#                                               (default: build/release/test)
#   BIN_DIR      folder with next_vs_cosine + fetch_random_parallel
#                                               (default: this script's folder; CMake
#                                               copies it next to the binaries)
#   OUT_ROOT     parent of the two output folders
#                                               (default: .../linkpred/flexography/microbench)
#   GT, ROUNDS, CALLS, WT_CACHE, THREADS        passed to the two runners
#
# Output: OUT_ROOT/next_vs_cosine/ and OUT_ROOT/fetch_random_parallel/ (each
# with README.md, results.csv, logs/), plus OUT_ROOT/sweep_<timestamp>.log.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO=/FlexoGraph/pull-property/margraphita
DIMS=${DIMS:-"16 32 64 128"}
RESTORE_DIM=${RESTORE_DIM-16}
CLEAN=${CLEAN:-0}
LOAD_DIR=${LOAD_DIR:-$REPO/build/release/test}
BIN_DIR=${BIN_DIR:-$SCRIPT_DIR}
OUT_ROOT=${OUT_ROOT:-/drives/ml-pytorch-data/linkpred/flexography/microbench}
GRAPHS=("$@")

PARAMS=$LOAD_DIR/bench_params.json
NVC_OUT=$OUT_ROOT/next_vs_cosine
FRP_OUT=$OUT_ROOT/fetch_random_parallel

# ---- preconditions ----
for f in "$LOAD_DIR/add_kron_emb.sh" "$PARAMS" "$BIN_DIR/next_vs_cosine" "$BIN_DIR/fetch_random_parallel" \
         "$SCRIPT_DIR/run_next_vs_cosine.sh" "$SCRIPT_DIR/run_fetch_random_parallel.sh"; do
    [ -e "$f" ] || { echo "ERROR: missing $f" >&2; exit 2; }
done
# Match process names (comm, max 15 chars), not command lines, so shells that
# merely mention these names don't count.
BUSY='^(fg_linkpred_ben|next_vs_cosine|fetch_random_pa|add_pytorch_emb)'
if pgrep "$BUSY" >/dev/null; then
    echo "ERROR: another benchmark / loader is running (it may hold a DB lock):" >&2
    ps -o pid,stat,etime,args -p "$(pgrep -d, "$BUSY")" >&2
    exit 2
fi

mkdir -p "$NVC_OUT/logs" "$FRP_OUT/logs"
SWEEP_LOG=$OUT_ROOT/sweep_$(date +%Y%m%d_%H%M%S).log
exec > >(tee -a "$SWEEP_LOG") 2>&1          # everything below also goes to the sweep log

if [ "$CLEAN" = "1" ]; then
    echo "[clean] removing old results.csv and logs/ in $NVC_OUT and $FRP_OUT"
    rm -f "$NVC_OUT/results.csv" "$NVC_OUT"/logs/*.log "$FRP_OUT/results.csv" "$FRP_OUT"/logs/*.log
fi
# READMEs explain each output folder; refresh them from the source tree.
cp "$SCRIPT_DIR/next_vs_cosine_output_README.md"        "$NVC_OUT/README.md"
cp "$SCRIPT_DIR/fetch_random_parallel_output_README.md" "$FRP_OUT/README.md"

set_dim() { sed -i "s/\"dim\": [0-9]*/\"dim\": $1/" "$PARAMS"; }
cur_dim() { grep -oE '"dim": [0-9]+' "$PARAMS" | grep -oE '[0-9]+'; }

# Always load, even if bench_params.json already says this dim: the DBs may not
# match the config (interrupted sweep, hand loads), and a load takes seconds.
load_dim() {
    set_dim "$1"
    bash "$LOAD_DIR/add_kron_emb.sh"
}

BAD=()
echo "sweep: dims=[$DIMS] graphs=[${GRAPHS[*]:-all}] restore=${RESTORE_DIM:-none} $(date)"
for d in $DIMS; do
    echo ""
    echo "################ dim $d  $(date)"
    load_dim "$d" || { echo "[FAIL] load dim $d — skipping its runs"; continue; }
    # EXPECT_DIM: each run reads the dim from the DB it opened; a mismatch fails.
    EXPECT_DIM=$d BIN=$BIN_DIR/next_vs_cosine        LOG_ROOT=$NVC_OUT bash "$SCRIPT_DIR/run_next_vs_cosine.sh"        "${GRAPHS[@]}" || BAD+=("next_vs_cosine dim $d")
    EXPECT_DIM=$d BIN=$BIN_DIR/fetch_random_parallel LOG_ROOT=$FRP_OUT bash "$SCRIPT_DIR/run_fetch_random_parallel.sh" "${GRAPHS[@]}" || BAD+=("fetch_random_parallel dim $d")
done

if [ -n "$RESTORE_DIM" ]; then
    echo ""
    echo "################ restore dim $RESTORE_DIM  $(date)"
    load_dim "$RESTORE_DIM"
fi

echo ""
echo "################ done $(date)"
echo "  next_vs_cosine:        $NVC_OUT"
echo "  fetch_random_parallel: $FRP_OUT"
echo "  sweep log:             $SWEEP_LOG"
if [ ${#BAD[@]} -gt 0 ]; then
    echo "  PROBLEMS (failed runs or DIM MISMATCH; grep the sweep log):"
    printf '    %s\n' "${BAD[@]}"
    exit 1
fi
echo "  all runs OK, every DB held the expected dim"
