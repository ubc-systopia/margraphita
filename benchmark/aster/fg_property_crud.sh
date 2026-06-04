#!/bin/bash
# Run FlexoGraph property CRUD benchmark (search/update/insert/remove × vertex/edge).
#
# Usage: ./fg_property_crud.sh <dataset_alias> [graph_type]
#   dataset_alias: ldbc or freebase
#   graph_type: adj (default) or split_ekey
#
# Output: 8 lines matching reproduce_script.sh awk patterns:
#   Time of vertex property search: <N>ns
#   ...

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FG_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${FG_BUILD_DIR:-$FG_ROOT/build/preprocess_aster}"
BENCH_BIN="$BUILD_DIR/benchmark/aster/fg_property_bench"

# Aster artifact root (parent of flexograph/)
ASTER_ROOT="$(cd "$FG_ROOT/.." && pwd)"
DATA_DIR="$ASTER_ROOT/AsterDB/dataset"
DB_DIR="$BUILD_DIR/aster_props_dbs"

DATASET="${1:?Usage: $0 <dataset_alias> [graph_type]}"
GRAPH_TYPE="${2:-adj}"

mkdir -p "$DB_DIR"

DB_NAME="fg_props_${GRAPH_TYPE}_${DATASET}"

$BENCH_BIN -g "$GRAPH_TYPE" -p "$DB_DIR" -m "$DB_NAME" \
  --data_dir="$DATA_DIR" --dataset="$DATASET" 2>/dev/null \
  | grep -E '^Time of'
