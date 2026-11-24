#!/usr/bin/env bash
# run_fastrp_sweep.sh — the FASTRP SWEEP. FastRP embeddings: PyTorch/SciPy (run_node_emb.py fastrp)
# vs Flexograph (fg_fastrp, straight from the DB) over graphs x dim x iters, with
# fg_fastrp at several thread counts. One run per config per round.
#
# Run on the WSL host (NOT in the container): Python runs from ~/pyg here, and
# fg_fastrp runs through `docker exec $CONTAINER`. Nothing else should use the DB.
#   bash run_fastrp_sweep.sh
#   GRAPHS="s12_e8 web-Google" DIMS="16 32" ITERS="3" bash run_fastrp_sweep.sh
#   ROUNDS=3 bash run_fastrp_sweep.sh
#
# Env vars (all optional):
#   GRAPHS     (default: "s12_e8 web-Google facebook_combined soc-LiveJournal")
#   DIMS       (default: "16 32 64 128")
#   ITERS      (default: "3 6 9")
#   FG_THREADS fg_fastrp thread counts   (default: "1 2 6 10")
#   ROUNDS     rounds, interleaved        (default: 1)
#   PY         0 = skip Python            (default: 1)
#   FG         0 = skip fg_fastrp         (default: 1)
#   PY_THREADS OMP/MKL threads for Python (default: 1; SciPy sparse matmul is serial)
#   PY_IMPL    scipy | fast | rs  (default: scipy = node_emb_fastrp.py as before; fast = same maths,
#              dense B + torch threaded multiply; rs = PyPI "fastrp"
#              package, Rust, threads = PY_THREADS via RAYON_NUM_THREADS; rows labelled python_rs)
#   PY_VARIANT orig | paper  (default: orig = as fg_fastrp; paper = per-step row normalisation
#              + degree^beta scaling of R, Chen et al. 2019; PY_BETA, default -0.5)
#   PY_LOADER  loadtxt | pandas  (default: loadtxt as before)
#   FG_VARIANT orig | paper  (default: orig; fg_fastrp --variant, beta = PY_BETA; fg rows get _paper)
#              Python rows are labelled python[_impl][_paper][_pandas]
#   CONTAINER  (default: flexograph-fresh)
#   FG_OUT     fg_fastrp output modes, any of (default: "npy db"):
#                npy      --out=FILE.npy (embeddings to a file, as before)
#                db       --write-db: rows written into node_props during the last step
#                db_after --write-db=after: separate write pass, timed (todb_s)
#   FG_ADJ     how fg_fastrp reads the adjacency, any of (default: "cursor"):
#                cursor  re-read every neighbour list from the DB each step
#                csr     read once into an in-memory CSR (csr_s), then steps from RAM;
#                        rows get out = <mode>_csr, e.g. db_csr
#   PY_TO_DB   1 = after each Python run, load its .npy into the DB with
#              add_pytorch_emb_npy and time it (todb_s), i.e. the Python route to
#              the same end state as fg db   (default: 1)
#   DB_SUFFIX  appended to every DB name, to run on your copies
#              (default: "", or _fastrp with MAKE_COPY=1)
#              e.g. DB_SUFFIX=_fastrp -> adj_rd_web-Google_fastrp
#   MAKE_COPY  1 = before each graph: copy the original DB to <db><DB_SUFFIX>,
#              drop the copy's named checkpoints and compact it
#              (tools/wt_drop_checkpoints.c, compiled on first use), then run
#              on the copy. An existing copy is reused (still cleaned) unless
#              FRESH_COPY=1, which deletes and re-copies it.   (default: 0)
#
# db / db_after / PY_TO_DB write the embeddings into the DB they are given
# (node_props; each run overwrites the rows). Run on a copy, made beforehand,
# to keep the original as it is.
#
# Both sides symmetrise the graph (Python: A.maximum(A.T); fg_fastrp: -d reads
# in-lists too, used for the directed *_rd DBs), same weights [0.5,1,1,...],
# density 0.01, seed 42. The random projection differs (numpy RNG vs seeded
# hash), so embeddings are not bit-identical; the maths is.
#
# Timing columns (results.tsv; `out` = npy | db | db_after | file | file+db):
#   python:  load_s (np.loadtxt edge list), compute_s (train), save_s (.npy),
#            todb_s (add_pytorch_emb_npy into the DB copy, PY_TO_DB=1),
#            total_s = load + compute + save (+ todb_s)
#   fg:      load_s = csr_s, the CSR build (FG_ADJ=csr only);
#            compute_s = sum of the propagation steps' time_s (db: includes the
#            fused node_props writes), todb_s = write_s (db_after only),
#            total_s = DB open + all steps + output (fg_fastrp total_s)
# Output: benchmark/linkpred/output/sweeps/fastrp/<time>/
#   results.tsv  one row per run, failures included (status = ok | FAIL rc=N)
#                peak_rss_mb = process peak RSS (Python: /usr/bin/time %M, Python process only; fg: getrusage)
#   SUMMARY.md   median total_s / compute_s per config, Python vs Flexograph
#   sweep.log    everything printed to the console (copy/clean, FAIL lines)
#   config.txt   the settings, binaries and DBs used
#   logs/        one log per run (+ clean_<db>.log, add_embeddings_log.csv)
# Embeddings are deleted after each run (LiveJournal dim 128 is ~2.5 GB each).

