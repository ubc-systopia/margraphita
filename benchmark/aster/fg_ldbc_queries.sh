#!/bin/bash
# fg_ldbc_queries.sh — Run FlexoGraph LDBC SNB benchmark across 4 configs
#
# Runs fg_ldbc_bench with {adj, splitekey} x {embedded, columnar} and
# extracts the 10 target queries into a merged CSV for Fig 8.
#
# Usage: fg_ldbc_queries.sh <db_dir> [--sf=N] [--params-dir=DIR]
#                           [--warmup=N] [--queries=N]
#                           [--warmup-bi=N] [--queries-bi=N]
#                           [--out=FILE]
#
# Output CSV (11-column):
#   experiment,graph_type,mode,sf,query_params,
#   p50_ms,p95_ms,p99_ms,max_ms,throughput_ops_per_sec,n_measured

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BENCH_BIN="${SCRIPT_DIR}/../../build/ldbc/fg_ldbc_bench"

if [ ! -x "$BENCH_BIN" ]; then
    echo "ERROR: fg_ldbc_bench not found at $BENCH_BIN" >&2
    echo "Build it: cd flexograph/build/ldbc && cmake ../.. -DCMAKE_BUILD_TYPE=Release -DB64=True && make fg_ldbc_bench" >&2
    exit 1
fi

DB_DIR="${1:?Usage: fg_ldbc_queries.sh <db_dir> [--sf=N] [--params-dir=DIR] [--warmup=N] [--queries=N] [--out=FILE]}"
shift

# Parse optional args
SF_ARG=""
PARAMS_ARG=""
OUT_FILE=""
WARMUP_ARG=""
QUERIES_ARG=""
WARMUP_BI_ARG=""
QUERIES_BI_ARG=""
EXTRA_ARGS=""
for arg in "$@"; do
    case "$arg" in
        --sf=*)         SF_ARG="$arg" ;;
        --params-dir=*) PARAMS_ARG="$arg" ;;
        --warmup=*)     WARMUP_ARG="$arg" ;;
        --queries=*)    QUERIES_ARG="$arg" ;;
        --warmup-bi=*)  WARMUP_BI_ARG="$arg" ;;
        --queries-bi=*) QUERIES_BI_ARG="$arg" ;;
        --out=*)        OUT_FILE="${arg#--out=}" ;;
        *)              EXTRA_ARGS="$EXTRA_ARGS $arg" ;;
    esac
done

# Target queries to extract
TARGET_QUERIES="r1|x1|a2|a3|x5|x3|ic3|ic9|bi1|bi12"

TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

# Run 4 configurations
CONFIGS=(
    "adj --embedded flexograph-adj-emb"
    "splitekey --embedded flexograph-ekey-emb"
    "adj _ flexograph-adj-col"
    "splitekey _ flexograph-ekey-col"
)

# Output header (11-column)
HEADER="experiment,graph_type,mode,sf,query_params,p50_ms,p95_ms,p99_ms,max_ms,throughput_ops_per_sec,n_measured"
if [ -n "$OUT_FILE" ]; then
    echo "$HEADER" > "$OUT_FILE"
else
    echo "$HEADER"
fi

for config in "${CONFIGS[@]}"; do
    read -r gtype emb_flag sys_name <<< "$config"

    CSV="$TMPDIR/${sys_name}.csv"

    CMD="$BENCH_BIN $DB_DIR $gtype $SF_ARG $PARAMS_ARG"
    [ -n "$WARMUP_ARG" ]     && CMD="$CMD $WARMUP_ARG"
    [ -n "$QUERIES_ARG" ]    && CMD="$CMD $QUERIES_ARG"
    [ -n "$WARMUP_BI_ARG" ]  && CMD="$CMD $WARMUP_BI_ARG"
    [ -n "$QUERIES_BI_ARG" ] && CMD="$CMD $QUERIES_BI_ARG"
    CMD="$CMD $EXTRA_ARGS"
    if [ "$emb_flag" != "_" ]; then
        CMD="$CMD $emb_flag"
    fi
    CMD="$CMD --out=$CSV"

    echo "[fg_ldbc_queries] Running: $sys_name ($gtype, ${emb_flag/_/columnar})" >&2
    eval $CMD 2>&1 | grep -v "^experiment," >&2 || true

    if [ ! -f "$CSV" ]; then
        echo "[fg_ldbc_queries] WARNING: no output for $sys_name" >&2
        continue
    fi

    # Extract target queries — pass through full 11-column rows,
    # replacing graph_type with our system name
    awk -F, -v sys="$sys_name" -v targets="$TARGET_QUERIES" '
    BEGIN { split(targets, t, "|"); for (i in t) tgt[t[i]] = 1 }
    NR > 1 {
        exp = $1
        if (exp in tgt) {
            # Replace graph_type (col 2) with sys_name
            printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n", \
                $1, sys, $3, $4, $5, $6, $7, $8, $9, $10, $11
        }
    }
    ' "$CSV" >> "${OUT_FILE:-/dev/stdout}"
done

echo "[fg_ldbc_queries] Done." >&2
