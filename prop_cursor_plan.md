# Property Cursor Abstraction — Implementation Plan

**Goal:** Replace all raw `WT_CURSOR*` usage in colgroup property scans with proper
RAII cursor abstractions that hide WiredTiger internals, fix exception-safety leaks,
and give the open-source API a clean surface.  The abstraction must be **generic over
both EMBEDDED and COLUMNAR storage modes** so that query code contains no
`if (prop_mode == COLUMNAR)` branches.

---

## Problem

`GraphBase::open_colgroup_cursor` returns a raw `WT_CURSOR*`.  Callers manually
drive the full WiredTiger C API — 10–14 lines per call site, duplicated across 22
sites.  Three problems:

1. **Exception-safety leak:** if anything between `open_colgroup_cursor` and
   `cur->close(cur)` throws, the cursor leaks.
2. **API over-exposure:** `WT_CURSOR*` exposes ~50 methods when callers only need
   next/seek.
3. **Mode branching in queries:** queries that support both storage modes contain
   explicit `if (has_props && prop_mode == COLUMNAR) { ... } else { get_edge_properties(...) }`
   forks.  Three queries (R2, A2, A3) have these today; IC-7/8/9/IC-5 have no EMBEDDED
   path at all and would silently produce wrong results in EMBEDDED mode.

---

## Key Design Decision: Unified API Over Both Modes

The two modes access properties in structurally different ways:

| | COLUMNAR | EMBEDDED |
|---|---|---|
| Node scan | Sequential cursor over colgroup B-tree | NodeCursor over topology + `get_node_properties` per node |
| Edge range scan | `search_near(src,0)` + forward scan on edge prop table | `get_out_nodes_id(src)` + `get_edge_properties` per neighbor |
| Edge point lookup | `search(src,dst)` on edge prop table | `get_edge_properties(src,dst)` directly |

The factory methods (`get_node_prop_cursor`, `get_edge_prop_cursor`) dispatch on
`opts.prop_mode` and return the right concrete class.  Query code becomes
mode-agnostic — it calls `cur->next()` / `cur->seek()` / `cur->get_uint64(0)`
identically regardless of which mode the DB was opened in.

**EMBEDDED cursor performance note:** `EmbEdgePropCursor::set_src(pid)` pre-fetches
the full neighbor list via `get_out_nodes_id(pid)`.  This matches what current
EMBEDDED query code already does; there is no extra cost.  Each `next()` call then
issues one `get_edge_properties` point lookup — the same access pattern as the
existing EMBEDDED else-branches.

---

## File Structure

```
src/iterator.h              ← add NodePropCursor + EdgePropCursor abstract bases
src/wt_prop_cursor.h        ← NEW: WTNodePropCursor + WTEdgePropCursor  (COLUMNAR)
src/emb_prop_cursor.h       ← NEW: EmbNodePropCursor + EmbEdgePropCursor (EMBEDDED)
src/graph.h                 ← replace open_colgroup_cursor with two factory virtuals
src/edgekey_split.h         ← #include both cursor headers + override declarations
src/edgekey_split.cpp       ← factory bodies (dispatch on prop_mode); remove old impl
src/adj_list.h              ← same as edgekey_split.h
src/adj_list.cpp            ← same as edgekey_split.cpp
test/ldbc_snb_queries.cpp   ← rewrite 22 COLUMNAR call sites + remove EMBEDDED branches
test/test_columnar_ekey.cpp ← rewrite 1 test function
test/test_columnar_adj.cpp  ← rewrite 1 test function
```

**Why two cursor headers, not one:**
- `wt_prop_cursor.h` — needs only `<wiredtiger.h>`.  No Flexograph types.
- `emb_prop_cursor.h` — needs `GraphBase*` to call `get_out_nodes_id` /
  `get_edge_properties` / `get_node_properties`.  Including `graph.h` here would
  create a circular include (`graph.h` → `emb_prop_cursor.h` → `graph.h`).
  Solution: forward-declare `GraphBase` in `emb_prop_cursor.h`; include `graph.h`
  only in the `.cpp` files.

**Why no `adjlist_nodpropcursor.h` etc.:** the prop cursor implementations are
backend-agnostic — unlike topology cursors (`AdjEdgeCursor` etc.) which access
different internal data structures, prop cursors use only the public `GraphBase` API
(EMBEDDED) or the WiredTiger colgroup cursor API (COLUMNAR), both of which are
identical regardless of backend.  A single shared header per mode is correct.

