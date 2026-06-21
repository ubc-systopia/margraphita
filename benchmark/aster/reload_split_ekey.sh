#!/bin/bash
# Re-ingest all datasets using existing preprocessed adjlist files.
# Skips steps 2-4 (split/reverse/mk_adjlists) — only re-inits DBs and bulk inserts.
#
# Usage: ./reload_split_ekey.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FG_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

BUILD_DIR="${FG_BUILD_DIR:-$FG_ROOT/build/preprocess_aster}"
PREPROCESS_BIN="$BUILD_DIR/preprocess"
ASTER_ROOT="$(cd "$FG_ROOT/.." && pwd)"
DB_DIR="${FG_DB_DIR:-$ASTER_ROOT/flexograph_dbs}"
WORK_BASE="$BUILD_DIR/aster_preprocess"
LOG_DIR="$BUILD_DIR/aster_logs"

mkdir -p "$LOG_DIR"

resolve_dataset() {
  case "$1" in
    dblp)        NODES=317080;  EDGES=1049866;   DIR=false ;;
    wikipedia)   NODES=3333397; EDGES=123709901; DIR=true  ;;
    wikitalk)    NODES=2394385; EDGES=5021409;   DIR=true  ;;
    cit-patents) NODES=3774768; EDGES=16518947;  DIR=true  ;;
    *)           echo "Unknown dataset: $1" >&2; exit 1 ;;
  esac
}

NUM_THREADS=16

for ds in dblp wikitalk cit-patents wikipedia; do
  resolve_dataset "$ds"
  WORK_DIR="$WORK_BASE/$ds"

  if [ ! -d "$WORK_DIR" ]; then
    echo "[ERROR] Preprocessed files not found: $WORK_DIR — skipping $ds"
    continue
  fi

  echo ""
  echo "============================================"
  echo "  Re-ingesting: $ds"
  echo "============================================"

  # Step 1: Init DBs (both adj and split_ekey)
  echo "[1/2] Initializing DBs..."
  for GT in adj split_ekey; do
    RO_STR="r"
    DIR_STR=""
    if [ "$DIR" = "true" ]; then DIR_STR="d"; fi
    _DB_NAME="${GT}_${RO_STR}${DIR_STR}_${ds}"
    INIT_CMD="$PREPROCESS_BIN/init_db -n -m $_DB_NAME -p $DB_DIR -s $ds -o -g $GT -e -l $LOG_DIR -r"
    if [ "$DIR" = "true" ]; then INIT_CMD="$INIT_CMD -d"; fi
    echo "  $INIT_CMD"
    $INIT_CMD
  done

  # Step 2: Bulk insert (uses existing preprocessed adjlist files)
  echo "[2/2] Bulk inserting..."
  BULK_CMD="$PREPROCESS_BIN/bulk_insert_low_mem -d $ds -e $EDGES -n $NODES -f $WORK_DIR/$ds -p $DB_DIR -l $LOG_DIR/bulk_insert.log -m $NUM_THREADS -r"
  if [ "$DIR" = "true" ]; then BULK_CMD="$BULK_CMD -D"; fi
  echo "  $BULK_CMD"
  $BULK_CMD

  echo "=== Done: $ds ==="
done

echo ""
echo "=== All datasets re-ingested ==="