set -uo pipefail

GRAPHS=${GRAPHS:-"s12_e8 web-Google facebook_combined soc-LiveJournal"}
DIMS=${DIMS:-"16 32 64 128"}
ITERS=${ITERS:-"3 6 9"}
FG_THREADS=${FG_THREADS:-"1 2 6 10"}
ROUNDS=${ROUNDS:-1}
PY=${PY:-1}
FG=${FG:-1}
PY_THREADS=${PY_THREADS:-1}
PY_IMPL=${PY_IMPL:-scipy}
PY_VARIANT=${PY_VARIANT:-orig}
PY_BETA=${PY_BETA:--0.5}
PY_LOADER=${PY_LOADER:-loadtxt}
FG_VARIANT=${FG_VARIANT:-orig}
PYLAB=python; [ "$PY_IMPL" = scipy ] || PYLAB=${PYLAB}_$PY_IMPL; [ "$PY_VARIANT" = orig ] || PYLAB=${PYLAB}_$PY_VARIANT; [ "$PY_LOADER" = loadtxt ] || PYLAB=${PYLAB}_$PY_LOADER
CONTAINER=${CONTAINER:-flexograph-fresh}
FG_OUT=${FG_OUT:-"npy db"}
FG_ADJ=${FG_ADJ:-cursor}
PY_TO_DB=${PY_TO_DB:-1}
MAKE_COPY=${MAKE_COPY:-0}
FRESH_COPY=${FRESH_COPY:-0}
if [ "$MAKE_COPY" = "1" ]; then DB_SUFFIX=${DB_SUFFIX:-_fastrp}; else DB_SUFFIX=${DB_SUFFIX:-}; fi
if [ "$MAKE_COPY" = "1" ] && [ -z "$DB_SUFFIX" ]; then
    echo "ERROR: MAKE_COPY=1 needs a non-empty DB_SUFFIX (it would copy the DB onto itself)" >&2; exit 2
fi

HOST_REPO=/home/larsav/flexography-clone-structural/pull-property/margraphita
CT_REPO=/FlexoGraph/pull-property/margraphita
FG_BIN=$CT_REPO/build/release/benchmark/linkpred/fg_fastrp
LOAD_BIN=$CT_REPO/build/release/test/add_pytorch_emb_npy
DROP_SRC=$CT_REPO/benchmark/linkpred/tools/wt_drop_checkpoints.c
DROP_BIN=$CT_REPO/build/release/benchmark/linkpred/wt_drop_checkpoints
WT_INST=$CT_REPO/libs/wiredtiger/wt_release/wiredtiger-install

# MAKE_COPY: build the checkpoint dropper if missing or older than its source
if [ "$MAKE_COPY" = "1" ]; then
    docker exec "$CONTAINER" bash -c "[ -x $DROP_BIN ] && [ $DROP_BIN -nt $DROP_SRC ] || \
        gcc -O1 $DROP_SRC -I$WT_INST/include -L$WT_INST/lib -lwiredtiger -Wl,-rpath,$WT_INST/lib -o $DROP_BIN" \
        || { echo "ERROR: could not build $DROP_BIN" >&2; exit 2; }
