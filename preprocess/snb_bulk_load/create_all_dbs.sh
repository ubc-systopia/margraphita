#!/usr/bin/env bash
# create_all_dbs.sh — build all four FG database variants for an LDBC SNB dataset.
#
# Usage:
#   ./create_all_dbs.sh <data_dir> <db_parent_dir> [options]
#
# Arguments:
#   data_dir       Root of the LDBC SNB flat CSV dataset (contains static/ and dynamic/)
#   db_parent_dir  Directory under which all four DB subdirectories will be created
#
# Options:
#   --threads N        Worker threads per load run (default: number of logical CPUs)
#   --cache N          WiredTiger cache size in GB per load run (default: 4)
#   --bin PATH         Path to snb_bulk_load binary (default: auto-detect)
#   --shared-spool DIR Use DIR as the shared spool directory across all variants.
#                      Spool files are written once (Phase 1+2 of the first variant)
#                      and reused by the remaining three, saving ~3x the spool I/O.
#                      If DIR already contains spools, all four variants reuse them.
#                      Default: <db_parent_dir>/shared-spool
#   --skip NAME        Skip a specific variant; can be repeated.
#                      NAME: splitekey-embedded | splitekey-split | adj-embedded | adj-split
#   --dry-run          Pass --dry-run to snb_bulk_load (generates spools, no WT writes)
#   -h, --help         Show this help
#
# Output DB directories (created under db_parent_dir):
#   fg-splitekey-embedded-db
#   fg-splitekey-split-db
#   fg-adj-embedded-db
#   fg-adj-split-db
#
# Shared spool strategy:
#   Spool files encode property blobs using typed compact IDs derived from the
#   CSV data alone — they are independent of graph type (adj vs splitekey) and
#   property storage mode (embedded vs split/columnar).  Writing them once and
#   reusing them eliminates ~3x of redundant Phase 1/2 spool I/O.
#
#   Workflow:
#     1. splitekey-split  — writes spools to shared-spool/, keeps them (--keep-spool)
#     2. splitekey-embedded — reads existing spools (--reuse-spool)
#     3. adj-split          — reads existing spools (--reuse-spool)
#     4. adj-embedded       — reads existing spools (--reuse-spool)

set -euo pipefail

# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------
THREADS=$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 8)
CACHE=4
BIN=""
SHARED_SPOOL=""
DRY_RUN_FLAG=""
declare -A SKIP=()

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
usage() {
    sed -n '2,/^set -/p' "$0" | grep '^#' | sed 's/^# \{0,1\}//'
    exit 0
}

die() { echo "[ERROR] $*" >&2; exit 1; }
log() { echo "[$(date '+%H:%M:%S')] $*"; }

