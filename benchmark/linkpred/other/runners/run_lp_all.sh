#!/usr/bin/env bash
# run_lp_all.sh — every direct and reference linkpred variant on one graph,
# interleaved over ROUNDS, then a median summary.
#
# Direct (threaded, --threads=$THREADS) and reference (single-thread) only; the
# pipeline paths are not benchmarked (see steps.md 0.1). Each run is its own
# process with --warmup=1 --num-trials=1, so every row is a warm run and the
# variants alternate (A B C … A B C …) instead of running back to back.
#
# Usage (inside the flexograph-perf container, nothing else using the DB):
#   ./run_lp_all.sh                 # s12_e8
#   GRAPH=s16_e8 ./run_lp_all.sh    # exhaustive grows as n²: s16 ≈ 2.1e9 pairs
#
# Env vars (all optional):
#   GRAPH     kron graph               (default: s12_e8)
#   GT        adj | split_ekey         (default: adj)
#   THREADS   threads for direct runs  (default: 20)
#   ROUNDS    interleaved rounds       (default: 3; median reported)
#   TILES     tile sizes for tiled     (default: "64 256")
#   SET       all | fof | mem | emb | struct   (default: all)
#             struct = structural only (cn + jaccard) over fof / khop2: DB cursor,
#             DB wedge, DB outcursor_wedge, CSR cursor, CSR wedge (no embeddings)
#             emb = embedding scoring (cosine + dot) over sampled candidates, the
#             variants expected to win: fof / khop2 x DB cursor, CSR + pvector
#             (--variant=csr), and khop2 outcursor_nested. Run it on a DB whose
#             node_props hold FastRP (fg_fastrp --write-db), e.g. DB_NAME=adj_r_<g>_fastrp
#             mem = in-memory vs Flexograph on the same candidates (fof, khop2):
#             DB cursor / wedge vs csr_cursor (same algorithm, CSR in RAM) /
#             csr (wedge in RAM), cn + jaccard; cosine cursor vs csr (CSR + pvector)
#   CACHES    WT cache sizes to sweep, e.g. "100MB 1GB 4GB" (default: "" = WT default);
#             every config runs once per size (--cache_size), named "<config> @<size>"
#             fof = FoF candidates only, plain (N(v) per candidate) vs wedge
#             (CN counted along u->w->v), cn + jaccard, direct + reference;
#             adds exhaustive tiled64 / csr as a reference point when the graph
#             is small enough (EXHAUSTIVE=1, default on for s12 / s16 only);
#             also cosine over the same FoF candidates (+ tiled64 / pvector
#             exhaustive cosine when EXHAUSTIVE=1)
#   DB_ROOT   parent of <GRAPH>/       (default: /drives/ssd2/flexograph_rd/kron)
#   DB_NAME   DB folder in DB_ROOT/GRAPH (default: adj_r_<GRAPH> / split_ekey_r_<GRAPH>;
#             real graphs: adj_rd_web-Google, adj_rd_soc-LiveJournal, ...)
#   BIN       fg_linkpred_bench_db     (default: sibling)
#   OUT_ROOT  results parent folder    (default: benchmark/linkpred/output)
#
# Output (bench --out-root): each run lands with its run type, all graphs together:
#   OUT_ROOT/<structural|embedding>/<impl>_<candidates>_<variant>/metrics.csv, topk.csv,
#   logs/<graph>/<time>_t<threads>.log
# This sweep's index + summary: OUT_ROOT/sweeps/<sweep>/<graph>_<timestamp>/runs.tsv, <sweep> =
# sample_candidates_cache (SET=mem + CACHES: the sample-candidates cache sweep),
# sample_candidates (SET=mem), lp_all, lp_fof,
# SUMMARY.md (median per config + agreement check).

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GRAPH=${GRAPH:-s12_e8}
GT=${GT:-adj}
THREADS=${THREADS:-20}
ROUNDS=${ROUNDS:-3}
TILES=${TILES:-"64 256"}
SET=${SET:-all}
CACHES=${CACHES:-}
# 2026-11-23: direction / held-out split / variant list (see fg_linkpred_bench_db --help)
DIRECTION=${DIRECTION:-both}       # both | out   (only matters for _rd DBs, opened with -d)
EVAL=${EVAL:-recon}                # recon | holdout (holdout needs SPLIT=<test-pair file>)
SPLIT=${SPLIT:-}                   # container path, from tools/make_edge_split.py
VARIANTS=${VARIANTS:-"cursor wedge outcursor_wedge wedge_compact outcursor_wedge_compact csr_cursor csr"}   # SET=struct
EMB_VARIANTS=${EMB_VARIANTS:-"cursor csr outcursor_nested"}                                          # SET=emb
case "$GRAPH" in s12_e8|s16_e8) EXHAUSTIVE=${EXHAUSTIVE:-1} ;; *) EXHAUSTIVE=${EXHAUSTIVE:-0} ;; esac
# Reference (single-thread) FoF only where it finishes in reasonable time.
case "$GRAPH" in s12_e8|s16_e8|s17_e8) FOF_REF=${FOF_REF:-1} ;; *) FOF_REF=${FOF_REF:-0} ;; esac
BIN=${BIN:-$SCRIPT_DIR/fg_linkpred_bench_db}
OUT_ROOT=${OUT_ROOT:-/FlexoGraph/pull-property/margraphita/benchmark/linkpred/output}   # container path of benchmark/linkpred/output
DB_ROOT=${DB_ROOT:-/drives/ssd2/flexograph_rd/kron}   # real graphs: DB_ROOT=/drives/ssd2/flexograph_rd