fi
# copy (or reuse) ORIG -> COPY in DBDIR, then drop named checkpoints + compact
make_copy() {   # DBDIR ORIG COPY
    if [ "$FRESH_COPY" = "1" ]; then docker exec "$CONTAINER" rm -rf "$1/$3"; fi
    if docker exec "$CONTAINER" test -d "$1/$3"; then
        echo "  [copy] reusing $1/$3"
    else
        echo "  [copy] $1/$2 -> $3"
        docker exec "$CONTAINER" cp -r "$1/$2" "$1/$3" || return 1
    fi
    local t0=$(date +%s)
    docker exec "$CONTAINER" "$DROP_BIN" "$1/$3" > "$OUT/logs/clean_$3.log" 2>&1 || return 1
    echo "  [copy] checkpoints dropped + compacted in $(( $(date +%s) - t0 ))s ($(docker exec "$CONTAINER" du -sh "$1/$3" | cut -f1))"
}
PYTHON=${PYTHON:-$HOME/pyg/bin/python}
PY_DIR=/mnt/c/ml-pytorch/pytorch-2v/scripts-tests/node_embeddings
RAW=/mnt/c/FlexoGraphData/hdd_main/raw-dataset_downloads

# graph -> edge list (host, for Python) | DB dir (container) | DB name | fg flags
graph_info() {
    case "$1" in
        s12_e8)            echo "/mnt/c/FlexoGraphData/hdd_main/kron_graphs/s12_e8/graph_s12_e8|/drives/ssd2/flexograph_rd/kron/s12_e8|adj_r_s12_e8|-r" ;;
        facebook_combined) echo "$RAW/facebook_combined/facebook_combined.txt|/drives/ssd2/flexograph_rd/facebook_combined|adj_r_facebook_combined|-r" ;;
        web-Google)        echo "$RAW/web-Google/web-Google.e|/drives/ssd2/flexograph_rd/web-Google|adj_rd_web-Google|-r -d" ;;
        soc-LiveJournal)   echo "$RAW/soc-LiveJournal/soc-LiveJournal.e|/drives/ssd2/flexograph_rd/soc-LiveJournal|adj_rd_soc-LiveJournal|-r -d" ;;
        s16_e8)            echo "/mnt/c/FlexoGraphData/hdd_main/kron_graphs/s16_e8/graph_s16_e8|/drives/ssd2/flexograph_rd/kron/s16_e8|adj_r_s16_e8|-r" ;;
        s17_e8)            echo "/mnt/c/FlexoGraphData/hdd_main/kron_graphs/s17_e8/graph_s17_e8|/drives/ssd2/flexograph_rd/kron/s17_e8|adj_r_s17_e8|-r" ;;
        s19_e8)            echo "/mnt/c/FlexoGraphData/hdd_main/kron_graphs/s19_e8/graph_s19_e8|/drives/ssd2/flexograph_rd/kron/s19_e8|adj_r_s19_e8|-r" ;;
        s20_e8)            echo "/mnt/c/FlexoGraphData/hdd_main/kron_graphs/s20_e8/graph_s20_e8|/drives/ssd2/flexograph_rd/kron/s20_e8|adj_r_s20_e8|-r" ;;
        cit)               echo "$RAW/cit/cit-Patents.txt|/drives/ssd2/flexograph_rd/cit|adj_rd_cit|-r -d" ;;
        web-Stanford)      echo "$RAW/web-Stanford/web-Stanford.txt|/drives/ssd2/flexograph_rd/web-Stanford|adj_rd_web-Stanford|-r -d" ;;
        ego-Twitter)       echo "$RAW/ego-Twitter/ego-Twitter.e|/drives/ssd2/flexograph_rd/ego-Twitter|adj_rd_ego-Twitter|-r -d" ;;
        soc-linkedin)      echo "$RAW/soc-linkedin/soc-linkedin.e|/drives/ssd2/flexograph_rd/soc-linkedin|adj_r_soc-linkedin|-r" ;;
        *) return 1 ;;
    esac
}

