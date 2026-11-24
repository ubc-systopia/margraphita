# `python-benchmarks/README.md`

```markdown
# Python benchmarks — PyG backend

Python-side integration between FlexoGraph and PyTorch Geometric. Separate from
the C++ link-prediction pipeline in `../` — nothing here modifies the LP
benchmark binaries, options, or dispatch.

---

## Purpose

Two goals:

1. **Option A (done):** Prove a custom PyG `GraphStore` + `FeatureStore` can
   serve `NeighborLoader` batches. Validates the integration shape.
2. **Option B (not started):** Custom `BaseSampler` that pushes neighbour
   sampling down to C++ per batch, avoiding materialization. This is the
   version that would scale past RAM.

Option A is what exists. Option B is future work.

---

## Directory layout

```
python-benchmarks/
├── README.md                    (this file)
├── pyg_smoke_test.py            integration test — materialized + tuple paths
├── py_khop_baseline.py          (not part of the backend; the standalone
│                                 linkpred Python baseline, see ../)
├── cuGraph.py                   (earlier cuGraph experiment, separate)
│
├── backend/
│   ├── __init__.py              re-exports
│   ├── edge_loader.py           load_edges(path) + load_edges_from_flexo(...)
│   ├── stub_feature_store.py    FeatureStore subclass (random features)
│   └── stub_graph_store.py      GraphStore subclass (COO edge index)
│
└── backend-cpp/
    └── dump_edges.cpp           standalone DB → edge-list tool
```

The C++ tool (`dump_edges.cpp`) is a sibling of the Python code, not part of
the LP benchmark tree. Its CMake target lives in
`../CMakeLists.txt` (one `add_executable` block).

---

## What each file does

### `backend/edge_loader.py`

Two functions with identical return contract — `(edge_index [2, |E|] int64, n)`:

| Function | Source |
|---|---|
| `load_edges(path)` | Raw text file, `u v` per line. Validated against the LP benchmark (matches `pairs_scored` / `true_edges` exactly on s12). |
| `load_edges_from_flexo(db_dir, db_name, graph_type, directed)` | Subprocess to `dump_edges`. Opens the FlexoGraph DB, parses `u v` output, symmetrizes. |

Both dedupe and drop self-loops. `directed=False` symmetrizes (adds reverse
edges); `directed=True` keeps direction.

### `backend/stub_feature_store.py`

`StubFeatureStore(FeatureStore)` — random feature matrix of shape `(n, 16)`.
Implements the PyG abstract contract:

- `_get_tensor(attr)` — full matrix (`attr.index` unset) or per-batch (index set)
- `_get_tensor_size(attr)` — `(n, 16)`
- `get_all_tensor_attrs()` — `[TensorAttr("x")]`

Uses `attr.is_set("index")` to distinguish unset from set — **not**
`attr.index is None`, because PyG uses `_FieldStatus.UNSET` sentinel, not `None`.

### `backend/stub_graph_store.py`

`StubGraphStore(GraphStore)` — wraps a COO `edge_index` in RAM. Implements:

- `_get_edge_index(attr)` — returns the whole `[2, |E|]` tensor
- `get_all_edge_attrs()` — one edge type, layout COO, size `(n, n)`
- `csc()` — column-compressed form (needed by `NeighborSampler`)
- `_put_edge_index`, `_remove_edge_index` — raise (read-only)

### `backend-cpp/dump_edges.cpp`

Standalone tool. Usage:

```
dump_edges <db_dir> <db_name> <graph_type> [--directed]
```

Example:
```
./dump_edges /drives/ssd2/flexograph_rd/kron/s12_e8 adj_r_s12_e8 adj
```

Output:
```
# n=4091 m=32447 directed=0
0 1
0 2
...
```

For undirected DBs, prints each unordered pair once (`u < v`). The consumer
symmetrizes. Header line is `#`-prefixed so comment-skipping parsers see only
edges.

---

## How to build `dump_edges`

CMake target in `../CMakeLists.txt`:

```cmake
add_executable(dump_edges python-benchmarks/backend-cpp/dump_edges.cpp)
target_include_directories(dump_edges PRIVATE ${PATH_SRC} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dump_edges PUBLIC ${NAME_LIB} graph_utils)
```

Build:

```bash
cd /FlexoGraph/pull-property/margraphita
cmake --build build/release --target dump_edges
```

Verify on s12:

```bash
cd build/release/benchmark/linkpred
./dump_edges /drives/ssd2/flexograph_rd/kron/s12_e8 adj_r_s12_e8 adj | head -3
# want:
#   # n=4091 m=32447 directed=0
#   0 1
#   0 2
```

`m=32447` is the validation gate — same number as `true_edges=32447` in every
LP run on s12.

---

## How to run the smoke test

```bash
cd python-benchmarks
~/pyg/bin/python pyg_smoke_test.py
```

Two paths are exercised:

| Path | Works? | What it does |
|---|---|---|
| **Materialized `Data`** | ✅ | `Data(x=fs._get_tensor(), edge_index=gs._get_edge_index())` → `NeighborLoader(data=data, ...)` |
| **Tuple `(fs, gs)`** | ✅ | `NeighborLoader(data=(fs, gs), ...)`: samples through `gs.csc()`, fetches features from `fs` per batch |

