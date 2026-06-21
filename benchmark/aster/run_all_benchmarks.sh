#!/bin/bash
# Run all FlexoGraph benchmarks across all datasets and graph types.
#
# Copies the pristine DB to a temp location before mutating benchmarks
# (throughput, structural latency, in-memory), so the original DB stays clean.
#
# Usage: ./run_all_benchmarks.sh [results_dir]
#
# Results are written to <results_dir>/flexograph_*.log

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FG_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${FG_BUILD_DIR:-$FG_ROOT/build/preprocess_aster}"
ASTER_ROOT="$(cd "$FG_ROOT/.." && pwd)"
DB_DIR="${FG_DB_DIR:-$ASTER_ROOT/flexograph_dbs}"
RESULTS_DIR="${1:-$ASTER_ROOT/results/flexograph}"

mkdir -p "$RESULTS_DIR"

# Temp dir for DB copies (mutating benchmarks work on copies)
TMPDB_DIR=$(mktemp -d "${BUILD_DIR}/tmp_bench_dbs.XXXXXX")
echo "[INFO] Temp DB dir: $TMPDB_DIR"

cleanup() {
  echo "[INFO] Cleaning up temp DBs..."
  rm -rf "$TMPDB_DIR"
}
trap cleanup EXIT

# ── Helper: copy pristine DB to temp dir ─────────────────────────────────────
# Usage: copy_db <db_name>
# Sets FG_BUILD_DIR so the shell wrappers use the temp copy.
copy_db() {
  local db_name="$1"
  local src="$DB_DIR/$db_name"
  local dst="$TMPDB_DIR/aster_dbs/$db_name"
  if [ ! -d "$src" ]; then
    echo "[WARN] DB not found: $src — skipping"
    return 1
  fi
  echo "[INFO] Copying $db_name to temp dir..."
  mkdir -p "$TMPDB_DIR/aster_dbs"
  rm -rf "$dst"
  cp -r "$src" "$dst"
  return 0
}

# ── Helper: resolve DB name ──────────────────────────────────────────────────
db_name() {
  local ds="$1" gt="$2"
  case "$ds" in
    dblp) echo "${gt}_r_${ds}" ;;
    *)    echo "${gt}_rd_${ds}" ;;
  esac
}

# ── Run order ────────────────────────────────────────────────────────────────
# 1. Algorithms (read-only, safe on pristine DB)
# 2. Mutating benchmarks on temp copies (throughput, structural, in-memory)

GRAPH_TYPES=(adj split_ekey)

# ═══════════════════════════════════════════════════════════════════════════════
# Phase 1: Algorithms (non-mutating) — run on pristine DBs
# ═══════════════════════════════════════════════════════════════════════════════
echo ""
echo "================================================================"
echo "  Phase 1: Graph Algorithms (non-mutating)"
echo "================================================================"

# Table 6: cit-patents + wikitalk
for ds in wikitalk cit-patents; do
  for gt in "${GRAPH_TYPES[@]}"; do
    dbn=$(db_name "$ds" "$gt")
    if [ ! -d "$DB_DIR/$dbn" ]; then
      echo "[WARN] Skipping algorithms $ds/$gt — DB not found"
      continue
    fi
    gt_label="${gt/split_ekey/ekey}"
    logfile="$RESULTS_DIR/algorithms_${ds}_${gt_label}.log"
    echo ""
    echo "--- Algorithms: $ds ($gt) ---"
    bash "$SCRIPT_DIR/fg_algorithms.sh" "$ds" "$gt" 2>&1 | tee "$logfile"
  done
done

# Also run algorithms on dblp (for Table 6 / Fig 9)
for gt in "${GRAPH_TYPES[@]}"; do
  dbn=$(db_name "dblp" "$gt")
  if [ ! -d "$DB_DIR/$dbn" ]; then
    echo "[WARN] Skipping algorithms dblp/$gt — DB not found"
    continue
  fi
  gt_label="${gt/split_ekey/ekey}"
  logfile="$RESULTS_DIR/algorithms_dblp_${gt_label}.log"
  echo ""
  echo "--- Algorithms: dblp ($gt) ---"
  bash "$SCRIPT_DIR/fg_algorithms.sh" "dblp" "$gt" 2>&1 | tee "$logfile"