STAMP=$(date +%Y%m%d_%H%M%S)
OUT=$HOST_REPO/benchmark/linkpred/output/sweeps/fastrp/$STAMP
mkdir -p "$OUT/logs" "$OUT/pydata" || { echo "ERROR: cannot create $OUT (output/sweeps is root-owned if the container made it: docker exec $CONTAINER chown 1000:1000 <dir>)" >&2; exit 2; }
RES=$OUT/results.tsv
printf "round\tgraph\tsystem\tout\tthreads\tdim\titers\tload_s\tcompute_s\tsave_s\ttodb_s\ttotal_s\tlog\tstatus\tpeak_rss_mb\n" > "$RES"
exec > >(tee -a "$OUT/sweep.log") 2>&1          # keep the console output with the results
{
    echo "started    $(date '+%F %T')"
    for v in GRAPHS DIMS ITERS FG_THREADS FG_OUT ROUNDS PY FG PY_THREADS PY_IMPL PY_VARIANT PY_BETA PY_LOADER FG_VARIANT PY_TO_DB DB_SUFFIX MAKE_COPY FRESH_COPY CONTAINER PYTHON; do
        printf "%-11s %s\n" "$v" "${!v}"
    done
    echo "fg_fastrp  $(docker exec "$CONTAINER" stat -c '%y' "$FG_BIN" 2>/dev/null)"
    echo "python     $("$PYTHON" -c 'import sys, numpy, scipy, torch; print(sys.version.split()[0], "numpy", numpy.__version__, "scipy", scipy.__version__, "torch", torch.__version__)' 2>/dev/null)"
    echo "host       $(nproc) cores, $(free -g | awk '/Mem:/ {print $2}') GB RAM"
} > "$OUT/config.txt"
echo "FastRP sweep: graphs=[$GRAPHS] dims=[$DIMS] iters=[$ITERS] fg_threads=[$FG_THREADS] fg_out=[$FG_OUT] py_to_db=$PY_TO_DB db_suffix=[$DB_SUFFIX] make_copy=$MAKE_COPY rounds=$ROUNDS -> $OUT"
FAILS=0