# Check whether any *.nspool or *.espool file exists in a directory
spools_exist() {
    local dir="$1"
    [[ -d "$dir" ]] && compgen -G "$dir"/*.nspool "$dir"/*.espool &>/dev/null
}

run_load() {
    local label="$1"
    local db_dir="$2"
    local gtype="$3"
    local extra="$4"       # e.g. "--embedded" or ""
    local spool_flags="$5" # "--spool DIR [--keep-spool] [--reuse-spool]"

    if [[ -n "${SKIP[$label]+x}" ]]; then
        log "SKIP  $label (--skip requested)"
        return 0
    fi

    if [[ -d "$db_dir" ]]; then
        log "SKIP  $label — db dir already exists: $db_dir"
        return 0
    fi

    mkdir -p "$db_dir"
    log "START $label → $db_dir"

    # Build command as an array so extra / spool_flags word-split correctly
    local cmd=("$BIN" "$DATA_DIR" "$db_dir" "$gtype"
               "--threads" "$THREADS"
               "--cache"   "$CACHE")
    [[ -n "$extra"       ]] && cmd+=($extra)
    [[ -n "$spool_flags" ]] && cmd+=($spool_flags)
    [[ -n "$DRY_RUN_FLAG" ]] && cmd+=("$DRY_RUN_FLAG")

    log "CMD   ${cmd[*]}"
    local t0=$SECONDS

    if "${cmd[@]}"; then
        log "DONE  $label ($(( SECONDS - t0 ))s)"
    else
        local rc=$?
        log "FAIL  $label — snb_bulk_load exited with code $rc"
        rm -rf "$db_dir"
        exit $rc
    fi
}

# ---------------------------------------------------------------------------
# Parse arguments
# ---------------------------------------------------------------------------
[[ $# -lt 2 ]] && { echo "Usage: $0 <data_dir> <db_parent_dir> [options]" >&2; exit 1; }

DATA_DIR="$(realpath "$1")"; shift
DB_PARENT="$(realpath "$1")"; shift

while [[ $# -gt 0 ]]; do
    case "$1" in
        --threads)      THREADS="$2";      shift 2 ;;
        --cache)        CACHE="$2";        shift 2 ;;
        --bin)          BIN="$2";          shift 2 ;;
        --shared-spool) SHARED_SPOOL="$2"; shift 2 ;;
        --skip)         SKIP["$2"]=1;      shift 2 ;;
        --dry-run)      DRY_RUN_FLAG="--dry-run"; shift ;;
        -h|--help) usage ;;
        *) die "Unknown option: $1" ;;
    esac
done

# ---------------------------------------------------------------------------
# Locate snb_bulk_load binary
# ---------------------------------------------------------------------------
if [[ -z "$BIN" ]]; then
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    CANDIDATE="$SCRIPT_DIR/../../../build/preprocess/snb_bulk_load/snb_bulk_load"
    if [[ -x "$CANDIDATE" ]]; then
        BIN="$(realpath "$CANDIDATE")"
    else
        BIN="$(command -v snb_bulk_load 2>/dev/null || true)"
    fi
fi
[[ -z "$BIN" || ! -x "$BIN" ]] && \
    die "Cannot find snb_bulk_load binary. Use --bin <path> to specify it."

# ---------------------------------------------------------------------------
# Validate inputs and set spool directory
# ---------------------------------------------------------------------------
[[ -d "$DATA_DIR" ]] || die "data_dir does not exist: $DATA_DIR"
mkdir -p "$DB_PARENT"

[[ -z "$SHARED_SPOOL" ]] && SHARED_SPOOL="$DB_PARENT/shared-spool"
mkdir -p "$SHARED_SPOOL"

log "=== FG bulk-load all variants ==="
log "data_dir     : $DATA_DIR"
log "db_parent    : $DB_PARENT"
log "binary       : $BIN"
log "threads      : $THREADS"
log "cache        : ${CACHE} GB"
log "shared-spool : $SHARED_SPOOL"
[[ -n "$DRY_RUN_FLAG" ]] && log "mode         : DRY RUN (no WT writes)"
echo

# ---------------------------------------------------------------------------
# Determine spool flags for each run
# ---------------------------------------------------------------------------
# If spools already exist in SHARED_SPOOL (e.g., from a previous partial run),
# all four variants reuse them.  Otherwise the first variant writes them and
# keeps them; the remaining three reuse them.
if spools_exist "$SHARED_SPOOL"; then
    log "Found existing spools in $SHARED_SPOOL — all variants will reuse them"
    SPOOL_FLAGS_1="--spool $SHARED_SPOOL --keep-spool --reuse-spool"
    SPOOL_FLAGS_R="--spool $SHARED_SPOOL --keep-spool --reuse-spool"
else
    log "No spools found — first variant will write them, rest will reuse"
    SPOOL_FLAGS_1="--spool $SHARED_SPOOL --keep-spool"           # write + keep
    SPOOL_FLAGS_R="--spool $SHARED_SPOOL --keep-spool --reuse-spool"  # reuse
fi

# ---------------------------------------------------------------------------
# Run the four variants
# ---------------------------------------------------------------------------
# Order: splitekey first (faster, good sanity check before long adj runs).
# First variant (splitekey-split) writes the shared spools; the rest reuse them.

run_load "splitekey-split"    "$DB_PARENT/fg-splitekey-split-db"    "splitekey" ""           "$SPOOL_FLAGS_1"
run_load "splitekey-embedded" "$DB_PARENT/fg-splitekey-embedded-db" "splitekey" "--embedded" "$SPOOL_FLAGS_R"
run_load "adj-split"          "$DB_PARENT/fg-adj-split-db"          "adj"       ""           "$SPOOL_FLAGS_R"
run_load "adj-embedded"       "$DB_PARENT/fg-adj-embedded-db"       "adj"       "--embedded" "$SPOOL_FLAGS_R"

log "=== All variants complete ==="
