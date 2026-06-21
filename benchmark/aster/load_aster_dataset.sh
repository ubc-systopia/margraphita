#!/bin/bash
# Load an Aster artifact dataset into FlexoGraph.
#
# Usage: ./load_aster_dataset.sh <dataset_alias> [graph_type]
#   dataset_alias: dblp, wikipedia, wikitalk, cit-patents
#   graph_type:    adj (default) or split_ekey
#
# This calls the FlexoGraph preprocess pipeline (init_db, mk_adjlists,
# bulk_insert_low_mem) directly, without the Python wrapper.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FG_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
DATA_DIR="$FG_ROOT/../graph-baselines/runtime/data"

# Build dir — adjust if your build is elsewhere
BUILD_DIR="${FG_BUILD_DIR:-$FG_ROOT/build/preprocess_aster}"
PREPROCESS_BIN="$BUILD_DIR/preprocess"

DATASET_ALIAS="${1:?Usage: $0 <dataset_alias> [graph_type]}"
GRAPH_TYPE="${2:-adj}"

# ── Resolve dataset ──────────────────────────────────────────────────────────
resolve_dataset() {
  case "$1" in
    dblp)        FILE="com-dblp.ungraph.json3"; NODES=317080;  EDGES=1049866;    DIR=false ;;
    wikipedia)   FILE="wikipedia.json3";        NODES=3333397; EDGES=123709901;  DIR=true  ;;
    wikitalk)    FILE="wikitalk.json3";          NODES=2394385; EDGES=5021409;   DIR=true  ;;
    cit-patents) FILE="cit-patents.json3";       NODES=3774768; EDGES=16518947;  DIR=true  ;;
    twitter)     FILE="twitter-2010.json3";      NODES=41652230; EDGES=1468365182; DIR=true ;;
    *)           echo "Unknown dataset: $1" >&2; exit 1 ;;
  esac
}

resolve_dataset "$DATASET_ALIAS"
GRAPH_PATH="$DATA_DIR/$FILE"

if [ ! -f "$GRAPH_PATH" ]; then
  echo "ERROR: Dataset file not found: $GRAPH_PATH" >&2
  exit 1
fi

# ── Directories ──────────────────────────────────────────────────────────────
ASTER_ROOT="$(cd "$FG_ROOT/.." && pwd)"
DB_DIR="${FG_DB_DIR:-$ASTER_ROOT/flexograph_dbs}"
WORK_DIR="$BUILD_DIR/aster_preprocess/$DATASET_ALIAS"
LOG_DIR="$BUILD_DIR/aster_logs"

mkdir -p "$DB_DIR" "$WORK_DIR" "$LOG_DIR"

# DB name: e.g., adj_r_dblp (read-optimized undirected) or adj_rd_cit-patents
RO_STR="r"
DIR_STR=""
if [ "$DIR" = "true" ]; then DIR_STR="d"; fi
DB_NAME="${GRAPH_TYPE}_${RO_STR}${DIR_STR}_${DATASET_ALIAS}"
NUM_THREADS=16

echo "=== Loading $DATASET_ALIAS into FlexoGraph ==="
echo "  Graph file: $GRAPH_PATH"
echo "  Nodes: $NODES, Edges: $EDGES, Directed: $DIR"
echo "  Graph type: $GRAPH_TYPE, DB name: $DB_NAME"
echo "  DB dir: $DB_DIR"

# ── Step 1: Init DBs (bulk_insert needs both adj and split_ekey) ─────────────
echo "[1/5] Initializing DBs..."
for GT in adj split_ekey; do
  _DB_NAME="${GT}_${RO_STR}${DIR_STR}_${DATASET_ALIAS}"
  INIT_CMD="$PREPROCESS_BIN/init_db -n -m $_DB_NAME -p $DB_DIR -s $DATASET_ALIAS -o -g $GT -e -l $LOG_DIR -r"
  if [ "$DIR" = "true" ]; then INIT_CMD="$INIT_CMD -d"; fi
  echo "  $INIT_CMD"
  $INIT_CMD
done

# ── Step 2: Split forward graph ─────────────────────────────────────────────
echo "[2/5] Splitting forward graph into $NUM_THREADS parts..."
split --number=l/$NUM_THREADS "$GRAPH_PATH" "$WORK_DIR/${DATASET_ALIAS}_"

# ── Step 3: Create reverse graph (sorted) and split ─────────────────────────
echo "[3/5] Creating reverse graph..."
REVERSE_FILE="$WORK_DIR/${DATASET_ALIAS}_reverse"
awk '{print $2"\t"$1}' "$GRAPH_PATH" > "$REVERSE_FILE"
echo "  Sorting reverse graph..."
sort -g -k1,1 -k2,2 --parallel=10 -S 4G "$REVERSE_FILE" > "${REVERSE_FILE}_sorted"
mv "${REVERSE_FILE}_sorted" "$REVERSE_FILE"
echo "  Splitting reverse graph into $NUM_THREADS parts..."
split --number=l/$NUM_THREADS "$REVERSE_FILE" "$WORK_DIR/${DATASET_ALIAS}_reverse_"

# ── Step 4: Create adjacency lists (forward + reverse) ──────────────────────
echo "[4/5] Creating adjacency lists..."
MK_CMD="$PREPROCESS_BIN/mk_adjlists -d $DATASET_ALIAS -e $EDGES -n $NODES -f $WORK_DIR/$DATASET_ALIAS -t $GRAPH_TYPE -p $DB_DIR -l $LOG_DIR/${GRAPH_TYPE}_rd_${DATASET_ALIAS}.log -m $NUM_THREADS"
if [ "$DIR" = "true" ]; then MK_CMD="$MK_CMD -D"; fi
echo "  $MK_CMD"
$MK_CMD

# ── Step 5: Bulk insert ─────────────────────────────────────────────────────
echo "[5/5] Bulk inserting..."
BULK_CMD="$PREPROCESS_BIN/bulk_insert_low_mem -d $DATASET_ALIAS -e $EDGES -n $NODES -f $WORK_DIR/$DATASET_ALIAS -p $DB_DIR -l $LOG_DIR/bulk_insert.log -m $NUM_THREADS -r"
if [ "$DIR" = "true" ]; then BULK_CMD="$BULK_CMD -D"; fi
echo "  $BULK_CMD"
$BULK_CMD

echo "=== Done! DB at: $DB_DIR/$DB_NAME ==="
