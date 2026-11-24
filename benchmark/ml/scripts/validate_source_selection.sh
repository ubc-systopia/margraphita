#!/bin/bash
# Validate --source-selection=uniform in fg_ml_runner (run inside the container, on scratch DB copies).
#  1. all-source default unchanged (pairs/positives as before)
#  2. undirected sampled sources: fof, khop2, khop3, exhaustive vs a Python reference built from the DB's
#     own edges (khop --hops=1 = edge list): no unordered pair lost, no duplicate, reverse skips reported
#  3. selecting every vertex (uniform, count >= n) reproduces the full all-source candidate set
#  4. directed DB: filter off, ordered pairs kept
# Usage: validate_source_selection.sh [DB_DIR] [DB_NAME] [COUNT] [SEED] [DIRECTED_DB_DIR] [DIRECTED_DB_NAME]
B=${B:-/FlexoGraph/pull-property/margraphita/build/release/benchmark/ml/fg_ml_runner}
DIR=${1:-/FlexoGraph/dbs/scratch/mlrunner_s12}; DB=${2:-adj_r_s12_e8}; N=${3:-100}; SEED=${4:-7}
DDIR=${5:-/FlexoGraph/dbs/scratch/mlrunner/ws}; DDB=${6:-adj_rd_web-Stanford}
OUT=${OUT:-/tmp/validate_source_selection}; mkdir -p $OUT; BIG=100000000
P="-m $DB -p $DIR -g adj -r --embedding=stored --method=cosine --top-k=$BIG"
run() { local name=$1; shift; $B $P "$@" --out=$OUT/$name.csv > $OUT/$name.log 2>&1 || { echo "FAIL run $name"; tail -3 $OUT/$name.log; }; }

run edges --candidates=khop --hops=1
run all_fof --candidates=fof
for c in "fof" "khop --hops=2" "khop --hops=3" "exhaustive"; do
  t=$(echo $c | tr -d ' -' | sed 's/hops=//')
  run s_$t --candidates=$c --source-selection=uniform --source-count=$N --sampling-seed=$SEED --source-out=$OUT/sources.txt --count-fetches
  [ $t = exhaustive ] || run every_$t --candidates=$c --source-selection=uniform --source-count=$BIG
  [ $t = exhaustive ] || run allsrc_$t --candidates=$c
done
python3 - "$OUT" <<'PY'
import sys, collections, re
o = sys.argv[1]
def rows(n): return [tuple(map(int, l.split(",")[:2])) for l in open(f"{o}/{n}.csv").read().split()[1:]]
def log(n, k): m = re.search(k + r"=(\S+)", open(f"{o}/{n}.log").read()); return m.group(1) if m else None
adj = collections.defaultdict(set)
for u, v in rows("edges"): adj[u].add(v); adj[v].add(u)
S = [int(x) for x in open(f"{o}/sources.txt")]
def khop(u, k):
    d = {u: 0}; fr = [u]
    for i in range(k):
        fr = [y for x in fr for y in adj[x] if y not in d and not d.setdefault(y, i + 1) is None]
    return {v for v in d if v != u}
def fof(u): return {v for w in adj[u] for v in adj[w] if v != u}
ok = True
def check(cond, msg):
    global ok; ok &= cond; print(("PASS " if cond else "FAIL ") + msg)
print("all-source fof:", log("all_fof", "pairs_scored"), "pairs,", log("all_fof", "positive_pairs"), "positives (s12 before this change: 1061287, 16243)")
n_nodes = int(log("every_fof", "sources_selected"))
for t, gen in [("fof", fof), ("khop2", lambda u: khop(u, 2)), ("khop3", lambda u: khop(u, 3)),
               ("exhaustive", None)]:
    got = rows(f"s_{t}"); und = [tuple(sorted(p)) for p in got]
    if gen: exp = {tuple(sorted((u, v))) for u in S for v in gen(u)}
    else:   exp = None  # every pair (u, v), u in S, v any node != u
    if exp is None:
        ns = n_nodes; k = len(S); expect_n = k * (ns - 1) - k * (k - 1) // 2
        check(len(set(und)) == len(und) == expect_n and all(p[0] in S for p in got),
              f"{t}: {len(got)} pairs, no duplicates, = k(n-1) - k(k-1)/2 = {expect_n}")
    else:
        check(len(set(und)) == len(und), f"{t}: no duplicate unordered pairs ({len(got)})")
        check(set(und) == exp, f"{t}: equals reference set ({len(exp)}), lost {len(exp - set(und))}, extra {len(set(und) - exp)}")
    print(f"     reverse_duplicates_skipped={log('s_'+t,'reverse_duplicates_skipped')}")
    if t != "exhaustive":
        check(set(map(lambda p: tuple(sorted(p)), rows(f"every_{t}"))) == set(rows(f"allsrc_{t}")) and
              len(rows(f"every_{t}")) == len(rows(f"allsrc_{t}")), f"{t}: every vertex selected == all-source set ({len(rows(f'allsrc_{t}'))})")
print("ALL PASS" if ok else "SOME CHECKS FAILED")
PY
# 4. directed: filter must be off and ordered pairs kept
if [ -d "$DDIR/$DDB" ]; then
  $B -m $DDB -p $DDIR -g adj -r -d --embedding=stored --method=cosine --top-k=$BIG --candidates=fof \
     --source-selection=uniform --source-count=$N --sampling-seed=$SEED --out=$OUT/directed.csv > $OUT/directed.log 2>&1
  grep -E "reverse_filter|pairs_scored" $OUT/directed.log
  python3 -c "
r=[tuple(map(int,l.split(',')[:2])) for l in open('$OUT/directed.csv').read().split()[1:]]
s=set(r); both=sum(1 for u,v in s if (v,u) in s)//2
print(('PASS' if len(s)==len(r) else 'FAIL'),'directed: no repeated ordered pair;',both,'pairs kept in both orders (distinct targets)')"
fi
