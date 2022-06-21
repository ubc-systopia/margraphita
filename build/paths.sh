name="GRAPH_PROJECT_DIR"
if [[ "${!name@a}" == *x* ]]; then
    echo "Using GRAPH_PROJECT_DIR as set - ${GRAPH_PROJECT_DIR}"
else
    echo "Please set and export GRAPH_PROJECT_DIR as the path to the margraphita repo."
fi

KRON_GEN= #smooth_kron or parmat
KRON_GEN_PATH= #path to the kron generator binary
KRON_GRAPHS_PATH= #Directory where you want to store the kron graphs
DB_DIR=$GRAPH_PROJECT_DIR/db
PROFILE_PATH=$GRAPH_PROJECT_DIR/build/profile
RELEASE_PATH=$GRAPH_PROJECT_DIR/build/release
STATS_PATH=$GRAPH_PROJECT_DIR/build/wt_stats

# export GRAPH_PROJECT_DIR=/home/jackli/margraphita/