for r in $(seq 1 "$ROUNDS"); do
for g in $GRAPHS; do
    info=$(graph_info "$g") || { echo "unknown graph $g"; FAILS=$((FAILS+1)); continue; }
    IFS='|' read -r EL DBDIR DBNAME FGFLAGS <<< "$info"
    if [ "$MAKE_COPY" = "1" ] && [ "$r" = "1" ]; then
        make_copy "$DBDIR" "$DBNAME" "$DBNAME$DB_SUFFIX" || { echo "  FAIL copy/clean $g"; FAILS=$((FAILS+1)); continue; }
    fi
    DBNAME=$DBNAME$DB_SUFFIX
    docker exec "$CONTAINER" test -d "$DBDIR/$DBNAME" || { echo "  FAIL $g: no DB $DBDIR/$DBNAME"; FAILS=$((FAILS+1)); continue; }
    for d in $DIMS; do
    for it in $ITERS; do
        # ---- Python (PyTorch / SciPy)
        if [ "$PY" = "1" ]; then
            log=$OUT/logs/r${r}_${g}_${PYLAB#python}py_d${d}_i${it}.log
            ( cd "$PY_DIR" && PYTHONPATH=/mnt/c/ml-pytorch/pytorch-2v${PYTHONPATH:+:$PYTHONPATH} OMP_NUM_THREADS=$PY_THREADS MKL_NUM_THREADS=$PY_THREADS OPENBLAS_NUM_THREADS=$PY_THREADS RAYON_NUM_THREADS=$PY_THREADS \
              /usr/bin/time -f "peak_rss_kb=%M" "$PYTHON" run_node_emb.py fastrp --edgelist_file_path "$EL" --data_root "$OUT/pydata" --no_gpu \
                --embedding_dim "$d" --fastrp_iterations "$it" --seed 42 --fastrp_impl "$PY_IMPL" --fastrp_variant "$PY_VARIANT" --fastrp_beta "$PY_BETA" --edge_loader "$PY_LOADER" ) > "$log" 2>&1
            rc=$?
            # "✓ X.XXs" lines: [1] load, [2] train, [3] save; then "TOTAL: X.XXs"
            mapfile -t ts < <(grep -oE '✓ [0-9.]+s' "$log" | grep -oE '[0-9.]+' )
            tot=$(grep -oE 'TOTAL: +[0-9.]+' "$log" | grep -oE '[0-9.]+$')
            peak=$(grep -oE 'peak_rss_kb=[0-9]+' "$log" | tail -1 | cut -d= -f2); [ -n "$peak" ] && peak=$((peak / 1024))   # Python process only (not the DB load)
            todb=""; pout=file
            if [ $rc -eq 0 ] && [ -n "$tot" ] && [ "$PY_TO_DB" = "1" ]; then
                # load this run's .npy into the DB copy (add_pytorch_emb_npy), container paths
                npy=$(find "$OUT/pydata" -name 'emb_*.npy' | head -1)
                ct_npy=${npy/#$HOST_REPO/$CT_REPO}
                docker exec "$CONTAINER" "$LOAD_BIN" "$DBDIR" "$DBNAME" "$(dirname "$ct_npy")" "$(basename "$ct_npy")" \
                    "${OUT/#$HOST_REPO/$CT_REPO}/logs" -s fastrp -g adj >> "$log" 2>&1
                lrc=$?
                todb=$(grep -oE 'Done\. Wrote [0-9]+, skipped [0-9]+ \([0-9.e+-]+s\)' "$log" | grep -oE '\([0-9.e+-]+' | tr -d '(')
                if [ $lrc -ne 0 ] || [ -z "$todb" ]; then rc=1; echo "  (load to DB failed, see $log)"; else
                    tot=$(awk -v a="$tot" -v b="$todb" 'BEGIN {printf "%.4f", a + b}'); pout=file+db; fi
            fi
            if [ $rc -ne 0 ] || [ -z "$tot" ]; then
                echo "  FAIL python $g d=$d it=$it rc=$rc — $log"; FAILS=$((FAILS+1))
                printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t\t\t\t\t\t%s\tFAIL rc=%s\n" "$r" "$g" "$PYLAB" "$pout" "$PY_THREADS" "$d" "$it" "$log" "$rc" >> "$RES"
            else
                printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" "$r" "$g" "$PYLAB" "$pout" "$PY_THREADS" "$d" "$it" \
                    "${ts[0]:-}" "${ts[-2]:-}" "${ts[-1]:-}" "$todb" "$tot" "$log" >> "$RES"
                sed -i "$ s/$/\tok\t$peak/" "$RES"
                printf "  %-18s %s %-8s t=%-2s d=%-3s it=%s  compute_s=%s todb_s=%s total_s=%s\n" "$g" "$PYLAB" "$pout" "$PY_THREADS" "$d" "$it" "${ts[-2]:-}" "${todb:--}" "$tot"
            fi
            find "$OUT/pydata" -name 'emb_*' -delete 2>/dev/null
        fi
        # ---- Flexograph (fg_fastrp)
        if [ "$FG" = "1" ]; then
            for adj in $FG_ADJ; do
            for mode in $FG_OUT; do
            for t in $FG_THREADS; do
                lab=$mode; [ "$adj" = "cursor" ] || lab=${mode}_$adj; [ "$FG_VARIANT" = orig ] || lab=${lab}_$FG_VARIANT
                log=$OUT/logs/r${r}_${g}_fg_${lab}_t${t}_d${d}_i${it}.log
                case $mode in
                    npy)      outf="--out=/tmp/fastrp_sweep.npy" ;;
                    db)       outf="--write-db" ;;
                    db_after) outf="--write-db=after" ;;
                    *) echo "unknown FG_OUT mode $mode"; FAILS=$((FAILS+1)); continue ;;
                esac
                # shellcheck disable=SC2086
                docker exec "$CONTAINER" "$FG_BIN" -p "$DBDIR" -m "$DBNAME" -g adj $FGFLAGS \
                    $outf --adj-read=$adj --variant=$FG_VARIANT --beta=$PY_BETA --dim="$d" --iters="$it" --seed=42 --threads="$t" > "$log" 2>&1
                rc=$?
                docker exec "$CONTAINER" rm -f /tmp/fastrp_sweep.npy
                tot=$(grep -oE 'total_s=[0-9.e+-]+' "$log" | tail -1 | cut -d= -f2)
                comp=$(grep -oE 'step [0-9]+/[0-9]+ .*time_s=[0-9.e+-]+' "$log" | grep -oE 'time_s=[0-9.e+-]+' | cut -d= -f2 | awk '{s+=$1} END {printf "%.4f", s}')
                todb=$(grep -oE 'write_s=[0-9.e+-]+' "$log" | cut -d= -f2)
                peak=$(grep -oE 'peak_rss_mb=[0-9]+' "$log" | tail -1 | cut -d= -f2)
                csrs=$(grep -oE 'csr_s=[0-9.e+-]+' "$log" | cut -d= -f2)   # CSR build time (load_s column)
                if [ $rc -ne 0 ] || [ -z "$tot" ]; then
                    echo "  FAIL fg $lab $g t=$t d=$d it=$it rc=$rc — $log"; FAILS=$((FAILS+1))
                    printf "%s\t%s\tflexograph\t%s\t%s\t%s\t%s\t\t\t\t\t\t%s\tFAIL rc=%s\n" "$r" "$g" "$lab" "$t" "$d" "$it" "$log" "$rc" >> "$RES"
                else
                    printf "%s\t%s\tflexograph\t%s\t%s\t%s\t%s\t%s\t%s\t\t%s\t%s\t%s\n" "$r" "$g" "$lab" "$t" "$d" "$it" "$csrs" "$comp" "$todb" "$tot" "$log" >> "$RES"
                    sed -i "$ s/$/\tok\t$peak/" "$RES"
                    if [ "$MAKE_COPY" = "1" ] && [ "$mode" != "npy" ]; then   # record what this copy holds
                        docker exec "$CONTAINER" bash -c "printf '%s\n' '{' '  \"source\": \"fastrp\",' '  \"db_name\": \"$DBNAME\",' '  \"npy_file\": \"(none: fg_fastrp --write-db, dim${d}_itr${it}_d0.01, seed 42, variant $FG_VARIANT)\",' '  \"dim\": $d,' '  \"iters\": $it,' '  \"written_at\": \"$(date '+%F %T')\",' '  \"written_by\": \"run_fastrp_sweep.sh (fg_fastrp --adj-read=$adj)\"' '}' > $DBDIR/$DBNAME/embedding.meta"
                    fi
                    printf "  %-18s fg     %-10s t=%-2s d=%-3s it=%s  compute_s=%s todb_s=%s total_s=%s\n" "$g" "$lab" "$t" "$d" "$it" "$comp" "${todb:--}" "$tot"
                fi
            done
            done
            done
        fi
    done
    done
