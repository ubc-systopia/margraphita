#!/bin/bash
# Run FlexoGraph in-memory workload benchmark (50:50 get/add on wikitalk + dblp).
#
# Usage: ./fg_inmemory.sh [graph_type]
# Output: get/add latencies for wikitalk and dblp at 50:50 ratio.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FG_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${FG_BUILD_DIR:-$FG_ROOT/build/preprocess_aster}"
BENCH_BIN="$BUILD_DIR/benchmark/aster/fg_structural_bench"
DB_DIR="$BUILD_DIR/aster_dbs"

GRAPH_TYPE="${1:-adj}"
ROPS=50000
WOPS=50000

for DATASET in wikitalk dblp; do
  case "$DATASET" in
    wikitalk) DB_NAME="${GRAPH_TYPE}_r_wikitalk" ; LABEL="WikiTalk" ;;
    dblp)     DB_NAME="${GRAPH_TYPE}_r_dblp"     ; LABEL="DBLP" ;;
  esac

  echo "${LABEL}:"
  $BENCH_BIN -p "$DB_DIR" -m "$DB_NAME" -g "$GRAPH_TYPE" -r \
    --mode=fig6 --rops=$ROPS --wops=$WOPS 2>/dev/null
done
