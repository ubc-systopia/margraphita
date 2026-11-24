#!/usr/bin/env bash
# perf_ref.sh — profile every reference variant on one graph, then compare.
#
# Usage:
#   ./perf_ref.sh                                 # s12_e8, all 7 variants
#   GRAPH=s16_e8 ./perf_ref.sh                    # s16_e8
#   FILTER=structural ./perf_ref.sh               # only structural variants
#   FILTER=embedding  ./perf_ref.sh               # only embedding variants
#
# Output layout under $OUT_DIR (default .../profiles/ref_perf/):
#   structural/   ref_structural.{data,txt,log,summary.md}
#                 ref_structural_preloaded_ids.{data,txt,log,summary.md}
#                 ref_structural_preloaded_adj.{data,txt,log,summary.md}
#   embedded/     ref_cursor.{data,txt,log,summary.md}
#                 ref_cursor_cached.{data,txt,log,summary.md}
#                 ref_preloaded_db.{data,txt,log,summary.md}
#                 ref_pvector.{data,txt,log,summary.md}
#   comparison.txt   — the side-by-side table + top-5 per variant
#
# Source: benchmark/linkpred/runners/perf_ref.sh
# Copied to build/<cfg>/benchmark/linkpred/ by CMake configure_file.
#
# Env vars (all optional):
#   GRAPH        graph name suffix      (default: s12_e8)
#   DB_DIR_BASE  parent of graph dirs   (default: /drives/ssd2/flexograph_rd/kron)
#   REF_BIN      reference binary       (default: sibling fg_linkpred_bench_reference)
#   OUT_DIR      output root folder     (default: .../profiles/ref_perf)
#   TOP_N        top functions per var  (default: 10)
#   FILTER       all | structural | embedding  (default: all)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REF_BIN=${REF_BIN:-$SCRIPT_DIR/fg_linkpred_bench_reference}
GRAPH=${GRAPH:-s12_e8}
DB_DIR_BASE=${DB_DIR_BASE:-/drives/ssd2/flexograph_rd/kron}
OUT_DIR=${OUT_DIR:-/drives/ml-pytorch-data/linkpred/flexography/profiles/ref_perf}
TOP_N=${TOP_N:-10}
FILTER=${FILTER:-all}

DB="-p ${DB_DIR_BASE}/${GRAPH} -m adj_r_${GRAPH} -g adj -r --topk=10"

STRUCT_VARIANTS=(structural structural_preloaded_ids structural_preloaded_adj)
EMB_VARIANTS=(cursor cursor_cached preloaded_db pvector)

FAILED=()

# ---- precondition ----
if ! command -v perf >/dev/null 2>&1; then
    echo "ERROR: perf not on PATH (use the flexograph-perf container)" >&2
    exit 2
fi
if [[ ! -x "$REF_BIN" ]]; then
    echo "ERROR: reference binary not executable: $REF_BIN" >&2
    echo "       build it: cmake --build . --target fg_linkpred_bench_reference" >&2
    echo "       or set REF_BIN=/path/to/binary" >&2
    exit 2
fi

# ---- layout ----
mkdir -p "$OUT_DIR/structural" "$OUT_DIR/embedded"

variant_dir() {
    case "$1" in
        structural*) echo "$OUT_DIR/structural" ;;
        *)           echo "$OUT_DIR/embedded"   ;;
    esac
}

variant_method() {
    case "$1" in
        structural*) echo "cn"     ;;
        *)           echo "cosine" ;;
    esac
}

# ---- helpers ----

run_one() {
    local variant=$1
    local method=$(variant_method "$variant")
    local dir=$(variant_dir "$variant")
    local name="ref_${variant}"
    local data="$dir/$name.data"
    local report="$dir/$name.txt"
    local log="$dir/$name.log"

    printf '  → %-22s (%s)\n' "$name" "$method"

    if ! perf record -e cpu-clock -F 999 -g -o "$data" -- \
            "$REF_BIN" $DB --method="$method" --ref-variant="$variant" \
            > "$log" 2>&1; then
        echo "    ! record failed"
        FAILED+=("$name")
        return 1
    fi

    {
        perf report -f -i "$data" --no-children --sort dso,symbol --stdio 2>&1 || true
    } | grep -vE "tips\.txt|Cannot load" > "$report" || true

    return 0
}

# Extract (variant, pairs, edges, time_ms) from reference-runner table output.
# Row format: "cursor  8366095  32447  10  2245.7  OK"
extract_row() {
    awk '/^[a-z_]+[ \t]+[0-9]+[ \t]+[0-9]+[ \t]+[0-9]+[ \t]+[0-9.]+[ \t]+OK/ {
        print $1, $2, $3, $5
    }' "$1" | tail -1
}

# Extract top N functions from a perf report file.
extract_top() {
    local report=$1
    local n=${2:-10}
    grep -E '^[[:space:]]+[0-9]+\.[0-9]+%' "$report" 2>/dev/null | head -"$n" | awk '
    {
        pct = $1; dso = $2; line = $0
        sub(/^.*\[[.k]\] /, "", line)
        sym = line
        if (length(sym) > 55) sym = substr(sym, 1, 52) "..."
        if (length(dso) > 20) dso = substr(dso, 1, 17) "..."
        printf "    %8s  %-20s  %s\n", pct, dso, sym
    }' || true
}