done
done

# ---- SUMMARY.md: median over rounds per config; Python vs Flexograph side by side
"$PYTHON" - "$RES" "$OUT/SUMMARY.md" <<'PY'
import csv, sys, statistics as st
from collections import defaultdict
res, out = sys.argv[1:3]
rows = [r for r in csv.DictReader(open(res), delimiter="\t")]
ok = [r for r in rows if r["status"] == "ok"]
med = lambda xs: st.median(xs) if xs else None
agg = defaultdict(lambda: {"tot": [], "comp": []})
for r in ok:
    k = (r["graph"], r["dim"], r["iters"], r["system"], r["out"], r["threads"])
    agg[k]["tot"].append(float(r["total_s"]))
    if r["compute_s"]: agg[k]["comp"].append(float(r["compute_s"]))
fmt = lambda x: "–" if x is None else f"{x:.3f}"
L = ["# FastRP sweep: Python (SciPy) vs Flexograph (fg_fastrp)\n",
     f"{len(ok)} ok runs, {len(rows) - len(ok)} failed. Median over rounds. "
     "total_s: Python = load + compute + save (+ load into DB for file+db); "
     "Flexograph = DB open + steps + output (db: rows written during the last step). "
     "speedup = Python total / Flexograph total.\n"]
cols = sorted({(k[3], k[4], k[5]) for k in agg}, key=lambda c: (c[0] != "python", c[1], int(c[2])))
for g in sorted({k[0] for k in agg}):
    L.append(f"\n## {g}\n")
    L.append("| dim | iters | " + " | ".join(f"{s} {o} t{t} total (compute)" for s, o, t in cols) + " | best fg speedup |")
    L.append("|---|---|" + "---|" * len(cols) + "---|")
    for d, it in sorted({(k[1], k[2]) for k in agg if k[0] == g}, key=lambda x: (int(x[0]), int(x[1]))):
        cells, py, fg = [], None, []
        for s, o, t in cols:
            a = agg.get((g, d, it, s, o, t))
            tot = med(a["tot"]) if a else None
            cells.append(f"{fmt(tot)} ({fmt(med(a['comp']) if a else None)})")
            if tot is not None:
                if s == "python": py = tot
                else: fg.append(tot)
        sp = f"{py / min(fg):.1f}x" if py and fg else "–"
        L.append(f"| {d} | {it} | " + " | ".join(cells) + f" | {sp} |")
fails = [r for r in rows if r["status"] != "ok"]
if fails:
    L.append("\n## Failed runs\n")
    for r in fails: L.append(f"- {r['graph']} {r['system']} {r['out']} t{r['threads']} d{r['dim']} i{r['iters']} r{r['round']}: {r['status']} ({r['log']})")
open(out, "w").write("\n".join(L) + "\n")
PY
echo "finished $(date '+%F %T')" >> "$OUT/config.txt"
echo "results: $RES"
echo "summary: $OUT/SUMMARY.md"
[ $FAILS -eq 0 ] || { echo "$FAILS run(s) failed"; exit 1; }