---

## Step 1 — `src/iterator.h`: add abstract base classes

Add after `EdgeCursor`, before `#endif`.  Add `#include <memory>` at the top.

```cpp
// ─── NodePropCursor ──────────────────────────────────────────────────────────
// Abstracts over a property scan keyed by node_id_t.
// COLUMNAR: wraps a WiredTiger colgroup cursor.
// EMBEDDED: wraps get_node_properties() point lookups + NodeCursor iteration.
// Returned as unique_ptr — destructor releases all resources (RAII).
// No public close() method.
class NodePropCursor {
public:
    // Restrict sequential scan to [start, end).
    // Passing OutOfBand_ID_MAX for end = unbounded full-table scan.
    virtual void set_range(node_id_t start, node_id_t end) = 0;

    // Advance to the next row.  Returns false at EOF or past range end.
    virtual bool next() = 0;

    // Exact point lookup.  Returns false if not found.
    // Mutually exclusive with next()/set_range() per cursor lifetime.
    virtual bool seek(node_id_t id) = 0;

    virtual node_id_t key()             const = 0;
    virtual uint64_t  get_uint64(int n) const = 0;  // nth value col, 0-indexed
    virtual int32_t   get_int32 (int n) const = 0;

    virtual ~NodePropCursor() = default;
};

// ─── EdgePropCursor ──────────────────────────────────────────────────────────
// Abstracts over a property scan keyed by (src, dst).
// COLUMNAR: wraps a WiredTiger colgroup cursor.
// EMBEDDED: wraps get_out_nodes_id() + get_edge_properties() per neighbor.
// Same RAII contract as NodePropCursor.
class EdgePropCursor {
public:
    // Pin to a single src.  next() stops when src changes.
    // EMBEDDED: pre-fetches get_out_nodes_id(src) on this call.
    virtual void set_src(node_id_t src) = 0;

    // Restrict to edges with src in [src_start, src_end).
    // COLUMNAR only — no EMBEDDED equivalent needed by current queries.
    virtual void set_src_range(node_id_t src_start, node_id_t src_end) = 0;

    // Advance.  Returns false at EOF or when src-pin/range condition breaks.
    virtual bool next() = 0;

    // Exact point lookup on (src, dst).  Returns false if not found.
    virtual bool seek(node_id_t src, node_id_t dst) = 0;

    virtual node_id_t src()             const = 0;
    virtual node_id_t dst()             const = 0;
    virtual uint64_t  get_uint64(int n) const = 0;

    virtual ~EdgePropCursor() = default;
};
```

---

## Step 2a — `src/wt_prop_cursor.h` (new file, COLUMNAR)

Concrete classes backed by WiredTiger colgroup cursors.  No Flexograph types needed.

### Value layout enum

WiredTiger `get_value` arg count must match the colgroup value format at compile time.
Three layouts cover all current temporal colgroups:

| Tables | Value format | Layout |
|--------|-------------|--------|
| `post_props:temporal`, `comment_props:temporal` | `(cDate:Q, length:i)` | `CVL_Qi` |
| `person_props:temporal` | `(cDate:Q, birthday:Q)` | `CVL_QQ` |
| `knows/likes/hasmember/forum_props:temporal` | `(cDate:Q)` | `CVL_Q` |

```cpp
enum ColgroupValueLayout { CVL_Q, CVL_QQ, CVL_Qi };

inline ColgroupValueLayout layout_for(const std::string& table,
                                       const std::string& cg)
{
    if (cg == CG_TEMPORAL) {
        if (table == POST_PROPS_TABLE || table == COMMENT_PROPS_TABLE)
            return CVL_Qi;
        if (table == PERSON_PROPS_TABLE)
            return CVL_QQ;
        return CVL_Q;   // knows, likes, hasmember, forum, studyat, workat
    }
    return CVL_Q;
}
```

### `WTNodePropCursor : public NodePropCursor`

```
Private:
    WT_CURSOR           *cur_
    node_id_t            range_end_    = OutOfBand_ID_MAX
    ColgroupValueLayout  layout_
    bool                 exhausted_    = false
    node_id_t            cur_key_
    uint64_t             u64_[2]
    int32_t              i32_[1]
```

Constructor `(WT_CURSOR*, ColgroupValueLayout)`.
Destructor: `cur_->close(cur_)`.

