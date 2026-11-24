#!/usr/bin/env bash
set -euo pipefail
# Usage: fastrp_sweep.sh SCRATCH_DB_DIR DB_NAME [undirected|directed]
[[ $# -ge 2 && $# -le 3 ]] || { echo "Usage: $0 SCRATCH_DB_DIR DB_NAME [undirected|directed]" >&2; exit 2; }
DB_DIR=$(realpath "$1"); DB_NAME=$2; DIRECTION=${3:-undirected}
[[ -d "$DB_DIR" && "$DB_NAME" != */* && -n "$DB_NAME" ]] || exit 2
case "$DIRECTION" in directed) direction=(-d);; undirected) direction=();; *) exit 2;; esac
: "${BIN:=./build/release/benchmark/linkpred/fg_fastrp}"
: "${DIM:=128}" "${ITERS:=4}" "${THREADS:=4}" "${REPS:=3}" "${SEED:=42}" "${DENSITY:=0.01}" "${ADJ:=cursor}" "${VARIANT:=orig}"
: "${MODES:=none after fused}"
[[ -x "$BIN" ]] || { echo "Executable not found: $BIN" >&2; exit 2; }
# Explicit opt-in: the driver overwrites existing vertex property blobs.
if [[ " $MODES " == *' after '* || " $MODES " == *' fused '* ]]; then
  [[ ${ALLOW_SCRATCH_WRITES:-0} == 1 ]] || { echo 'Use a closed, disposable DB copy and set ALLOW_SCRATCH_WRITES=1.' >&2; exit 2; }
fi
OUT=${OUT:-benchmark/linkpred/output/sweeps/fastrp/$(date +%Y%m%d_%H%M%S)_$$}
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
printf 'db,mode,rep,dim,iters,threads,seed,density,adj,variant,direction,total_s,peak_rss_mb,write_s,exit_code\n' > "$OUT/results.csv"
for ((rep=1;rep<=REPS;rep++)); do
  for mode in $MODES; do
    stem="$OUT/${mode}_${rep}"
    # Export in every mode for comparable end-to-end measurements.
    case "$mode" in none) write=();; after|fused) write=("--write-db=$mode");; *) echo "Unknown mode: $mode" >&2; exit 2;; esac
    cmd=("$BIN" -p "$DB_DIR" -m "$DB_NAME" -g adj -r "${direction[@]}" "--dim=$DIM" "--iters=$ITERS" "--threads=$THREADS" "--seed=$SEED" "--density=$DENSITY" "--adj-read=$ADJ" "--variant=$VARIANT" "--out=$stem.npy" "${write[@]}")
    printf '%q ' "${cmd[@]}" > "$stem.command"; printf '\n' >> "$stem.command"
    echo "Running $DB_NAME $mode repetition $rep"
    rc=0; "${cmd[@]}" > "$stem.log" 2>&1 || rc=$?
    metric() { grep -oE "(^|[[:space:]])$1=[0-9.eE+-]+" "$stem.log" | tail -1 | cut -d= -f2 || true; }
    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' "$DB_NAME" "$mode" "$rep" "$DIM" "$ITERS" "$THREADS" "$SEED" "$DENSITY" "$ADJ" "$VARIANT" "$DIRECTION" "$(metric total_s)" "$(metric peak_rss_mb)" "$(metric write_s)" "$rc" >> "$OUT/results.csv"
    if ((rc != 0)); then echo "Failed: $stem.log" >&2; exit "$rc"; fi
    tail -1 "$stem.log"
    # Keep only the latest export per mode unless explicitly requested.
    if [[ ${KEEP_NPY:-0} != 1 && $rep -lt $REPS ]]; then rm -f "$stem.npy"; fi
  done
done
echo "Results: $OUT/results.csv"