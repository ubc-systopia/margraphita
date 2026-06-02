#!/bin/bash
# Run FlexoGraph Fig 7A-B benchmark (individual operation latencies).
#
# Usage: ./fg_fig7.sh <dataset_alias>
# Output: 4 lines matching reproduce_script.sh awk patterns:
#   get avg: <X> us
#   addv avg: <Y> us
#   adde avg: <Z> us
#   dele avg: <W> us

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FG_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${FG_BUILD_DIR:-$FG_ROOT/build/preprocess_aster}"
BENCH_BIN="$BUILD_DIR/benchmark/aster/fg_aster_bench"
DB_DIR="$BUILD_DIR/aster_dbs"

DATASET="${1:?Usage: $0 <dataset_alias>}"
GRAPH_TYPE="${2:-adj}"

case "$DATASET" in
  cit-patents) DB_NAME="${GRAPH_TYPE}_rd_${DATASET}" ;;
  *)           DB_NAME="${GRAPH_TYPE}_r_${DATASET}" ;;
esac

$BENCH_BIN -p "$DB_DIR" -m "$DB_NAME" -g "$GRAPH_TYPE" -r \
  --mode=fig7a 2>/dev/null | grep -E '^(get|addv|adde|dele) avg:'
