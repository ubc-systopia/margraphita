#!/bin/bash
# Run FlexoGraph property CRUD benchmark (search/update/insert/remove × vertex/edge).
#
# Usage: ./fg_property_crud.sh <dataset_alias> [graph_type] [results_dir]
#   dataset_alias: ldbc or freebase
#   graph_type: adj (default) or split_ekey
#   results_dir: where to write <dataset>_raw.dat (default: results/figure_7)
#
# Output: CSV lines appended to <results_dir>/<dataset>_raw.dat
#   flexograph-adj,node-property-search.groovy,14567.72
#   ...

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FG_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${FG_BUILD_DIR:-$FG_ROOT/build/preprocess_aster}"
BENCH_BIN="$BUILD_DIR/benchmark/aster/fg_property_bench"

# Aster artifact root (parent of flexograph/)
ASTER_ROOT="$(cd "$FG_ROOT/.." && pwd)"
DATA_DIR="$ASTER_ROOT/AsterDB/dataset"
DB_DIR="${FG_DB_DIR:-$ASTER_ROOT/flexograph_dbs}"

DATASET="${1:?Usage: $0 <dataset_alias> [graph_type] [results_dir]}"
GRAPH_TYPE="${2:-adj}"
RESULTS_DIR="${3:-$ASTER_ROOT/results/figure_7}"

mkdir -p "$DB_DIR" "$RESULTS_DIR"

DB_NAME="fg_props_${GRAPH_TYPE}_${DATASET}"
RAW_FILE="$RESULTS_DIR/${DATASET}_raw.dat"

$BENCH_BIN -g "$GRAPH_TYPE" -p "$DB_DIR" -m "$DB_NAME" \
  --data_dir="$DATA_DIR" --dataset="$DATASET" 2>/dev/null \
  | tee -a "$RAW_FILE"

echo "[INFO] Appended to $RAW_FILE"