[ -x "$BIN" ] || { echo "ERROR: binary not executable: $BIN (set BIN=...)" >&2; exit 2; }
if [ -z "${DB_NAME:-}" ]; then
    if [ "$GT" = "adj" ]; then DB_NAME="adj_r_$GRAPH"; else DB_NAME="split_ekey_r_$GRAPH"; fi
fi
[ -d "$DB_ROOT/$GRAPH/$DB_NAME" ] || { echo "ERROR: DB not found: $DB_ROOT/$GRAPH/$DB_NAME" >&2; exit 2; }

case "$SET" in mem) SWEEP=sample_candidates ;; emb) SWEEP=sample_candidates_emb ;; struct) SWEEP=sample_candidates_struct ;; *) SWEEP=lp_$SET ;; esac
[ -n "$CACHES" ] && SWEEP=${SWEEP}_cache    # e.g. sample_candidates_cache
[ "$EVAL" = "holdout" ] && SWEEP=${SWEEP}_holdout
if [ "$EVAL" = "holdout" ] && [ -z "$SPLIT" ]; then echo "ERROR: EVAL=holdout needs SPLIT=<test-pair file>" >&2; exit 2; fi
OUT=$OUT_ROOT/sweeps/$SWEEP/${GRAPH}_$(date +%Y%m%d_%H%M%S)
mkdir -p "$OUT"
DFLAG=""; case "$DB_NAME" in *_rd_*|*_rd) DFLAG="-d" ;; esac      # directed DB: open with -d
EFLAGS="--direction=$DIRECTION"
[ "$EVAL" = "holdout" ] && EFLAGS="$EFLAGS --eval=holdout --split-file=$SPLIT"
DB="-p $DB_ROOT/$GRAPH -m $DB_NAME -g $GT -r $DFLAG --topk=10 --warmup=1 --num-trials=1 $EFLAGS"

