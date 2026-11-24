#!/usr/bin/env bash
# run_sample_candidates_cache_sweep.sh — the SAMPLE-CANDIDATES CACHE SWEEP. Sampling
# candidates (fof, khop2) and scoring them, in-memory (CSR) vs Flexograph (SET=mem),
# across WiredTiger cache sizes, over
# several graphs, one run_lp_all.sh call per graph (smallest first).
# (Not the FastRP sweep: that is run_fastrp_sweep.sh.)
#
# Usage (inside the flexograph-fresh container, nothing else using the DB):
#   bash run_sample_candidates_cache_sweep.sh                       # kron s12 s16 s17
#   GRAPHS="s19_e8" ROUNDS=1 bash run_sample_candidates_cache_sweep.sh
#   GRAPHS="" REAL="web-Google:adj_rd_web-Google" bash run_sample_candidates_cache_sweep.sh
#
# Env vars (all optional):
#   GRAPHS    kron graphs, in order       (default: "s12_e8 s16_e8 s17_e8")
#   REAL      real graphs as GRAPH:DB_NAME, space-separated, run after GRAPHS
#             (DB_ROOT=/drives/ssd2/flexograph_rd), e.g.
#             "web-Google:adj_rd_web-Google soc-LiveJournal:adj_rd_soc-LiveJournal_fastrp"
#   CACHES    WT cache sizes              (default: "100MB 512MB 2GB 6GB")
#   ROUNDS    interleaved rounds          (default: 3)
#   THREADS   threads                     (default: 20)
#   SET       run_lp_all.sh config set    (default: mem)
#   BIN       fg_linkpred_bench_db        (default: build/release copy)
# Each graph lands in OUT_ROOT/sweeps/sample_candidates_cache/<graph>_<time>/ (SUMMARY.md).

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GRAPHS=${GRAPHS-"s12_e8 s16_e8 s17_e8"}
REAL=${REAL:-}
export CACHES=${CACHES:-"100MB 512MB 2GB 6GB"}
export ROUNDS=${ROUNDS:-3}
export THREADS=${THREADS:-20}
export SET=${SET:-mem}
export BIN=${BIN:-/FlexoGraph/pull-property/margraphita/build/release/benchmark/linkpred/fg_linkpred_bench_db}

RUNNER=$SCRIPT_DIR/run_lp_all.sh
FAILED=()
echo "Sample-candidates cache sweep: graphs=[$GRAPHS] real=[$REAL] caches=[$CACHES] rounds=$ROUNDS set=$SET"

for g in $GRAPHS; do
    echo "======== $g  $(date '+%F %T')"
    GRAPH=$g bash "$RUNNER" || FAILED+=("$g")
done

for gd in $REAL; do
    g=${gd%%:*}; db=${gd#*:}
    echo "======== $g ($db)  $(date '+%F %T')"
    GRAPH=$g DB_NAME=$db DB_ROOT=/drives/ssd2/flexograph_rd bash "$RUNNER" || FAILED+=("$g")
done

echo "Sample-candidates cache sweep done $(date '+%F %T')"
[ ${#FAILED[@]} -eq 0 ] || { echo "graphs with failed runs: ${FAILED[*]}"; exit 1; }
