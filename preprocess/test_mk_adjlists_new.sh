#!/usr/bin/env bash
# test_mk_adjlists_new.sh
#
# End-to-end correctness test for mk_adjlists_new.
#
# Test graph (6 nodes, 9 edges, input sorted by src then dst):
#   0->1, 0->2, 1->2, 1->3, 2->3, 2->4, 3->4, 3->5, 4->5
#
# Expected out-adjacency (src: sorted neighbors):
#   0  [1,2]   1  [2,3]   2  [3,4]   3  [4,5]   4  [5]
#
# Expected in-adjacency (dst: sorted in-neighbors):
#   1  [0]     2  [0,1]   3  [1,2]   4  [2,3]   5  [3,4]
#
# We run three scenarios to exercise different code paths:
#   T1: 1 thread,  budget=9  edges  → K=1 band  (single-band path)
#   T2: 1 thread,  budget=3  edges  → K=3 bands (multi-band, single thread)
#   T3: 4 threads, budget=3  edges  → K=3 bands (multi-band, multi-thread)
#

set -euo pipefail

BINARY="${1:-./mk_adjlists_new}"   # path to binary; override with $1
TMPDIR_BASE=$(mktemp -d)
PASS=0
FAIL=0

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
cleanup() { rm -rf "$TMPDIR_BASE"; }
trap cleanup EXIT

die() { echo "FAIL: $*" >&2; FAIL=$((FAIL+1)); }
ok()  { echo "PASS: $*"; PASS=$((PASS+1)); }

# collect_adjlists <dir> <prefix>
# Concatenates all files matching <dir>/<prefix>_?? and sorts by node ID.
collect_adjlists() {
    local dir="$1" prefix="$2"
    cat "$dir/${prefix}"_?? 2>/dev/null | sort -n -k1,1
}

# check_adjlists <label> <dir> <prefix> <expected_string>
# expected_string: one "node degree neighbors" line per line, separated by \n
check_adjlists() {
    local label="$1" dir="$2" prefix="$3" expected="$4"
    local got
    got=$(collect_adjlists "$dir" "$prefix")
    if [ "$got" = "$expected" ]; then
        ok "$label"
    else
        die "$label"
        echo "  expected:"
        echo "$expected" | sed 's/^/    /'
        echo "  got:"
        echo "$got"      | sed 's/^/    /'
    fi
}

# ---------------------------------------------------------------------------
# Build the test edge-list file
# ---------------------------------------------------------------------------
EDGE_FILE="$TMPDIR_BASE/test_edges.txt"
cat > "$EDGE_FILE" <<'EOF'
0 1
0 2
1 2
1 3
2 3
2 4
3 4
3 5
4 5
EOF

# Expected outputs (node_id degree neighbors — format written by the binary)
EXPECTED_OUT=$'0 2 1,2\n1 2 2,3\n2 2 3,4\n3 2 4,5\n4 1 5'
EXPECTED_IN=$'1 1 0\n2 2 0,1\n3 2 1,2\n4 2 2,3\n5 2 3,4'

# ---------------------------------------------------------------------------
# Run one scenario
# run_scenario <label> <threads> <budget_edges>
# ---------------------------------------------------------------------------
run_scenario() {
    local label="$1" threads="$2" budget="$3"
    local outdir="$TMPDIR_BASE/$label"
    mkdir -p "$outdir"

    # mk_adjlists_new flags:
    #   -f  output prefix (dir/dataset_name)
    #   -i  input edge-list file
    #   -n  num_nodes (used as fallback for max_node_id when -N is absent)
    #   -e  num_edges
    #   -m  num_threads
    #   -B  budget_edges (direct override; avoids needing a huge -M value)
    #   -d  db name (required by InsertOpts but unused here)
    #   -t  graph type (required by InsertOpts but unused here)
    "$BINARY"           \
        -f "$outdir/g"  \
        -i "$EDGE_FILE" \
        -n 6            \
        -e 9            \
        -m "$threads"   \
        -B "$budget"    \
        -d testdb       \
        -t adj          \
        > "$outdir/run.log" 2>&1

    local exit_code=$?
    if [ $exit_code -ne 0 ]; then
        die "$label: binary exited with code $exit_code"
        cat "$outdir/run.log" | sed 's/^/  /'
        return
    fi

    check_adjlists "$label: out-adj" "$outdir" "out"  "$EXPECTED_OUT"
    check_adjlists "$label: in-adj"  "$outdir" "in"   "$EXPECTED_IN"
}

# ---------------------------------------------------------------------------
# Scenarios
# ---------------------------------------------------------------------------
echo "Binary: $BINARY"
echo "Test graph: 6 nodes, 9 edges"
echo ""

run_scenario "1thread_1band"   1 9   # K=1: single band (all 9 edges fit)
run_scenario "1thread_3bands"  1 3   # K=3: budget=3 forces 3 bands
run_scenario "4thread_3bands"  4 3   # K=3, 4 threads: exercises parallel scatter

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo ""
echo "Results: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ] && exit 0 || exit 1