**`set_range(start, end)`:**
1. `range_end_ = end`
2. `set_key(start)` → `search_near(&cmp)`
3. `ret != 0` → `exhausted_ = true; return`
4. `cmp < 0` → `next()` on the WT cursor; `ret != 0` → `exhausted_ = true; return`
5. `load_current()` — if `cur_key_ >= range_end_` → `exhausted_ = true`

**`next()`** — consume-then-advance pattern:
1. `exhausted_` → return `false`
2. `load_current()` into cache
3. `cur_->next(cur_)` → `ret != 0` or `cur_key_ >= range_end_` → `exhausted_ = true`
4. Return `true`

**`seek(id)`:**
1. `set_key(id)` → `search()`
2. `ret != 0` → return `false`
3. `load_current()` → return `true`

**`load_current()` (private):**
```cpp
cur_->get_key(cur_, (uint64_t*)&cur_key_);
if      (layout_ == CVL_Q)  cur_->get_value(cur_, &u64_[0]);
else if (layout_ == CVL_QQ) cur_->get_value(cur_, &u64_[0], &u64_[1]);
else /* CVL_Qi */           cur_->get_value(cur_, &u64_[0], &i32_[0]);
// Use int32_t*, not int* — sizeof(int) may != 4.
```

### `WTEdgePropCursor : public EdgePropCursor`

```
Private:
    WT_CURSOR *cur_
    node_id_t  pinned_src_    = OutOfBand_ID_MAX
    node_id_t  src_range_end_ = OutOfBand_ID_MAX
    bool       exhausted_     = false
    node_id_t  cur_src_, cur_dst_
    uint64_t   u64_[1]
```

Constructor `(WT_CURSOR*)`.  Destructor: `cur_->close(cur_)`.

**`set_src(src)`:** `pinned_src_ = src`; position at `(src, 0)` via `search_near`;
advance if `cmp < 0`; `load_current()`; if `cur_src_ != src` → `exhausted_`.

**`set_src_range(s, e)`:** `pinned_src_ = OutOfBand_ID_MAX`, `src_range_end_ = e`;
position at `(s, 0)` the same way; stop condition `cur_src_ >= src_range_end_`.

**`next()`:** consume current row, advance WT cursor, re-check pin/range condition.

**`seek(src, dst)`:** `set_key(src, dst)` → `search()` → `load_current()`.

**`load_current()` (private):**
```cpp
cur_->get_key(cur_, (uint64_t*)&cur_src_, (uint64_t*)&cur_dst_);
cur_->get_value(cur_, &u64_[0]);
```

---

## Step 2b — `src/emb_prop_cursor.h` (new file, EMBEDDED)

Concrete classes that wrap the existing `get_node_properties` / `get_edge_properties`
blob API.  These need `GraphBase*` — forward-declare it here; full include is in the
`.cpp` files.

```cpp
class GraphBase;  // forward declaration — full include in .cpp only
```

### Schema dispatch

EMBEDDED blobs use the fixed-offset schemas in `prop_schema.h`.  The cursor needs to
know which field in the blob corresponds to column 0.  All current temporal colgroup
usages only read `creation_date` (column 0) and `length` (column 1 for post/comment).
A simple function pointer stored at construction covers all cases:

```cpp
using U64Extractor = uint64_t (*)(const uint8_t*);  // extracts a uint64 from blob
using I32Extractor = int32_t  (*)(const uint8_t*);  // extracts an int32  from blob
```

The factory methods pass the appropriate schema helpers when constructing EMBEDDED
cursors.  For example, for `knows_props`:
```cpp
// col 0 = creation_date
U64Extractor u64_extractors[1] = { [](const uint8_t* b){ return SNBKnowsSchema::get_creation_date(b); } };
```

For `post_props` / `comment_props`:
```cpp
U64Extractor u64_extractors[2] = { get_creation_date, ...  };
I32Extractor i32_extractors[1] = { get_length };
```

This keeps the cursor class generic without a schema-switch inside it.

### `EmbNodePropCursor : public NodePropCursor`

```
Private:
    GraphBase          *graph_
    U64Extractor        u64_ex_[2]
    I32Extractor        i32_ex_[1]
    int                 n_u64_, n_i32_
    // iteration state:
    std::unique_ptr<NodeCursor> node_cur_   // for next() sequential scan
    // point-lookup cache:
    node_id_t           cur_key_
    std::vector<uint8_t> cur_blob_
    uint64_t             u64_[2]
    int32_t              i32_[1]
```

