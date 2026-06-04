#!/bin/bash
# Run FlexoGraph throughput benchmark (mixed get+add at different R/W ratios).
#
# Usage: ./fg_throughput.sh <dataset_alias> [graph_type]
# Output: one line per ratio config: "flexograph-<graph_type>,<get_avg_us>,<add_avg_us>"

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FG_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${FG_BUILD_DIR:-$FG_ROOT/build/preprocess_aster}"
BENCH_BIN="$BUILD_DIR/benchmark/aster/fg_structural_bench"
DB_DIR="$BUILD_DIR/aster_dbs"

DATASET="${1:?Usage: $0 <dataset_alias>}"
GRAPH_TYPE="${2:-adj}"

case "$DATASET" in
  cit-patents|twitter) DB_NAME="${GRAPH_TYPE}_rd_${DATASET}" ;;
  *)                   DB_NAME="${GRAPH_TYPE}_r_${DATASET}" ;;
esac

# 9 ratio configs matching reproduce_script.sh gen_figure_6
RATIOS="10:90 20:80 30:70 40:60 50:50 60:40 70:30 80:20 90:10"

for ratio in $RATIOS; do
  ROPS=$(( ${ratio%%:*} * 10000 ))
  WOPS=$(( ${ratio##*:} * 10000 ))
  OUTPUT=$($BENCH_BIN -p "$DB_DIR" -m "$DB_NAME" -g "$GRAPH_TYPE" -r \
    --mode=fig6 --rops=$ROPS --wops=$WOPS 2>/dev/null)
  GET=$(echo "$OUTPUT" | grep -oP 'get:\s*\K[0-9.eE+-]+')
  ADD=$(echo "$OUTPUT" | grep -oP 'add:\s*\K[0-9.eE+-]+')
  LABEL="flexograph-${GRAPH_TYPE/split_ekey/ekey}"
  echo "$LABEL,$GET,$ADD"
done
