name="GRAPH_PROJECT_DIR"
if [[ "${!name@a}" == *x* ]]; then
    echo "Using GRAPH_PROJECT_DIR as set - ${GRAPH_PROJECT_DIR}"
else
    echo "Please set and export GRAPH_PROJECT_DIR as the path to the margraphita repo."
fi

KRON_GEN=undef
name="KRON_GEN_PATH"

if [[ "${!name@a}" == *x* ]]; then
    echo "Using Kronecker Generator at ${KRON_GEN_PATH}"
    if [[ "${KRON_GEN_PATH}" =~ "PaRMAT" ]]
    then
        KRON_GEN=parmat #smooth_kron or parmat
        echo "Using PaRMAT"
    elif [[ "${KRON_GEN_PATH}" =~ "smooth_kron" ]]
    then
        KRON_GEN=smooth_kron
        echo "Using smooth kronecker"
    else
        echo "Unknown Kronecker Generator used."
        exit 1
    fi
else
    echo "Please set and export KRON_GEN_PATH as the path to a kronecker generator binary (PaRMAT or Smooth Kron)."
fi

name="KRON_GRAPHS_PATH"
if [[ "${!name@a}" == *x* ]]; then
    echo "Using KRON_GRAPHS_PATH as set - ${KRON_GRAPHS_PATH}"
else
    echo "Please set and export KRON_GRAPHS_PATH to indicate where to create kronecker graphs."
fi

DB_DIR=$GRAPH_PROJECT_DIR/db
PROFILE_PATH=$GRAPH_PROJECT_DIR/build/profile
RELEASE_PATH=$GRAPH_PROJECT_DIR/build/release
STATS_PATH=$GRAPH_PROJECT_DIR/build/wt_stats
OUTPUT_PATH=$GRAPH_PROJECT_DIR/outputs