**`set_range(start, end)`:** open a `NodeCursor` via `graph_->get_node_iter()`,
call `set_key_range({start, end})`, advance to first row.

**`next()`:** call `node_cur_->next(&n)`, call
`graph_->get_node_properties(n.id)`, run extractors into cache, return `true`/`false`.

**`seek(id)`:** call `graph_->get_node_properties(id)`; run extractors; return
`false` if blob is empty (node not found).

### `EmbEdgePropCursor : public EdgePropCursor`

```
Private:
    GraphBase          *graph_
    std::string         table_       // used only for get_edge_properties routing
    U64Extractor        u64_ex_[1]
    // iteration state for set_src:
    node_id_t           pinned_src_
    std::vector<node_id_t> neighbors_  // pre-fetched by set_src
    size_t              idx_
    // current row cache:
    node_id_t           cur_src_, cur_dst_
    uint64_t            u64_[1]
```

**`set_src(src)`:**
1. `pinned_src_ = src`
2. `neighbors_ = graph_->get_out_nodes_id(src)` — pre-fetch all neighbors
3. `idx_ = 0`; advance to first neighbor whose edge type matches the table
   (i.e., skip neighbors of wrong vertex type using `VTYPE_OF`)
4. Load first row into cache

**`next()`:** advance `idx_`, call `graph_->get_edge_properties(pinned_src_,
neighbors_[idx_])`, run extractor, cache result, return `true`/`false`.

**`set_src_range`:** not needed by any current EMBEDDED query.  Throw
`GraphException("set_src_range not supported in EMBEDDED mode")` — it will
never be called since BI parallel variants only run in COLUMNAR mode.

**`seek(src, dst)`:** call `graph_->get_edge_properties(src, dst)`, run
extractor, return `false` if blob is empty.

---

## Step 3 — `src/graph.h`

Remove:
```cpp
virtual WT_CURSOR* open_colgroup_cursor(const std::string& table,
                                         const std::string& colgroup) = 0;
```

Add (below `get_edge_iter`):
```cpp
virtual std::unique_ptr<NodePropCursor> get_node_prop_cursor(
    const std::string& table, const std::string& colgroup) = 0;

virtual std::unique_ptr<EdgePropCursor> get_edge_prop_cursor(
    const std::string& table, const std::string& colgroup) = 0;
```

Add `#include <memory>` and explicit `#include "iterator.h"` at top.

---

## Step 4 — `src/edgekey_split.h` and `src/adj_list.h`

In each file:
- Add `#include "wt_prop_cursor.h"` and `#include "emb_prop_cursor.h"`
- Replace `open_colgroup_cursor` override declaration with:

```cpp
std::unique_ptr<NodePropCursor> get_node_prop_cursor(
    const std::string& table, const std::string& colgroup) override;
std::unique_ptr<EdgePropCursor> get_edge_prop_cursor(
    const std::string& table, const std::string& colgroup) override;
```

No concrete cursor class definitions in these files.

---

## Step 5 — `src/edgekey_split.cpp` and `src/adj_list.cpp`

Add `#include "graph.h"` in `emb_prop_cursor.h` implementations (or include it
directly in the `.cpp` — the forward declaration in the header is sufficient for the
header; the full definition is needed only in the `.cpp`).

Delete `open_colgroup_cursor` implementations.

Factory method bodies dispatch on `opts.prop_mode`:

