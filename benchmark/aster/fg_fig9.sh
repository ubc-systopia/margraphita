#!/bin/bash
# Run FlexoGraph Fig 9 benchmark (in-memory workload comparison).
#
# Usage: ./fg_fig9.sh
# Output: get/add latencies for wikitalk and dblp at 50:50 ratio.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FG_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${FG_BUILD_DIR:-$FG_ROOT/build/preprocess_aster}"
BENCH_BIN="$BUILD_DIR/benchmark/aster/fg_aster_bench"
DB_DIR="$BUILD_DIR/aster_dbs"

ROPS=50000
WOPS=50000

for DATASET in wikitalk dblp; do
  case "$DATASET" in
    wikitalk) DB_NAME="adj_r_wikitalk" ; LABEL="WikiTalk" ;;
    dblp)     DB_NAME="adj_r_dblp"     ; LABEL="DBLP" ;;
  esac

  echo "${LABEL}:"
  $BENCH_BIN -p "$DB_DIR" -m "$DB_NAME" -g adj -r \
    --mode=fig6 --rops=$ROPS --wops=$WOPS 2>/dev/null
done