The tuple path is PyG's remote-backend interface. It works on PyG 2.8.1 once
the stores describe a homogeneous graph: `EdgeAttr(edge_type=None)`,
`TensorAttr(group_name=None)`, and `csc()` returning plain tensors in the
order NeighborSampler unpacks them, `row, colptr, perm`. (An earlier version
of the stubs used a typed edge `("node", "link", "node")` and returned
`(colptr, row, perm)` dicts, which raised `AttributeError: colptr`; that was
a stub bug, not a PyG bug.)

Toggle `EDGE_SOURCE = "db" | "raw"` in `pyg_smoke_test.py` to switch between
the DB-backed source (`load_edges_from_flexo`) and the raw-file source
(`load_edges`). Both must produce identical `|E|` and `n`.

---

## Where we paused

**Done:**

- `FeatureStore` and `GraphStore` subclasses validated against PyG's abstract
  contract via the materialized-`Data` path.
- `dump_edges` built; `load_edges_from_flexo` tested against the DB
  (2026-11-17): `EDGE_SOURCE = "db"` and `"raw"` both give `n=4091`,
  `|E|=64894` and identical materialized batches on s12.
- Tuple `(fs, gs)` path working (see above).

**Not started:**

- Option B: custom `BaseSampler` that calls a C++ `--sample-neighbors` mode
  per batch.
- Any benchmark comparison between PyG-native sampling and the standalone
  FlexoGraph binary.

---

## Immediate next steps (in order)

1. **Build `dump_edges`.** Paste the source into
   `backend-cpp/dump_edges.cpp`, `touch` it, run the cmake build above.
2. **Verify the tool directly.**
   `./dump_edges ... | wc -l` should give `32448` (32447 edges + 1 header).
3. **Flip `EDGE_SOURCE` to `"db"` in `pyg_smoke_test.py`.** Run it. Compare
   `n` and `|E|` to the raw-file run. If they match, Option A is complete.
4. ~~Note a PyG tuple-path bug in the writeup~~ (withdrawn 2026-11-17: the
   `AttributeError: colptr` came from the stubs' typed edge and `csc()` order,
   not from PyG; the tuple path works).

---

## Package notes

- `backend/` is a package (`__init__.py` present). Import as
  `from backend import load_edges_from_flexo, StubGraphStore, StubFeatureStore`.
- `_DEFAULT_DUMP_BIN` in `edge_loader.py` assumes
  `/FlexoGraph/pull-property/margraphita/build/release/benchmark/linkpred/dump_edges`.
  Override via `FLEXO_DUMP_BIN` env var if the tree moves.
- The Python venv at `~/pyg` currently has `torch 2.14.1+cu132` and
  `pyg_lib 0.9.0+pt214cu132`. `WITH_PYG_LIB: True` — `NeighborSampler` requires
  this.
- `torch_scatter` and `torch_sparse` are **not** installed. They have no
  `pt214cu132` wheel and `pyg_lib` alone satisfies `NeighborSampler`.

---

## Relationship to the paper

The paper's main comparison is standalone C++ (`fg_linkpred_bench_db`) versus
Python `sparse_chunked` (`py_khop_baseline.py`, in `../`). **That work is
complete and unchanged.** This backend directory is the "we also integrate
with the standard PyG interface" paragraph — a method-section addition, not a
results-section replacement.

The finding this backend enables: even with a custom `GraphStore` and
`FeatureStore`, PyG's supported `NeighborLoader` path materializes the full
edge index into RAM, which is the same memory ceiling that causes
`sparse_chunked` to OOM at s19. Option B (custom `BaseSampler` that streams
from C++ per batch) is what would avoid it, and that's future work.
```

Drop this into `python-benchmarks/README.md`. Everything the next person (or future you) needs to know about what state the backend is in, what still needs testing, and where the LP pipeline stands.
---

## Real node features (2026-11-24)

`FlexoFeatureStore` (in `backend/stub_feature_store.py`) serves the DB's own node properties
(e.g. FastRP embeddings stored by `fg_fastrp` with `FG_OUT=db`) instead of random tensors.

1. `export_features_from_flexo(db_dir, db_name, out_npy)` runs
   `dump_edges <db_dir> <db_name> adj --features=OUT.npy --features-only`
   (through `docker exec $FLEXO_CONTAINER` when called from the WSL host) and writes a float32
   `[max_id + 1, dim]` matrix, row = node id, same indexing as `load_edges_from_flexo`.
2. `FlexoFeatureStore(out_npy, n)` memory-maps it; a batch reads only its rows.

`pyg_smoke_test.py`: `FEATURE_SOURCE = "db"` (default now) or `"random"`; with `"db"` every batch
checks `batch.x` row by row against the DB features. On the host:
`FLEXO_CONTAINER=flexograph-fresh ~/pyg/bin/python pyg_smoke_test.py`.

Still an export (whole feature matrix written once); reading features per batch straight from the
DB is part of Option B.
