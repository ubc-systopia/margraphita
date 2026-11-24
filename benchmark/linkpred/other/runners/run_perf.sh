#!/usr/bin/env bash
# run_perf.sh — profile one linkpred benchmark run and print a summary.
#
# Usage (from the build dir, alongside the binary):
#   ./run_perf.sh NAME -- <args to fg_linkpred_bench_db>
#
# Example:
#   ./run_perf.sh pipeline_exh -- \
#       -p /drives/ssd2/flexograph_rd/kron/s12_e8 -m adj_r_s12_e8 \
#       -g adj -r --method=cn --threads=20 --topk=10 \
#       --warmup=1 --num-trials=1 --impl=pipeline --candidates=exhaustive
#
# Structural, adjacency preloaded in memory (the in-memory ceiling for cursor):
#   ./run_perf.sh csr -- \
#       -p /drives/ssd2/flexograph_rd/kron/s12_e8 -m adj_r_s12_e8 \
#       -g adj -r --method=cn --threads=20 --topk=10 \
#       --warmup=1 --num-trials=1 --variant=csr
#
# One structural reference (cursor | preloaded_ids | preloaded_adj):
#   ./run_perf.sh ref_preloaded_adj -- \
#       -p /drives/ssd2/flexograph_rd/kron/s12_e8 -m adj_r_s12_e8 \
#       -g adj -r --method=cn --topk=10 \
#       --impl=reference --variant=preloaded_adj
#
# Source: benchmark/linkpred/runners/run_perf.sh
# Copied to build/<cfg>/benchmark/linkpred/ by CMake configure_file.
#
# Env vars (all optional):
#   BIN          benchmark binary   (default: sibling fg_linkpred_bench_db)
#   OUT_DIR      output folder      (default: .../profiles/perf_clean)
#   PERF_EVENT   sampling event     (default: cpu-clock — no HW counters in WSL2)
#   PERF_FREQ    sampling rate Hz   (default: 999)
#   TOP_N        top functions shown (default: 15)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN=${BIN:-$SCRIPT_DIR/fg_linkpred_bench_db}
OUT_DIR=${OUT_DIR:-/drives/ml-pytorch-data/linkpred/flexography/profiles/perf_clean}
PERF_EVENT=${PERF_EVENT:-cpu-clock}
PERF_FREQ=${PERF_FREQ:-999}
TOP_N=${TOP_N:-15}

WARNINGS=()