done

# ═══════════════════════════════════════════════════════════════════════════════
# Phase 2: Structural latency (mutating) — run on temp DB copies
# ═══════════════════════════════════════════════════════════════════════════════
echo ""
echo "================================================================"
echo "  Phase 2: Structural Latency (mutating — uses temp DB copies)"
echo "================================================================"

# Fig 7A-B: dblp + wikipedia
for ds in dblp wikipedia; do
  for gt in "${GRAPH_TYPES[@]}"; do
    dbn=$(db_name "$ds" "$gt")
    if ! copy_db "$dbn"; then continue; fi
    gt_label="${gt/split_ekey/ekey}"
    logfile="$RESULTS_DIR/structural_${ds}_${gt_label}.log"
    echo ""
    echo "--- Structural latency: $ds ($gt) ---"
    FG_DB_DIR="$TMPDB_DIR/aster_dbs" bash "$SCRIPT_DIR/fg_structural_latency.sh" "$ds" "$gt" 2>&1 | tee "$logfile"
    rm -rf "$TMPDB_DIR/aster_dbs/$dbn"
  done
done

# ═══════════════════════════════════════════════════════════════════════════════
# Phase 3: Throughput (mutating) — run on temp DB copies
# ═══════════════════════════════════════════════════════════════════════════════
echo ""
echo "================================================================"
echo "  Phase 3: Throughput (mutating — uses temp DB copies)"
echo "================================================================"

# Fig 6: dblp + wikipedia
for ds in dblp wikipedia; do
  for gt in "${GRAPH_TYPES[@]}"; do
    dbn=$(db_name "$ds" "$gt")
    if ! copy_db "$dbn"; then continue; fi
    gt_label="${gt/split_ekey/ekey}"
    logfile="$RESULTS_DIR/throughput_${ds}_${gt_label}.log"
    echo ""
    echo "--- Throughput: $ds ($gt) ---"
    FG_DB_DIR="$TMPDB_DIR/aster_dbs" bash "$SCRIPT_DIR/fg_throughput.sh" "$ds" "$gt" 2>&1 | tee "$logfile"
    rm -rf "$TMPDB_DIR/aster_dbs/$dbn"
  done
done

# ═══════════════════════════════════════════════════════════════════════════════
# Phase 4: In-memory (mutating) — run on temp DB copies
# ═══════════════════════════════════════════════════════════════════════════════
echo ""
echo "================================================================"
echo "  Phase 4: In-memory (mutating — uses temp DB copies)"
echo "================================================================"

# Fig 9: wikitalk + dblp (fg_inmemory.sh runs both internally)
for gt in "${GRAPH_TYPES[@]}"; do
  # Need both wikitalk and dblp DBs available
  dbn_wt=$(db_name "wikitalk" "$gt")
  dbn_db=$(db_name "dblp" "$gt")
  copy_db "$dbn_wt" || true
  copy_db "$dbn_db" || true
  gt_label="${gt/split_ekey/ekey}"
  logfile="$RESULTS_DIR/inmemory_${gt_label}.log"
  echo ""
  echo "--- In-memory: wikitalk + dblp ($gt) ---"
  FG_DB_DIR="$TMPDB_DIR/aster_dbs" bash "$SCRIPT_DIR/fg_inmemory.sh" "$gt" 2>&1 | tee "$logfile"
  rm -rf "$TMPDB_DIR/aster_dbs/$dbn_wt" "$TMPDB_DIR/aster_dbs/$dbn_db"
done

echo ""
echo "================================================================"
echo "  All benchmarks complete. Results in: $RESULTS_DIR"
echo "================================================================"
ls -la "$RESULTS_DIR"/*.log
