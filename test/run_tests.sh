#!/usr/bin/env bash
# run_tests.sh
#
# Runs all test binaries across all combinations of:
#   MK_NEDGES    : ON  (UseProps=ON)  or  OFF
#   read_optimize: true or false  (passed as a CLI flag to each binary)
#
# Intended to be run from the build directory:
#   cd flexograph_code/build && ./run_tests.sh
#
# Can also be run from anywhere by passing the build dir as an argument:
#   ./run_tests.sh /path/to/build

set -euo pipefail
# set -x

# ---------------------------------------------------------------------------
# Locate build and source directories
# ---------------------------------------------------------------------------

BUILD_DIR="${1:-$(pwd)}"
BUILD_DIR="$(cd "$BUILD_DIR" && pwd)"

# Extract the source directory from the cmake cache so cmake re-runs work.
CACHE="${BUILD_DIR}/CMakeCache.txt"
if [ ! -f "$CACHE" ]; then
    echo "ERROR: No CMakeCache.txt found in ${BUILD_DIR}." >&2
    echo "       Run cmake first, or pass the build directory as an argument." >&2
    exit 1
fi
SOURCE_DIR="$(grep -m1 "^CMAKE_HOME_DIRECTORY:INTERNAL=" "$CACHE" | cut -d= -f2)"
if [ -z "$SOURCE_DIR" ]; then
    echo "ERROR: Could not determine source directory from ${CACHE}." >&2
    exit 1
fi

PASS=0
FAIL=0
RESULTS=()

# ---------------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------------

run_test()
{
    local label="$1"; shift
    local cmd=("$@")

    printf "  %-60s" "$label"
    if "${cmd[@]}" > /dev/null 2>&1; then
        echo "PASS"
        PASS=$(( PASS + 1 ))
        RESULTS+=("PASS  $label")
    else
        echo "FAIL"
        FAIL=$(( FAIL + 1 ))
        RESULTS+=("FAIL  $label")
    fi
}

build_with()
{
    local mk_nedges="$1"   # ON or OFF

    echo ""
    echo "================================================================"
    echo " Building: MK_NEDGES=${mk_nedges}"
    echo "================================================================"

    cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" \
          -DCMAKE_BUILD_TYPE=Debug \
          -DUseProps="${mk_nedges}" \
          -DB64=ON \
          -Wno-dev \
          > "${BUILD_DIR}/cmake_${mk_nedges}.log" 2>&1

    cmake --build "$BUILD_DIR" \
          --parallel "$(nproc)" \
          > "${BUILD_DIR}/build_${mk_nedges}.log" 2>&1
}

run_suite()
{
    local mk_nedges="$1"
    local ro="$2"   # true or false

    local tag="MK_NEDGES=${mk_nedges} read_optimize=${ro}"
    echo ""
    echo "----------------------------------------------------------------"
    echo " Running: ${tag}"
    echo "----------------------------------------------------------------"

    local T="${BUILD_DIR}/test"

    # Flags to pass to each binary.
    # test_adj_list   defaults to read_optimize=false → needs --read-optimize to enable
    # test_split_ekey, test_mt_* default to read_optimize=true → need --no-read-optimize to disable
    local adj_ro_flag=""
    local ekey_ro_flag=""
    local mt_ro_flag=""

    if [ "$ro" = "true" ]; then
        adj_ro_flag="--read-optimize"
    else
        ekey_ro_flag="--no-read-optimize"
        mt_ro_flag="--no-read-optimize"
    fi

    # test_adj_list uses is_weighted=true which requires the edge table (MK_NEDGES).
    # Skip it when MK_NEDGES=OFF since there is no edge table to store weights.
    if [ "$mk_nedges" = "ON" ]; then
        run_test "[${tag}] test_adj_list"         "$T/test_adj_list"            $adj_ro_flag
    fi
    # test_error_handling_* always run both read_optimize modes internally.
    run_test "[${tag}] test_split_ekey"           "$T/test_split_ekey"          $ekey_ro_flag
    run_test "[${tag}] test_error_handling_adj"   "$T/test_error_handling_adj"
    run_test "[${tag}] test_error_handling_ekey"  "$T/test_error_handling_ekey"
    run_test "[${tag}] test_mt_adj"               "$T/test_mt_adj"              $mt_ro_flag
    run_test "[${tag}] test_mt_ekey"              "$T/test_mt_ekey"             $mt_ro_flag
}

# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

echo "Build dir  : ${BUILD_DIR}"
echo "Source dir : ${SOURCE_DIR}"

for mk_nedges in ON OFF; do
    build_with "$mk_nedges"
    for ro in true false; do
        run_suite "$mk_nedges" "$ro"
    done
done

# ---------------------------------------------------------------------------
# summary
# ---------------------------------------------------------------------------

echo ""
echo "================================================================"
echo " Summary"
echo "================================================================"
for r in "${RESULTS[@]}"; do
    echo "  $r"
done
echo ""
echo "  Passed: ${PASS}  Failed: ${FAIL}  Total: $(( PASS + FAIL ))"

if [ "$FAIL" -gt 0 ]; then
    echo "  OVERALL: FAIL"
    exit 1
else
    echo "  OVERALL: PASS"
    exit 0
fi