# ---- usage / help ----
if [[ $# -eq 0 || "$1" == "-h" || "$1" == "--help" ]]; then
    sed -n '2,20p' "$0" | sed 's/^# \?//'
    exit 0
fi

if [[ $# -lt 3 || "$2" != "--" ]]; then
    echo "usage: $0 NAME -- <benchmark args...>" >&2
    echo "       $0 --help for full help" >&2
    exit 2
fi

NAME=$1
shift 2
BMK_ARGS=("$@")

# ---- precondition checks ----
if ! command -v perf >/dev/null 2>&1; then
    echo "ERROR: perf not found on PATH" >&2
    echo "       run this from the flexograph-perf container" >&2
    exit 2
fi

if [[ ! -x "$BIN" ]]; then
    echo "ERROR: benchmark binary not executable: $BIN" >&2
    echo "       this script expects fg_linkpred_bench_db next to it" >&2
    echo "       build it: cmake --build . --target fg_linkpred_bench_db" >&2
    echo "       or set BIN=/path/to/binary" >&2
    exit 2
fi

mkdir -p "$OUT_DIR"
cd "$OUT_DIR"

DATA="$NAME.data"
REPORT="$NAME.txt"
LOG="$NAME.log"
SUMMARY="$NAME.summary.md"

# ---- record ----
echo "[run_perf] NAME=$NAME"
echo "[run_perf] BIN=$BIN"
echo "[run_perf] event=$PERF_EVENT @ $PERF_FREQ Hz"
echo "[run_perf] recording..."

if ! perf record -e "$PERF_EVENT" -F "$PERF_FREQ" -g -o "$DATA" -- \
        "$BIN" "${BMK_ARGS[@]}" > "$LOG" 2>&1; then
    echo "ERROR: perf record failed. See $OUT_DIR/$LOG for details." >&2
    exit 2
fi

# ---- report (tolerant of failure — record+log are still useful) ----
echo "[run_perf] generating report..."

REPORT_OK=1
{
    perf report -f -i "$DATA" --no-children --sort dso,symbol --stdio 2>&1 || true
} | grep -vE "tips\.txt|Cannot load" > "$REPORT" || true

if [[ ! -s "$REPORT" ]]; then
    REPORT_OK=0
    WARNINGS+=("perf report produced an empty file — see $OUT_DIR/$REPORT and $OUT_DIR/$LOG")
fi

# Sanity: at least one % line should be present
if ! grep -qE '^[[:space:]]+[0-9]+\.[0-9]+%' "$REPORT" 2>/dev/null; then
    REPORT_OK=0
    WARNINGS+=("no '%' rows in $OUT_DIR/$REPORT — perf report may have failed silently")
fi

# ---- exact reproducible command as a single line ----
FULL_CMD="perf record -e $PERF_EVENT -F $PERF_FREQ -g -o $OUT_DIR/$DATA -- $BIN"
for arg in "${BMK_ARGS[@]}"; do
    FULL_CMD+=" $(printf '%q' "$arg")"
done

REPORT_CMD="perf report -f -i $OUT_DIR/$DATA --no-children --sort dso,symbol --stdio"

# ---- extract fields from the benchmark log (all guarded) ----
TIME=$(grep -oP 'time_ms=\K[0-9.]+'           "$LOG" | tail -1 || true)
LOAD=$(grep -oP 'load_ms=\K[0-9.]+'           "$LOG" | head -1 || true)
PAIRS=$(grep -oP 'pairs_scored=\K[0-9]+'      "$LOG" | tail -1 || true)
EDGES=$(grep -oP 'true_edges=\K[0-9]+'        "$LOG" | tail -1 || true)
WARMUP=$(grep -oP '\[warmup.*?time_ms=\K[0-9.]+' "$LOG" | tail -1 || true)
CALLER=$(grep -oP '\[main\] calling \K.*'      "$LOG" | tail -1 || true)
TIME=${TIME:-?}
LOAD=${LOAD:-?}
PAIRS=${PAIRS:-?}
EDGES=${EDGES:-?}
CALLER=${CALLER:-?}

# ---- top functions (guarded — empty if report is bad) ----
TOP_ROWS=$(
    grep -E '^[[:space:]]+[0-9]+\.[0-9]+%' "$REPORT" 2>/dev/null | head -"$TOP_N" | awk '
    {
        pct = $1
        dso = $2
        line = $0
        sub(/^.*\[[.k]\] /, "", line)
        sym = line
        if (length(sym) > 62) sym = substr(sym, 1, 59) "..."
        if (length(dso) > 22) dso = substr(dso, 1, 19) "..."
        printf "    %8s  %-22s  %s\n", pct, dso, sym
    }' || true
)
if [[ -z "$TOP_ROWS" ]]; then
    TOP_ROWS="    (no functions extracted — report is empty or unreadable)"
fi

# ---- write markdown summary ----
{
    echo "# perf summary: $NAME"
    echo
    echo "- **when**:    $(date '+%Y-%m-%d %H:%M:%S')"
    echo "- **binary**:  \`$BIN\`"
    echo "- **event**:   \`$PERF_EVENT\` @ $PERF_FREQ Hz"
    if (( ${#WARNINGS[@]} > 0 )); then
        echo "- **warnings**: ${#WARNINGS[@]} (see below)"
    fi
    echo
    echo "## Result"
    echo
    echo "| field | value |"
    echo "|---|---|"
    echo "| caller | \`$CALLER\` |"
    echo "| **time_ms** | **$TIME** |"
    if [[ -n "$WARMUP" ]]; then
        echo "| warmup_ms | $WARMUP (discarded) |"
    fi
    echo "| load_ms | $LOAD |"
    echo "| pairs_scored | $PAIRS |"
    echo "| true_edges | $EDGES |"
    echo
    if (( ${#WARNINGS[@]} > 0 )); then
        echo "## Warnings"
        echo
        for w in "${WARNINGS[@]}"; do
            echo "- $w"
        done
        echo
    fi
    echo "## Exact command (reproduce)"
    echo
    echo '```'
    echo "$FULL_CMD"
    echo '```'
    echo
    echo "## Regenerate report from data"
    echo
    echo '```'
    echo "$REPORT_CMD"
    echo '```'
    echo
    echo "## Artifacts"
    echo
    echo "| file | purpose |"
    echo "|---|---|"
    echo "| \`$OUT_DIR/$REPORT\`  | perf report |"
    echo "| \`$OUT_DIR/$DATA\`    | raw perf samples (for \`perf diff\`) |"
    echo "| \`$OUT_DIR/$LOG\`     | benchmark stdout + stderr + perf warnings |"
    echo "| \`$OUT_DIR/$SUMMARY\` | this summary |"
    echo
    echo "## Top $TOP_N functions (self time)"
    echo
    echo '```'
    echo "$TOP_ROWS"
    echo '```'
    echo
    echo "## Notes"
    echo
    echo "- \`time_ms\` is the benchmark's own \`std::chrono\` measurement of one"
    echo "  trial. It includes ~1–3% perf sampling overhead, paid equally"
    echo "  by both sides of any A/B, so deltas remain valid."
    echo "- Kernel symbols are unresolved (WSL2 restriction). Userspace"
    echo "  (WiredTiger, libc, benchmark binary) symbolicate correctly."
    echo "- \`load_ms\` is DB open + embedding preload, paid once per process."
} > "$SUMMARY"

# ---- print to terminal (always reached) ----
echo
printf '════════════════════════════════════════════════════════════════\n'
printf '  %s  ·  %s\n' "$NAME" "$(date '+%Y-%m-%d %H:%M:%S')"
printf '════════════════════════════════════════════════════════════════\n'
printf '  caller:    %s\n' "$CALLER"
printf '  time_ms:   %s\n' "$TIME"
if [[ -n "$WARMUP" ]]; then
    printf '  warmup_ms: %s  (discarded)\n' "$WARMUP"
fi
printf '  load_ms:   %s\n' "$LOAD"
printf '  pairs:     %s\n' "$PAIRS"
printf '  edges:     %s\n' "$EDGES"
printf '────────────────────────────────────────────────────────────────\n'
printf '  top %s functions (self time):\n' "$TOP_N"
printf '%s\n' "$TOP_ROWS"
if (( ${#WARNINGS[@]} > 0 )); then
    printf '────────────────────────────────────────────────────────────────\n'
    printf '  ⚠ warnings:\n'
    for w in "${WARNINGS[@]}"; do
        printf '    - %s\n' "$w"
    done
fi
printf '════════════════════════════════════════════════════════════════\n'
printf '  Files written\n'
printf '────────────────────────────────────────────────────────────────\n'
printf '    report   %s\n' "$OUT_DIR/$REPORT"
printf '    data     %s\n' "$OUT_DIR/$DATA"
printf '    log      %s\n' "$OUT_DIR/$LOG"
printf '    summary  %s\n' "$OUT_DIR/$SUMMARY"
printf '────────────────────────────────────────────────────────────────\n'
printf '  Exact command (copy-paste to reproduce)\n'
printf '────────────────────────────────────────────────────────────────\n'
printf '    %s\n' "$FULL_CMD"
printf '────────────────────────────────────────────────────────────────\n'
printf '  Regenerate report from data\n'
printf '────────────────────────────────────────────────────────────────\n'
printf '    %s\n' "$REPORT_CMD"
printf '────────────────────────────────────────────────────────────────\n'
printf '  Compare with another run\n'
printf '────────────────────────────────────────────────────────────────\n'
printf '    diff -u %s/%s.summary.md <other>.summary.md\n' "$OUT_DIR" "$NAME"
printf '    perf diff -f %s/%s.data <other>.data\n'         "$OUT_DIR" "$NAME"
printf '════════════════════════════════════════════════════════════════\n'
printf '\n'

# Exit nonzero if report generation failed (but summary already printed)
if (( REPORT_OK == 0 )); then
    exit 1
fi