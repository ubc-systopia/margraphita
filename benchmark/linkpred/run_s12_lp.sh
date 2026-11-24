#!/bin/bash
# run_lp.sh — run link-pred configs on s12 + s16, log everything, print summary.
# Does NOT build. Does NOT clean prior outputs.

set -u

BENCH=/FlexoGraph/pull-property/margraphita/build/release/benchmark/linkpred/fg_linkpred_bench_db
LOGS=/drives/ml-pytorch-data/linkpred/flexography/run_logs
DB_ROOT=/drives/ssd2/flexograph_rd/kron

GRAPHS="s12_e8 s16_e8 s17_e8 s19_e8 s20_e8"
VARS="cursor"
THREAD_COUNTS="20"
# Structural (cn) variants, run after the cosine ones. Empty = skip.
# e.g. STRUCT_VARS="cursor csr" ./run_s12_lp.sh   (csr = adjacency preloaded)
STRUCT_VARS="${STRUCT_VARS:-}"

mkdir -p "$LOGS"

for g in $GRAPHS; do
  DB_DIR="$DB_ROOT/$g"
  DB_NAME="adj_r_$g"
  for v in $VARS; do
    for t in $THREAD_COUNTS; do
      TAG="${g}_${v}_${t}t"
      echo ""
      echo "=== $TAG ==="
      "$BENCH" \
        -p "$DB_DIR" -m "$DB_NAME" -g adj -r \
        --method=cosine --variation="$v" --threads="$t" \
        --topk=100 --source=fastrpdim16 \
        --out="$LOGS/test_metrics_${TAG}.csv" \
        --topk-out="$LOGS/test_topk_${TAG}.csv" \
        > "$LOGS/run_${TAG}.log" 2>&1
      rc=$?
      if [ $rc -ne 0 ]; then
        echo "  FAIL rc=$rc — see $LOGS/run_${TAG}.log"
        continue
      fi
      grep '^LinkPredDB ' "$LOGS/run_${TAG}.log" | tail -1 | sed 's/^/  /'
      grep -m1 'MemoryCounter:.*MB ->' "$LOGS/run_${TAG}.log" | sed 's/^/  /'
    done
  done
done

# ---- structural (cn) runs: TAG uses cn_<variant>, so the summaries below pick them up ----
for g in $GRAPHS; do
  DB_DIR="$DB_ROOT/$g"
  DB_NAME="adj_r_$g"
  for s in $STRUCT_VARS; do
    for t in $THREAD_COUNTS; do
      TAG="${g}_cn_${s}_${t}t"
      echo ""
      echo "=== $TAG ==="
      "$BENCH" \
        -p "$DB_DIR" -m "$DB_NAME" -g adj -r \
        --method=cn --variant="$s" --threads="$t" \
        --topk=100 \
        --out="$LOGS/test_metrics_${TAG}.csv" \
        --topk-out="$LOGS/test_topk_${TAG}.csv" \
        > "$LOGS/run_${TAG}.log" 2>&1
      rc=$?
      if [ $rc -ne 0 ]; then
        echo "  FAIL rc=$rc — see $LOGS/run_${TAG}.log"
        continue
      fi
      grep '^LinkPredDB ' "$LOGS/run_${TAG}.log" | tail -1 | sed 's/^/  /'
      grep -m1 'MemoryCounter:.*MB ->' "$LOGS/run_${TAG}.log" | sed 's/^/  /'
    done
  done
done

# Summaries cover both: cosine variants as-is, structural ones as cn_<variant>.
SUMMARY_VARS="$VARS"
for s in $STRUCT_VARS; do SUMMARY_VARS="$SUMMARY_VARS cn_$s"; done

echo ""
echo "=== Time / quality ==="
{
  echo "graph,variation,threads,time_ms,pairs_scored,true_edges,P@100,R@100"
  for g in $GRAPHS; do
    for v in $SUMMARY_VARS; do
      for t in $THREAD_COUNTS; do
        TAG="${g}_${v}_${t}t"
        f="$LOGS/run_${TAG}.log"
        [ -f "$f" ] || { echo "$g,$v,$t,FAIL,FAIL,FAIL,FAIL,FAIL"; continue; }
        line=$(grep '^LinkPredDB ' "$f" | tail -1)
        [ -z "$line" ] && { echo "$g,$v,$t,FAIL,FAIL,FAIL,FAIL,FAIL"; continue; }
        tm=$(echo "$line" | sed -E 's/.*time_ms=([0-9.]+).*/\1/')
        ps=$(echo "$line" | sed -E 's/.*pairs_scored=([0-9]+).*/\1/')
        te=$(echo "$line" | sed -E 's/.*true_edges=([0-9]+).*/\1/')
        pk=$(echo "$line" | sed -E 's/.*P@100=([0-9.eE+-]+).*/\1/')
        rk=$(echo "$line" | sed -E 's/.*R@100=([0-9.eE+-]+).*/\1/')
        echo "$g,$v,$t,$tm,$ps,$te,$pk,$rk"
      done
    done
  done
} | awk -F, 'NR==1{printf "%-8s %-8s %7s %10s %12s %10s %8s %10s\n",$1,$2,$3,$4,$5,$6,$7,$8; next}
            {printf "%-8s %-8s %7s %10s %12s %10s %8s %10s\n",$1,$2,$3,$4,$5,$6,$7,$8}'

echo ""
echo "=== Memory ==="
{
  echo "graph,variation,threads,start_MB,end_MB,delta_MB,major_faults,minor_faults"
  for g in $GRAPHS; do
    for v in $SUMMARY_VARS; do
      for t in $THREAD_COUNTS; do
        TAG="${g}_${v}_${t}t"
        f="$LOGS/run_${TAG}.log"
        [ -f "$f" ] || { echo "$g,$v,$t,?,?,?,?,?"; continue; }
        s=""; e=""; d=""; mj=""; mn=""
        if grep -q 'MB ->' "$f"; then
          read -r s e d <<< $(grep -m1 'MB ->' "$f" | sed -E 's/.*: ([0-9]+) MB -> ([0-9]+) MB, ([0-9]+) MB.*/\1 \2 \3/')
        fi
        if grep -q 'major faults' "$f"; then
          read -r mj mn <<< $(grep -m1 'major faults' "$f" | sed -E 's/.*: ([0-9]+) major faults, ([0-9]+) minor.*/\1 \2/')
        fi
        echo "$g,$v,$t,${s:-?},${e:-?},${d:-?},${mj:-?},${mn:-?}"
      done
    done
  done
} | awk -F, 'NR==1{printf "%-8s %-8s %7s %10s %8s %10s %12s %12s\n",$1,$2,$3,$4,$5,$6,$7,$8; next}
            {printf "%-8s %-8s %7s %10s %8s %10s %12s %12s\n",$1,$2,$3,$4,$5,$6,$7,$8}'