# ---- header ----
echo "════════════════════════════════════════════════════════════════"
echo "  perf_ref.sh — reference variants on $GRAPH"
echo "════════════════════════════════════════════════════════════════"
echo "  BIN:     $REF_BIN"
echo "  OUT_DIR: $OUT_DIR"
echo "  graph:   $GRAPH"
echo "  filter:  $FILTER"
echo "════════════════════════════════════════════════════════════════"
echo

# ---- structural ----
if [[ "$FILTER" == "all" || "$FILTER" == "structural" ]]; then
    echo "[structural, --method=cn]  →  $OUT_DIR/structural/"
    for v in "${STRUCT_VARIANTS[@]}"; do
        run_one "$v"
    done
    echo
fi

# ---- embedding ----
if [[ "$FILTER" == "all" || "$FILTER" == "embedding" ]]; then
    echo "[embedding, --method=cosine]  →  $OUT_DIR/embedded/"
    for v in "${EMB_VARIANTS[@]}"; do
        run_one "$v"
    done
    echo
fi

# ---- per-variant summary .md files (inside their subfolders) ----
for variant in "${STRUCT_VARIANTS[@]}" "${EMB_VARIANTS[@]}"; do
    dir=$(variant_dir "$variant")
    name="ref_${variant}"
    log="$dir/$name.log"
    report="$dir/$name.txt"
    summary="$dir/$name.summary.md"

    [[ -f "$log" ]] || continue

    row=$(extract_row "$log")
    if [[ -n "$row" ]]; then
        read -r _vname pairs edges time_ms <<< "$row"
    else
        pairs="?"; edges="?"; time_ms="?"
    fi

    top=$(extract_top "$report" "$TOP_N")

    {
        echo "# perf summary: $name"
        echo
        echo "- **when**:    $(date '+%Y-%m-%d %H:%M:%S')"
        echo "- **variant**: \`$variant\`"
        echo "- **method**:  \`$(variant_method "$variant")\`"
        echo "- **graph**:   \`$GRAPH\`"
        echo "- **binary**:  \`$REF_BIN\`"
        echo
        echo "## Result"
        echo
        echo "| field | value |"
        echo "|---|---|"
        echo "| **time_ms** | **$time_ms** |"
        echo "| pairs_scored | $pairs |"
        echo "| true_edges | $edges |"
        echo
        echo "## Artifacts"
        echo
        echo "| file | purpose |"
        echo "|---|---|"
        echo "| \`$report\`  | perf report |"
        echo "| \`$dir/$name.data\` | raw samples |"
        echo "| \`$log\`     | benchmark output |"
        echo "| \`$summary\` | this summary |"
        echo
        echo "## Top $TOP_N functions (self time)"
        echo
        echo '```'
        echo "$top"
        echo '```'
    } > "$summary"
done

# ---- build comparison block once, reuse for terminal + file ----
comparison_block() {
    echo "════════════════════════════════════════════════════════════════"
    echo "  COMPARISON — $GRAPH"
    echo "════════════════════════════════════════════════════════════════"
    printf "  %-22s %12s %12s %12s  %s\n" "variant" "pairs" "edges" "time_ms" "folder"
    printf "  %-22s %12s %12s %12s  %s\n" "-------" "-----" "-----" "-------" "------"

    for variant in "${STRUCT_VARIANTS[@]}" "${EMB_VARIANTS[@]}"; do
        dir=$(variant_dir "$variant")
        log="$dir/ref_${variant}.log"
        [[ -f "$log" ]] || continue
        row=$(extract_row "$log")
        short_dir=$(basename "$dir")
        if [[ -n "$row" ]]; then
            read -r _vname pairs edges time_ms <<< "$row"
            printf "  %-22s %12s %12s %12s  %s\n" "$variant" "$pairs" "$edges" "$time_ms" "$short_dir"
        else
            printf "  %-22s %12s %12s %12s  %s\n" "$variant" "?" "?" "?" "$short_dir"
        fi
    done

    echo
    echo "════════════════════════════════════════════════════════════════"
    echo "  TOP 5 FUNCTIONS PER VARIANT"
    echo "════════════════════════════════════════════════════════════════"

    for variant in "${STRUCT_VARIANTS[@]}" "${EMB_VARIANTS[@]}"; do
        dir=$(variant_dir "$variant")
        report="$dir/ref_${variant}.txt"
        [[ -f "$report" ]] || continue
        echo
        echo "  $variant ($(basename "$dir")):"
        extract_top "$report" 5
    done

    echo
    echo "════════════════════════════════════════════════════════════════"
    echo "  OUTPUT LAYOUT"
    echo "════════════════════════════════════════════════════════════════"
    echo "  $OUT_DIR/structural/"
    for v in "${STRUCT_VARIANTS[@]}"; do echo "    ref_${v}.{data,txt,log,summary.md}"; done
    echo "  $OUT_DIR/embedded/"
    for v in "${EMB_VARIANTS[@]}"; do echo "    ref_${v}.{data,txt,log,summary.md}"; done
    echo "  $OUT_DIR/comparison.txt"
    echo "════════════════════════════════════════════════════════════════"
}

# ---- write comparison.txt + print to terminal ----
comparison_block | tee "$OUT_DIR/comparison.txt"

# ---- failure report ----
if (( ${#FAILED[@]} > 0 )); then
    echo
    echo "════════════════════════════════════════════════════════════════"
    echo "  FAILED: ${FAILED[*]}"
    echo "════════════════════════════════════════════════════════════════"
    exit 1
fi

echo
echo "  done → $OUT_DIR"