```cpp
std::unique_ptr<NodePropCursor>
SplitEdgeKey::get_node_prop_cursor(const std::string& table,
                                    const std::string& colgroup)
{
    if (opts.prop_mode == COLUMNAR) {
        std::string uri = "colgroup:" + table + ":" + colgroup;
        WT_CURSOR *c = nullptr;
        int ret = session->open_cursor(session, uri.c_str(), nullptr, nullptr, &c);
        if (ret != 0)
            throw GraphException("open_cursor failed for " + uri + ": "
                                 + wiredtiger_strerror(ret));
        return std::make_unique<WTNodePropCursor>(c, layout_for(table, colgroup));
    } else {
        return make_emb_node_prop_cursor(this, table, colgroup);
        // make_emb_node_prop_cursor: free function in emb_prop_cursor.h that
        // selects the right extractor functions from prop_schema.h and constructs
        // an EmbNodePropCursor.
    }
}

std::unique_ptr<EdgePropCursor>
SplitEdgeKey::get_edge_prop_cursor(const std::string& table,
                                    const std::string& colgroup)
{
    if (opts.prop_mode == COLUMNAR) {
        std::string uri = "colgroup:" + table + ":" + colgroup;
        WT_CURSOR *c = nullptr;
        int ret = session->open_cursor(session, uri.c_str(), nullptr, nullptr, &c);
        if (ret != 0)
            throw GraphException("open_cursor failed for " + uri + ": "
                                 + wiredtiger_strerror(ret));
        return std::make_unique<WTEdgePropCursor>(c);
    } else {
        return make_emb_edge_prop_cursor(this, table, colgroup);
        // make_emb_edge_prop_cursor: selects extractor from table name, constructs
        // EmbEdgePropCursor.
    }
}
```

`AdjList::` versions are identical — substitute `AdjList::` for `SplitEdgeKey::`.

### `make_emb_edge_prop_cursor` extractor selection

```cpp
inline std::unique_ptr<EdgePropCursor>
make_emb_edge_prop_cursor(GraphBase* g, const std::string& table,
                           const std::string& /*colgroup*/)
{
    U64Extractor ex = nullptr;
    if      (table == KNOWS_PROPS_TABLE)     ex = SNBKnowsSchema::get_creation_date;
    else if (table == LIKES_PROPS_TABLE)     ex = SNBLikesSchema::get_creation_date;
    else if (table == HASMEMBER_PROPS_TABLE) ex = SNBHasMemberSchema::get_creation_date;
    else throw GraphException("EMBEDDED cursor not implemented for table: " + table);
    return std::make_unique<EmbEdgePropCursor>(g, table, ex);
}
```

Similarly for `make_emb_node_prop_cursor`.

---

## Step 6 — `test/ldbc_snb_queries.cpp`

Two changes happen in this step:

### 6a — Rewrite the 22 COLUMNAR call sites (same as before)

All patterns are identical to the original plan.  See patterns A–E below.

### 6b — Remove the EMBEDDED else-branches

Queries R2, A2, A3 currently have:
```cpp
if (has_props && prop_mode == COLUMNAR) {
    // colgroup cursor path
} else {
    // get_out_nodes_id + get_edge_properties per neighbor
}
```

After this step, both branches collapse into the same cursor call:
```cpp
// R2 — works in both EMBEDDED and COLUMNAR:
auto cur = graph.get_edge_prop_cursor(KNOWS_PROPS_TABLE, CG_TEMPORAL);
cur->set_src(pid);
while (cur->next())
    result.emplace_back(cur->dst(), (int64_t)cur->get_uint64(0));
```

The `prop_mode` parameter can be removed from R2/A2/A3 signatures entirely.

Queries IC-7/8/9/IC-5 gain a working EMBEDDED path for the first time — the
`has_props` guard becomes the only condition, and the cursor handles both modes
internally.

### Pattern A — full sequential node-prop scan
Lines: 965, 1004, 2126

```cpp
// Before (10 lines):
WT_CURSOR *cur = graph.open_colgroup_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
while (cur->next(cur) == 0) {
    uint64_t vid, cDate; int32_t length;
    cur->get_key(cur, &vid);
    cur->get_value(cur, &cDate, &length);
}
cur->close(cur);

// After (5 lines):
auto cur = graph.get_node_prop_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
while (cur->next()) {
    node_id_t vid  = cur->key();
    uint64_t cDate = cur->get_uint64(0);
    int32_t  len   = cur->get_int32(0);
}
```

### Pattern B — partitioned sequential scan (parallel BI variants)
Lines: 1121, 1245, 1332

```cpp
// Before (12 lines with manual search_near + cmp):
WT_CURSOR *cg = handle->open_colgroup_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
cg->set_key(cg, (uint64_t)start_id);
int cmp = 0; bool ok = (cg->search_near(cg, &cmp) == 0);
if (ok && cmp < 0) ok = (cg->next(cg) == 0);
while (ok) { ... if (vid >= end_id) break; ... ok = (cg->next(cg) == 0); }
cg->close(cg);

// After (6 lines):
auto cur = handle->get_node_prop_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
cur->set_range(start_id, end_id);
while (cur->next()) {
    uint64_t cDate = cur->get_uint64(0);
    int32_t  len   = cur->get_int32(0);
}
```