# One config per line: <name>|<flags>. Names are what SUMMARY.md shows.
CONFIGS=()
if [ "$SET" = "struct" ]; then
for m in ${METHODS:-cn jaccard}; do
    for c in "fof|--candidates=fof" "khop2|--candidates=khop --khop=2"; do
        cn_=${c%%|*}; cf=${c#*|}
        for v in $VARIANTS; do
            lab=$v; [ "$v" = "csr" ] && lab=csr_wedge
            CONFIGS+=("$m direct ${cn_}_$lab|--method=$m $cf --variant=$v --threads=$THREADS")
        done
    done
done
elif [ "$SET" = "emb" ]; then
for m in ${EMB_METHODS:-cosine dot}; do
    for c in "fof|--candidates=fof" "khop2|--candidates=khop --khop=2"; do
        cn_=${c%%|*}; cf=${c#*|}
        for v in $EMB_VARIANTS; do
            [ "$v" = "outcursor_nested" ] && continue                 # khop2 only, added below
            CONFIGS+=("$m direct ${cn_}_$v|--method=$m $cf --variant=$v --threads=$THREADS")
        done
    done
    case " $EMB_VARIANTS " in *" outcursor_nested "*)
        CONFIGS+=("$m direct khop2_outcursor_nested|--method=$m --candidates=khop --khop=2 --variant=outcursor_nested --threads=$THREADS") ;; esac
done
elif [ "$SET" = "mem" ]; then
for m in ${METHODS:-cn jaccard}; do
    for c in "fof|--candidates=fof" "khop2|--candidates=khop --khop=2"; do
        cn_=${c%%|*}; cf=${c#*|}
        CONFIGS+=("$m direct ${cn_}_cursor|--method=$m $cf --variant=cursor --threads=$THREADS")
        CONFIGS+=("$m direct ${cn_}_wedge|--method=$m $cf --variant=wedge --threads=$THREADS")
        CONFIGS+=("$m direct ${cn_}_csr_cursor|--method=$m $cf --variant=csr_cursor --threads=$THREADS")
        CONFIGS+=("$m direct ${cn_}_csr_wedge|--method=$m $cf --variant=csr --threads=$THREADS")
    done
done
CONFIGS+=("cosine direct fof_cursor|--method=cosine --candidates=fof --variant=cursor --threads=$THREADS")
CONFIGS+=("cosine direct fof_csr|--method=cosine --candidates=fof --variant=csr --threads=$THREADS")
CONFIGS+=("cosine direct khop2_cursor|--method=cosine --candidates=khop --khop=2 --variant=cursor --threads=$THREADS")
CONFIGS+=("cosine direct khop2_csr|--method=cosine --candidates=khop --khop=2 --variant=csr --threads=$THREADS")
elif [ "$SET" = "fof" ]; then
for m in ${METHODS:-cn jaccard}; do
    CONFIGS+=("$m direct fof|--method=$m --candidates=fof --threads=$THREADS")
    CONFIGS+=("$m direct fof_wedge|--method=$m --candidates=fof --variant=wedge --threads=$THREADS")
    if [ "$FOF_REF" = "1" ]; then
        CONFIGS+=("$m ref fof|--method=$m --impl=reference --candidates=fof")
        CONFIGS+=("$m ref fof_wedge|--method=$m --impl=reference --candidates=fof --variant=wedge")
    fi
    if [ "$EXHAUSTIVE" = "1" ]; then
        CONFIGS+=("$m direct tiled64|--method=$m --variant=tiled --tile=64 --threads=$THREADS")
        CONFIGS+=("$m direct csr|--method=$m --variant=csr --threads=$THREADS")
    fi
done
# Embedding over the same FoF candidates: v's embedding read per candidate.
CONFIGS+=("cosine direct fof|--method=cosine --candidates=fof --threads=$THREADS")
[ "$FOF_REF" = "1" ] && CONFIGS+=("cosine ref fof|--method=cosine --impl=reference --candidates=fof")
[ "$EXHAUSTIVE" = "1" ] && CONFIGS+=("cosine direct tiled64|--method=cosine --variant=tiled --tile=64 --threads=$THREADS")
[ "$EXHAUSTIVE" = "1" ] && CONFIGS+=("cosine direct pvector|--method=cosine --variant=pvector --threads=$THREADS")
else
for m in ${METHODS:-cn jaccard}; do
    CONFIGS+=("$m direct cursor|--method=$m --variant=cursor --threads=$THREADS")
    for t in $TILES; do
        CONFIGS+=("$m direct tiled$t|--method=$m --variant=tiled --tile=$t --threads=$THREADS")
    done
    CONFIGS+=("$m direct csr|--method=$m --variant=csr --threads=$THREADS")
    CONFIGS+=("$m direct fof|--method=$m --candidates=fof --threads=$THREADS")
    CONFIGS+=("$m direct khop2|--method=$m --candidates=khop --khop=2 --threads=$THREADS")
    CONFIGS+=("$m direct khop3|--method=$m --candidates=khop --khop=3 --threads=$THREADS")
    CONFIGS+=("$m direct fof_csr_cursor|--method=$m --candidates=fof --variant=csr_cursor --threads=$THREADS")
    CONFIGS+=("$m direct khop2_csr_cursor|--method=$m --candidates=khop --khop=2 --variant=csr_cursor --threads=$THREADS")
    CONFIGS+=("$m ref structural|--method=$m --impl=reference --variant=cursor")
    CONFIGS+=("$m ref preloaded_ids|--method=$m --impl=reference --variant=preloaded_ids")
    CONFIGS+=("$m ref preloaded_adj|--method=$m --impl=reference --variant=preloaded_adj")
    CONFIGS+=("$m ref tiled64|--method=$m --impl=reference --variant=tiled --tile=64")
    CONFIGS+=("$m ref fof|--method=$m --impl=reference --candidates=fof")
    CONFIGS+=("$m ref khop2|--method=$m --impl=reference --candidates=khop --khop=2")
done
for m in ${EMB_METHODS:-cosine dot}; do
    CONFIGS+=("$m direct cursor|--method=$m --variant=cursor --threads=$THREADS")
    for t in $TILES; do
        CONFIGS+=("$m direct tiled$t|--method=$m --variant=tiled --tile=$t --threads=$THREADS")
    done
    CONFIGS+=("$m direct pvector|--method=$m --variant=pvector --threads=$THREADS")
    CONFIGS+=("$m direct fof|--method=$m --candidates=fof --threads=$THREADS")
    CONFIGS+=("$m direct khop2|--method=$m --candidates=khop --khop=2 --threads=$THREADS")
    CONFIGS+=("$m direct fof_csr|--method=$m --candidates=fof --variant=csr --threads=$THREADS")
    CONFIGS+=("$m direct khop2_csr|--method=$m --candidates=khop --khop=2 --variant=csr --threads=$THREADS")
    CONFIGS+=("$m ref cursor|--method=$m --impl=reference --variant=cursor")
    CONFIGS+=("$m ref cursor_cached|--method=$m --impl=reference --variant=cursor_cached")
    CONFIGS+=("$m ref preloaded_db|--method=$m --impl=reference --variant=preloaded_db")
    CONFIGS+=("$m ref pvector|--method=$m --impl=reference --variant=pvector")
    CONFIGS+=("$m ref tiled64|--method=$m --impl=reference --variant=tiled --tile=64")
    CONFIGS+=("$m ref fof|--method=$m --impl=reference --candidates=fof")
done
fi

# CACHES: one copy of every config per WT cache size (same interleaving per round)
if [ -n "$CACHES" ]; then
    BASE=("${CONFIGS[@]}"); CONFIGS=()
    for cs in $CACHES; do
        for c in "${BASE[@]}"; do CONFIGS+=("${c%%|*} @$cs|${c#*|} --cache_size=$cs"); done
    done
fi

echo "Sweep $SWEEP: graph=$GRAPH db=$DB_NAME caches=${CACHES:-default} direction=$DIRECTION eval=$EVAL${SPLIT:+ split=$SPLIT} ${DFLAG:+(-d)} threads=$THREADS rounds=$ROUNDS configs=${#CONFIGS[@]} -> $OUT"
RUNLIST=$OUT/runs.tsv
printf "round\tconfig\tlog\n" > "$RUNLIST"
FAILS=0
for r in $(seq 1 "$ROUNDS"); do
    echo "---- round $r/$ROUNDS"
    for c in "${CONFIGS[@]}"; do
        name=${c%%|*}; flags=${c#*|}
        tag=$(echo "$name" | tr ' ' '_')
        # shellcheck disable=SC2086
        text=$("$BIN" $DB $flags --out-root="$OUT_ROOT" 2>&1)
        rc=$?
        log=$(echo "$text" | grep -oE '^\[out-root\] .* log=.*' | sed 's/.* log=//' | tail -1)
        line=$(echo "$text" | grep '^LinkPredDB ' | tail -1)
        [ -n "$log" ] || { log=$OUT/failed_${tag}_r$r.log; echo "$text" > "$log"; }
        if [ $rc -ne 0 ] || [ -z "$line" ]; then
            echo "  FAIL rc=$rc $name — see $log"; FAILS=$((FAILS + 1)); continue
        fi
        printf "%s\t%s\t%s\n" "$r" "$name" "$log" >> "$RUNLIST"
        printf "  %-26s %s\n" "$name" "$(echo "$line" | grep -oE 'time_ms=[0-9.]+')"
    done
done

python3 - "$RUNLIST" "$OUT/SUMMARY.md" "$GRAPH" "$THREADS" "$ROUNDS" <<'PY'
import re, sys, statistics as st
runlist, out_md, graph, threads, rounds = sys.argv[1:6]
rows = {}
for line in open(runlist).read().splitlines()[1:]:
    r, name, log = line.split("\t")
    text = open(log).read()
    m = [l for l in text.splitlines() if l.startswith("LinkPredDB ")][-1]
    kv = dict(re.findall(r"(\w[\w@]*)=([^ ]+)", m))
    reads = re.findall(r"\[tiled\] tile=\d+ v_reads=(\d+)", text)
    d = rows.setdefault(name, {"t": [], "load": [], "pairs": set(), "edges": set(), "p": set(), "reads": set()})
    d["t"].append(float(kv["time_ms"])); d["load"].append(float(kv["load_ms"]))
    d["pairs"].add(kv["pairs_scored"]); d["edges"].add(kv["true_edges"]); d["p"].add(kv.get("P@10", "?"))
    if reads: d["reads"].add(reads[-1])

def fmt(v): return f"{v:,.1f}"
L = [f"# Link prediction, all direct + reference variants — {graph}\n",
     f"Direct runs: {threads} threads; reference runs: 1 thread. {rounds} interleaved rounds, each run "
     "`--warmup=1 --num-trials=1` (warm). Median `time_ms` (algorithm only) and `load_ms` (DB open + "
     "any preload). `v reads` only for direct tiled.\n"]
groups = {}
for name, d in rows.items():
    m, impl = name.split()[0], name.split()[1]
    groups.setdefault((m, impl), []).append((name, d))
for (m, impl) in sorted(groups):
    L.append(f"\n## {m}, {'direct (threaded)' if impl == 'direct' else 'reference (single thread)'}\n")
    L.append("| variant | time_ms (median) | min–max | load_ms | pairs | true edges | P@10 | v reads |")
    L.append("|---|---|---|---|---|---|---|---|")
    for name, d in groups[(m, impl)]:
        L.append(f"| {name.split(' ', 2)[2]} | {fmt(st.median(d['t']))} | {fmt(min(d['t']))}–{fmt(max(d['t']))} | "
                 f"{fmt(st.median(d['load']))} | {','.join(sorted(d['pairs']))} | {','.join(sorted(d['edges']))} | "
                 f"{','.join(sorted(d['p']))} | {','.join(sorted(d['reads'])) or '–'} |")

# Agreement: exhaustive variants of one method must share pairs/edges/P@10.
L.append("\n## Agreement check\n")
# Group by candidate set: the variant's first token (fof / khop2 / khop3, e.g.
# fof_csr_cursor -> fof), anything else is exhaustive. Within one method and one
# candidate set every variant / cache size must give the same pairs, edges, P@10.
def cand_of(n):
    c = n.split()[2].split("_")[0]
    return c if c in ("fof", "khop2", "khop3") else "exhaustive"
for m in sorted({n.split()[0] for n in rows}):
    for c in ("exhaustive", "fof", "khop2", "khop3"):
        cs = {n: d for n, d in rows.items() if n.split()[0] == m and cand_of(n) == c}
        if cs:
            sig = {(tuple(sorted(d['pairs'])), tuple(sorted(d['edges'])), tuple(sorted(d['p']))) for d in cs.values()}
            L.append(f"- {m}, {c} ({len(cs)} configs): " + ("all agree" if len(sig) == 1 else f"**DISAGREE** {sig}"))
open(out_md, "w").write("\n".join(L) + "\n")
print(open(out_md).read())
PY
echo "results: $OUT"
[ $FAILS -eq 0 ] || { echo "$FAILS run(s) failed"; exit 1; }
