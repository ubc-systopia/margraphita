#!/bin/bash
# Run FlexoGraph Fig 7C-D benchmark (property CRUD operations).
#
# Usage: ./fg_fig7_property.sh <dataset_alias>
#   dataset_alias: ldbc or freebase
#
# Prerequisite: preprocessed .vertex/.edge files must exist in
#   $ASTER_ROOT/AsterDB/dataset/ (run AsterDB's process_property_*.py first)
#
# Output: 8 lines matching reproduce_script.sh awk patterns:
#   Time of vertex property search: <N>ns
#   Time of edge property search: <N>ns
#   Time of update vertex property: <N>ns
#   Time of update edge property: <N>ns
#   Time of insert vertex property: <N>ns
#   Time of insert edge property: <N>ns
#   Time of remove vertex property: <N>ns
#   Time of remove edge property: <N>ns

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FG_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${FG_BUILD_DIR:-$FG_ROOT/build/preprocess_aster}"
BENCH_BIN="$BUILD_DIR/benchmark/aster/fg_aster_props"

# Aster artifact root (parent of flexograph/)
ASTER_ROOT="$(cd "$FG_ROOT/.." && pwd)"
DATA_DIR="$ASTER_ROOT/AsterDB/dataset"
DB_DIR="$BUILD_DIR/aster_props_dbs"

DATASET="${1:?Usage: $0 <dataset_alias>}"

mkdir -p "$DB_DIR"

$BENCH_BIN --data_dir="$DATA_DIR" --dataset="$DATASET" \
  --db_dir="$DB_DIR/fg_props_${DATASET}" 2>/dev/null \
  | grep -E '^Time of'