Note: parallel BI variants only run in COLUMNAR mode (asserted before the parallel
section).  `set_src_range` is COLUMNAR-only; this is Pattern B not Pattern C.

### Pattern C — src-pinned edge range scan
Lines: 520 (R2), 844 (A2), 899 (A3)

```cpp
// Before (14 lines + EMBEDDED else-branch):
if (has_props && prop_mode == COLUMNAR) {
    WT_CURSOR *cur = graph.open_colgroup_cursor(KNOWS_PROPS_TABLE, CG_TEMPORAL);
    cur->set_key(cur, (uint64_t)pid, (uint64_t)0); /* search_near boilerplate */
    while (...) { result.emplace_back(dst, cDate); }
    cur->close(cur);
} else {
    for (node_id_t nb : graph.get_out_nodes_id(pid)) {
        prop_blob pb = graph.get_edge_properties(pid, nb);
        result.emplace_back(nb, decode_knows(pb).creation_date);
    }
}

// After (4 lines, both modes):
auto cur = graph.get_edge_prop_cursor(KNOWS_PROPS_TABLE, CG_TEMPORAL);
cur->set_src(pid);
while (cur->next())
    result.emplace_back(cur->dst(), (int64_t)cur->get_uint64(0));
```

### Pattern D — batch sorted point lookup on node-prop
Lines: 720 (X3), 1518+1519 (IC-9), 1585 (IC-8), 1851 (X3 cached), 1953+1954 (IC-9 cached)

```cpp
// Before (7 lines per batch):
WT_CURSOR *cg = graph.open_colgroup_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
for (node_id_t post_id : candidate_posts) {
    cg->set_key(cg, (uint64_t)post_id);
    if (cg->search(cg) != 0) continue;
    uint64_t cDate; int32_t length;
    cg->get_value(cg, &cDate, &length);
}
cg->close(cg);

// After (4 lines):
auto cg = graph.get_node_prop_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
for (node_id_t post_id : candidate_posts) {
    if (!cg->seek(post_id)) continue;
    uint64_t cDate = cg->get_uint64(0);
    int32_t  len   = cg->get_int32(0);
}
```

For **dual-cursor** sites (IC-9, lines 1518/1519, 1953/1954) — both cursors are
`unique_ptr<NodePropCursor>`; use a reference for the ternary:
```cpp
auto post_cg    = graph.get_node_prop_cursor(POST_PROPS_TABLE,    CG_TEMPORAL);
auto comment_cg = graph.get_node_prop_cursor(COMMENT_PROPS_TABLE, CG_TEMPORAL);
for (auto &[msg, creator] : msg_creator) {
    auto &cg = (VTYPE_OF(msg) == VT_POST) ? post_cg : comment_cg;
    if (!cg->seek(msg)) continue;
    /* cg->get_uint64(0), cg->get_int32(0) */
}
// no close() calls needed
```

### Pattern E — batch sorted point lookup on edge-prop
Lines: 1456 (IC-7), 1708 (IC-5), 1903 (IC-7 cached), 2005 (IC-5 cached)

```cpp
// Before (7 lines):
WT_CURSOR *cg = graph.open_colgroup_cursor(LIKES_PROPS_TABLE, CG_TEMPORAL);
for (auto &[liker, msg] : liker_msg) {
    cg->set_key(cg, (uint64_t)liker, (uint64_t)msg);
    if (cg->search(cg) != 0) continue;
    uint64_t cDate; cg->get_value(cg, &cDate);
    result.emplace_back(liker, msg, (int64_t)cDate);
}
cg->close(cg);

// After (4 lines):
auto cg = graph.get_edge_prop_cursor(LIKES_PROPS_TABLE, CG_TEMPORAL);
for (auto &[liker, msg] : liker_msg) {
    if (!cg->seek(liker, msg)) continue;
    result.emplace_back(liker, msg, (int64_t)cg->get_uint64(0));
}
```

---

## Step 7 — Unit tests

**`test/test_columnar_ekey.cpp`** and **`test/test_columnar_adj.cpp`:**
One function each uses `open_colgroup_cursor` for a src-pinned edge scan.
Rewrite using Pattern C above.

**`test/test_columnar_full_ekey.cpp`** and **`test/test_columnar_full_adj.cpp`:**
No changes needed.

---

## Build / test checkpoints

