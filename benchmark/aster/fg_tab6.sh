#!/bin/bash
# Run FlexoGraph Table 6 benchmark (graph algorithm runtimes).
#
# Usage: ./fg_tab6.sh <dataset_alias>
# Output: algorithm runtimes matching reproduce_script.sh format

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FG_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${FG_BUILD_DIR:-$FG_ROOT/build/preprocess_aster}"
DB_DIR="$BUILD_DIR/aster_dbs"

DATASET="${1:?Usage: $0 <dataset_alias>}"
GRAPH_TYPE="${2:-adj}"

# For directed datasets (cit-patents), use rd suffix
case "$DATASET" in
  cit-patents) DB_NAME="${GRAPH_TYPE}_rd_${DATASET}" ; DIRECTED=true ;;
  *)           DB_NAME="${GRAPH_TYPE}_r_${DATASET}"  ; DIRECTED=false ;;
esac

COMMON_FLAGS="-p $DB_DIR -m $DB_NAME -g $GRAPH_TYPE -r"

# PageRank (pr_vc)
echo "=== PageRank ==="
$BUILD_DIR/benchmark/pr_vc $COMMON_FLAGS 2>/dev/null

# BFS (bfs_parallel)
echo "=== BFS ==="
$BUILD_DIR/benchmark/bfs_parallel $COMMON_FLAGS 2>/dev/null

# WCC (cc_parallel)
echo "=== WCC ==="
$BUILD_DIR/benchmark/cc_parallel $COMMON_FLAGS 2>/dev/null

# SSSP (sssp_parallel)
echo "=== SSSP ==="
$BUILD_DIR/benchmark/sssp_parallel $COMMON_FLAGS 2>/dev/null

# CDLP (fg_cdlp)
echo "=== CDLP ==="
$BUILD_DIR/benchmark/aster/fg_cdlp $COMMON_FLAGS 2>/dev/null
