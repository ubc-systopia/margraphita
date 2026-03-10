# Bug Log — Flexograph Property Storage Implementation

Bugs found and fixed during Phase 0 and Phase 1 of LDBC SNB property storage work.

---

## Bug 1: `degree_cursor` declared but never initialized

**File:** `src/edgekey_split.h:96`, `src/edgekey_split.cpp:init_cursors()`

**Symptom:** `update_node_degree` lazily created a fresh WT cursor each call; `set_node_properties` would segfault if called before the first `add_edge` (lazy init never triggered).

**Root cause:** Cursor declared as class member, initialized to `nullptr`, but `init_cursors()` never opened it. The lazy-init fallback in `update_node_degree` masked the bug during normal graph operations, but `set_node_properties` had no fallback and would dereference the null pointer.

**Fix:** Open `degree_cursor` eagerly in `init_cursors()` via `_get_table_cursor(OUT_EDGES, ...)`.

---

## Bug 2: `pack_values` — heap allocation with free() commented out (memory leak)

**File:** `src/edgekey_split.h:pack_values` template

**Symptom:** Every `add_node` / `add_edge` / `update_node_degree` call leaked `sizeof(T) * count` bytes.

**Root cause:** Template used `new T[count]` to build a temporary buffer for `WT_ITEM.data`. The matching `free()` was commented out because it was incorrectly placed after `cursor->insert()` was called (WT reads the data during insert, so free was safe after `insert()`, but the author was uncertain and left it commented).

**Fix:** `thread_local T buffer[4]` — no allocation, no leak. Safe because cursor operations always complete before the next `pack_values` call on the same thread. The array size of 4 is sufficient for the largest value tuple (in_degree, out_degree, edge_weight, padding).

---

## Bug 3: GraphBase implicit copy constructor shared raw WT pointers

**File:** `src/graph.h`, `GraphBase` class

**Symptom:** `test_update_edge(SplitEdgeKey graph, ...)` (pass by value) silently copied the graph object. Both the original and the copy shared the same `WT_SESSION*` and `WT_CURSOR*`. When the copy's destructor ran at end of scope, WT's internal cursor state was corrupted for the still-live original object.

**Root cause:** Compiler-generated copy constructor does a shallow copy; raw pointers are shared between instances. WiredTiger cursors and sessions are not reference-counted and cannot be safely shared.

**Fix:** `= delete` on copy constructor, move constructor, copy assignment, and move assignment in `GraphBase`. This causes a compile-time error for any code that accidentally copies a graph object.

---

## Bug 4: All test functions took graph objects by value

**File:** `test/test_split_edgekey.cpp`, `test/test_adj_list.cpp` (20+ functions)

**Symptom:** Same as Bug 3 — every test helper function implicitly copied the graph at the call site, triggering cursor sharing and eventual corruption. With the `= delete` fix for Bug 3, these became compile errors, making the scope of the problem visible.

**Fix:** Changed all signatures from `AdjList graph` / `SplitEdgeKey graph` to `AdjList &graph` / `SplitEdgeKey &graph`. No logic changes were needed; the functions never intended to modify the graph object's ownership.

---

## Bug 5: `CommonUtil::close_cursor` did not guard against nullptr

**File:** `src/common_util.cpp:close_cursor()`

**Symptom:** Calling `close_cursor(nullptr)` (e.g. when optional cursors such as `node_props_cursor` and `edge_props_cursor` were never opened because the feature was disabled) would dereference null and crash in `close_all_cursors()`.

**Fix:** Added `if (cursor == nullptr) return 0;` guard at function entry.

---

## Bug 6: WiredTiger double-free when passing `{nullptr, size=0}` WT_ITEM

**File:** `src/edgekey_split.cpp:add_edge()`, `src/edgekey_split.cpp:set_edge_properties()`

**Symptom:** `free(): double free detected in tcache 2` on `propEngine.close_graph()` after property tests. Confirmed by AddressSanitizer (ASAN) as a double-free inside `__wt_buf_free → __wt_cursor_close`.

**Root cause:** When `cursor->set_value(cursor, &item)` is called with `item.size=0` and `item.data=nullptr`, WiredTiger takes ownership of the value buffer pointer into its internal cursor value structure (`WT_ITEM`). It frees this buffer when the cursor's value is replaced or when the cursor is closed — but it does NOT null out the pointer after the first free. For a zero-size item with null data, the WT internal free path calls `free(NULL)` on first close (which is defined behavior and a no-op in glibc). However, in the specific WT version used, the cursor's internal `__wt_buf_free` path frees the stale pointer a second time on cleanup.

**Fix:** Never pass `size=0` to WiredTiger set_value. Use a 1-byte sentinel `{&placeholder, 1}` for "no properties" edges (edges where `has_edge_props` is false, or edges that genuinely have no property data). In `get_edge_properties`, treat `item.size <= 1` as "no properties" and return `{nullptr, 0}` to callers.

**Affected locations:**
- 2 places in `add_edge` (OUT_EDGES cursor branch and IN_EDGES cursor branch)
- 1 place in `set_edge_properties` (null data guard at function entry)
- Same sentinel pattern applied to AdjList's `set_node_properties` and `set_edge_properties` for safety