```bash
# After Steps 1–3 only: expected compile errors (backends not updated)
cd flexograph_code/build && make -j$(nproc) 2>&1 | grep "error:"

# After Steps 4–5: full build clean; run tests that don't use the old API
make -j$(nproc)
./test/test_columnar_full_ekey; echo "exit: $?"   # should pass
./test/test_columnar_full_adj;  echo "exit: $?"   # should pass

# Capture baseline before touching query file
./test/ldbc_snb_queries <data_dir> splitekey 2>&1 > baseline_splitekey.txt
./test/ldbc_snb_queries <data_dir> adj       2>&1 > baseline_adj.txt

# After Steps 6–7: full suite
make -j$(nproc)
./test/test_columnar_ekey;      echo "exit: $?"   # should pass
./test/test_columnar_adj;       echo "exit: $?"   # should pass
./test/ldbc_snb_queries <data_dir> splitekey 2>&1 | diff - baseline_splitekey.txt
./test/ldbc_snb_queries <data_dir> adj       2>&1 | diff - baseline_adj.txt

# Verify EMBEDDED mode also works end-to-end
./test/ldbc_snb_queries <data_dir> splitekey --embedded 2>&1 | grep -E "ms|ERROR"
./test/ldbc_snb_queries <data_dir> adj       --embedded 2>&1 | grep -E "ms|ERROR"
```

---

## Edge cases to not miss

1. **`set_src` / `set_range` on empty table (WT):** `search_near` returns
   `WT_NOTFOUND` — set `exhausted_ = true` immediately, do not call `next()`.

2. **`set_src` empty neighbor list (EMBEDDED):** `get_out_nodes_id` returns `{}` —
   `idx_ = 0`, nothing to iterate, `next()` returns `false` immediately.

3. **`EmbEdgePropCursor::set_src` vertex-type filtering:** `get_out_nodes_id(src)`
   returns all out-neighbors regardless of edge type.  The EMBEDDED cursor must
   skip neighbors whose `VTYPE_OF(dst)` doesn't match the expected destination type
   for the table (e.g., `likes_props` expects `VT_POST` or `VT_COMMENT` destinations;
   `knows_props` expects `VT_PERSON`).  Pass an expected-dst-type mask to the
   constructor alongside the extractor function.

4. **`EmbEdgePropCursor::seek` empty blob:** `get_edge_properties` returns an empty
   blob if the edge doesn't exist.  Check `pb.size == 0` → return `false`.

5. **`CVL_Qi` `get_int32`:** use `int32_t*` not `int*` in WT `get_value` call.

6. **`unique_ptr` in OMP loop:** each thread's cursor declared inside loop body,
   destroyed at iteration end — correct and thread-safe.

7. **Dual-cursor ternary (IC-9):** `auto &cg = ... ? post_cg : comment_cg` is a
   reference to `unique_ptr<NodePropCursor>` — non-copyable, reference is correct.

8. **`set_src_range` EMBEDDED:** throw `GraphException` — no current EMBEDDED query
   needs it, and it would require a different iteration strategy.

9. **`layout_for` for studyat/workat:** `value_format=i` (int32 only).  No query
   currently scans these colgroups, but `layout_for` must not return a wrong layout.
   Add a `CVL_i` case or note it as unimplemented.

---

## Summary

| Step | File(s) | Net change |
|------|---------|-----------|
| 1 | `src/iterator.h` | +50 lines |
| 2a | `src/wt_prop_cursor.h` (new) | +180 lines |
| 2b | `src/emb_prop_cursor.h` (new) | +150 lines |
| 3 | `src/graph.h` | ±5 lines |
| 4 | `src/edgekey_split.h`, `src/adj_list.h` | ±5 lines each |
| 5 | `src/edgekey_split.cpp`, `src/adj_list.cpp` | −20/+40 lines each |
| 6 | `test/ldbc_snb_queries.cpp` | −200/+80 lines (net −120) |
| 7 | `test/test_columnar_ekey.cpp`, `test/test_columnar_adj.cpp` | −10/+6 each |

~850 lines total touched.  No behavioral changes for COLUMNAR mode.  EMBEDDED mode
gains working implementations for IC-7/8/9/IC-5 (previously COLUMNAR-only).
`prop_mode` parameter removed from R2/A2/A3 signatures.  No `WT_CURSOR*` or
`if (prop_mode == COLUMNAR)` in query or test code after this change.
