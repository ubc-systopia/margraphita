# Property Storage Implementation Rationale

This document explains the key design decisions behind the PoC implementation of property storage in Flexograph, relative to what is proposed in Chapter 6 of the thesis ("Storing Property Graphs").

---

## A. Why LDBC SNB Instead of FinBench

The thesis originally proposed using the LDBC Financial Benchmark (FinBench) as the evaluation workload. LDBC SNB is strictly better for this PoC for two reasons.

**1. No multi-edge problem.**

FinBench's `transfer` edge type allows arbitrarily many transfers between the same pair of `Account` nodes — multiple edges with identical `(src, dst)`. This breaks SplitEdgeKey's fundamental invariant: its primary key is `(src, dst)`, which must be unique per table. Supporting FinBench's transfer edge would require a composite key extension (e.g., `(src, dst, edge_seq)`) — a fundamental schema change to the storage layer, not a property-storage concern.

The LDBC SNB specification explicitly prohibits multi-edges: *"Multiple edges (i.e., edges of the same type between two entity instances) are not allowed in SNB graphs."* This means SplitEdgeKey's `(src, dst)` key works for every edge type in SNB without any modification.

**2. Academic credibility.**

LDBC SNB is the de facto standard graph benchmark in the academic literature (published at VLDB, used in GraphflowDB and dozens of other system papers). FinBench is newer and less widely cited. For a dissertation evaluation, SNB provides a stronger basis for comparison with prior work.

The property storage research question — embedded inline vs. split column-store — is identical under both benchmarks. Switching to SNB is purely upside.

---

## B. Engineering Challenges We Are Sidestepping

The thesis proposes a comparative study of two property storage strategies: **embedded** (properties stored inline as a byte blob attached to each vertex/edge record) and **split** (properties stored in separate WiredTiger column groups, one B-tree per property column). We implement only the embedded approach.

Specifically, the following challenges described or implied by the thesis are **not** addressed in this PoC:

| Challenge | Status |
|-----------|--------|
| **Split/columnar property storage** | ✅ Implemented as `PropStorageMode::COLUMNAR` (commits `a01fb19`, `55e754a`, `8b74203`). Per-type typed tables (`person_props`, `post_props`, `knows_props`, `likes_props`) with WiredTiger column groups. `set/get_node/edge_properties` COLUMNAR branches in both SplitEdgeKey and AdjList. |
| **Comparative benchmark study** | ⚠️ Partial. COLUMNAR mode works end-to-end and BI-1/BI-12 are timed. The EMBEDDED vs COLUMNAR head-to-head comparison for R2/A2/A3 (the thesis experiment) is not yet wired up. See Section G Phase 1. |
| **API as specified in Table 6.2** | Not implemented. Raw byte blobs + hand-rolled schema namespaces. The string-key API is deferred. |
| **Full LDBC SNB schema** | Not implemented. 2 of ~11 vertex types (Person, Post), 3 of 20 edge types. See Section G Phase 3. |
| **String properties** | Not implemented. Fixed-size integers only; string fields silently dropped. |
| **Multi-valued attributes** | Not implemented. `Person.email` and `Person.speaks` not stored. |
| **Schema-agnostic serialization** | Not implemented. Hardcoded `SNBPersonSchema` / `SNBPostSchema` structs. |
| **Delete operations** | Not implemented. |
| **Indexing by label/property** | Not implemented. Explicitly deferred in the thesis. |

---

## C. Which Section 6.1 Decisions Are Handled

Section 6.1 ("Example Property Graph Schema") lists six decisions required to translate a property graph schema into a Flexograph-compatible representation. Here is where each stands:

### Decision 1 — Vertex property storage (inline vs. column store)
**Partially handled.** Embedded/inline storage is implemented: `set_node_properties` writes a fixed-size blob into the WiredTiger node row, and `get_node_properties` reads it back as a `prop_blob`. Split column-store storage (WT column groups with one column per property field) is **not implemented**. Reading a single field (e.g., `gender`) still requires deserializing the full 17-byte blob.

### Decision 2 — Edge property storage (inline vs. column store)
**Partially handled.** Same situation as Decision 1. `set_edge_properties` / `get_edge_properties` read and write a single opaque blob. The split variant is not implemented.

### Decision 3 — Non-unique vertex IDs across vertex types
**Fully handled.** We implement ID-space partitioning: Person IDs are assigned from `[0, person_count)` and Post IDs from `[person_count, person_count + post_count)`. The `LDBCLoader` maintains `unordered_map<int64_t, node_id_t>` remapping tables for both types, and the query harness uses `is_person(id)` / `is_post(id)` helpers to recover vertex type from the ID at query time. No additional column is needed in the storage layer.

### Decision 4 — Edges of different types with identical (src, dst)
**Handled by benchmark selection.** FinBench is where this decision is hard (multiple `transfer` edges per account pair). LDBC SNB's guarantee of no multi-edges means that within each edge type the `(src, dst)` key is unique. Additionally, all three edge types in our subset connect *distinct endpoint-type pairs*:
- `knows`: Person → Person
- `likes`: Person → Post
- `hasCreator`: Post → Person

Since endpoint ID ranges are disjoint, the edge type is fully determined by the `(src, dst)` ID ranges — no disambiguation column needed. The `(src, dst)` primary key works for all three edge types stored in the same SplitEdgeKey instance.

### Decision 5 — Indexing based on labels and properties
**Not handled.** This decision is explicitly deferred to a future chapter in the thesis itself ("we will discuss in Chapter X on specialized indexing"). No secondary indexes on properties are implemented.

### Decision 6 — Schema-agnostic index creation
**Not handled.** The serialization layer is hardcoded: `SNBPersonSchema`, `SNBPostSchema`, `SNBKnowsSchema`, and `SNBLikesSchema` are manually written C++ namespace structs with fixed byte offsets. Adding a new vertex type requires writing a new namespace by hand. There is no schema registry or automatic index generation.

---

## D. How We Tackle the Label Storage Problem

Section 6.4 of the thesis ("Label Storage") identifies three approaches and recommends the third:

| Approach | Description | Status |
|----------|-------------|--------|
| **Duplicating representations** | One SplitEdgeKey/AdjList instance per vertex type | Not used — complicates code and requires inter-table jumps for cross-type edges |
| **Embedding a type attribute** | Store `(src_type, dst_type)` as a column in each edge table | Not used — requires scanning full neighborhood to isolate by type |
| **Vertex re-labelling (ID-space partitioning)** | Assign disjoint ID ranges per type; type is implicit in the ID value | **This is what we implement** |

The thesis describes vertex re-labelling as "currently implemented outside Flexograph as a user-space framework on top of the Graph API." Our implementation matches this exactly:

- **At load time** (`ldbc_snb_loader.h`): `LDBCLoader` assigns compact IDs sequentially — persons first (`[0, person_count)`), then posts (`[person_count, person_count + post_count)`). The LDBC original IDs (arbitrary large integers) are remapped via `person_id_map` and `post_id_map`. Flexograph itself sees only the remapped IDs and is unaware of types.

- **At query time** (`ldbc_snb_queries.cpp`): Two inline helpers recover the type from the ID:
  ```cpp
  static inline bool is_person(node_id_t id, node_id_t person_count)
      { return id < person_count; }
  static inline bool is_post(node_id_t id, node_id_t person_count, node_id_t post_count)
      { return id >= person_count && id < person_count + post_count; }
  ```
  The property schema to use is selected based on `is_person` / `is_post`. For edges, the `(src_type, dst_type)` pair uniquely identifies the edge schema (knows vs. likes vs. hasCreator) without any stored discriminator.

- **Benefit realized**: When scanning the out-edges of a Person vertex, `knows` edges (dst in Person range) and `likes` edges (dst in Post range) appear in sorted key order within the same B-tree. A key-range scan `[person_id, person_count)` isolates all `knows` edges; `[person_count, ∞)` isolates all `likes` edges. This is the "sequential access" advantage the thesis diagrams in Figure 6.6.

The main limitation acknowledged in the thesis also applies here: changing a vertex's type requires deleting and re-inserting it with all connected edges, since the ID encodes the type.

---

## E. Implementing Column-Group (Split) Property Storage

This section describes what would need to change to implement the split storage mode proposed in the thesis. The central idea is: instead of packing all properties into a single opaque byte blob per row, store each property (or group of related properties) in its own WiredTiger B-tree via **column groups**. A cursor opened with a projection like `table:person_props(creation_date)` then reads only the B-tree file for `creation_date`, skipping the files for `birthday` and `gender` entirely. This is the I/O isolation that the split mode hypothesis relies on.

### Why the Current Tables Cannot Have Column Groups Added Directly

WiredTiger column groups require a **typed value format**. A column group declaration names specific columns, and columns must have declared types (`q` = int64, `i` = int32, `b` = int8, etc.). The current tables use `value_format=u`, a single opaque byte string with no named columns. You cannot declare column groups on a `value_format=u` table — WiredTiger will reject the `colgroup:` creation with an error.

Additionally, the current `OUT_EDGES` / `IN_EDGES` tables store **both node rows and edge rows** in the same B-tree, distinguished by `dst = OutOfBand_ID_MIN` for node sentinels. Node rows carry `{in_deg (4B) | out_deg (4B) | prop_blob}` and edge rows carry `{prop_blob}`. A single typed `value_format` cannot accommodate both layouts without adding NULL-able or unused columns to one of the row types.

This means split storage requires **separate property tables**, not column groups bolted onto the existing edge tables.

### Required Changes, Layer by Layer

#### 1. New property tables (in `create_wt_tables`)

When `prop_mode == SPLIT`, create dedicated typed tables and their column groups alongside the existing structural tables. The structural `edge_out` / `edge_in` tables remain unchanged — they continue to store node degrees in the sentinel row and edge structure (no properties).

```
Person property table:
  table:person_props  key_format=u  value_format=qqb
  columns=(node_id, creation_date, birthday, gender)
  colgroup:person_props/cg_creation_date  columns=(creation_date)
  colgroup:person_props/cg_birthday       columns=(birthday)
  colgroup:person_props/cg_gender         columns=(gender)

Post property table:
  table:post_props  key_format=u  value_format=qi
  columns=(node_id, creation_date, length)
  colgroup:post_props/cg_creation_date  columns=(creation_date)
  colgroup:post_props/cg_length         columns=(length)

Edge property table (shared by knows and likes — same schema):
  table:edge_props  key_format=uu  value_format=q
  columns=(src, dst, creation_date)
  colgroup:edge_props/cg_creation_date  columns=(creation_date)
  (single column — colgroup is trivial here, but still required to match the typed format)
```

hasCreator has no properties and needs no table.

#### 2. New cursor members in `SplitEdgeKey`

The class needs additional cursors opened against the property tables. The key design choice is whether to expose **full-row cursors** only (for writes and all-property reads) or also **projection cursors** (for single-column reads that skip unused column groups):

```cpp
// Full-row write/read cursors
WT_CURSOR *person_props_cursor = nullptr;
WT_CURSOR *post_props_cursor   = nullptr;
WT_CURSOR *edge_props_cursor   = nullptr;

// Projection cursors for column-group benefit (read one column only)
// Opened as: session->open_cursor(session, "table:person_props(creation_date)", ...)
WT_CURSOR *person_creation_date_cursor = nullptr;
WT_CURSOR *edge_creation_date_cursor   = nullptr;
```

All of these are opened in `init_cursors()` when `prop_mode == SPLIT`, and closed in `close_all_cursors()`.

#### 3. Changes to `set_node_properties`

In SPLIT mode, instead of embedding the blob after the degree bytes in the sentinel row, write to the appropriate typed property table. The method already receives the raw blob; it needs to deserialize it using `SNBPersonSchema` / `SNBPostSchema` to extract individual typed values:

```
if prop_mode == SPLIT:
    if id < person_count:
        deserialize blob → (creation_date, birthday, gender) via SNBPersonSchema
        set key on person_props_cursor
        set typed value: cursor->set_value(cursor, creation_date, birthday, gender)
        cursor->insert or update
    else:
        deserialize blob → (creation_date, length) via SNBPostSchema
        write to post_props_cursor similarly
```

The schema-type routing requires `person_count` from `graph_opts` — this is already stored there.

#### 4. Changes to `get_node_properties`

For the **all-columns read path** (needed for SR-1: person profile lookup):
- Read from `person_props_cursor` or `post_props_cursor`, extract typed values, pack into a blob using `SNBPersonSchema::set_*` helpers, return as `prop_blob`. This is a full-row read that hits all three column group B-trees — no I/O benefit over embedded mode.

For the **single-column read path** (where split mode shows its advantage):
- Open a cursor with a projection string, e.g., `table:person_props(creation_date)`. WiredTiger will only read the `cg_creation_date` B-tree file.
- This cannot be expressed through the current `prop_blob get_node_properties(node_id_t id)` virtual API — it has no way to specify which column to return.

#### 5. Changes to `set_edge_properties` and `get_edge_properties`

In SPLIT mode, write to / read from `edge_props_cursor`. The edge property tables use typed columns (`q` for `creation_date`), so `set_edge_properties` deserializes the blob to extract the int64 and writes it as a typed column value. `get_edge_properties` reads the typed int64 and packs it back into a blob for compatibility.

#### 6. Changes to `add_node` and `add_edge`

No changes to structure. In SPLIT mode:
- `add_node_txn` still writes the sentinel row `(node_id, 0)` with degrees into `edge_out`. It does **not** append property bytes (that is handled by the deferred `flush_node_props()` call, same as in embedded mode).
- `add_edge` still writes the 1-byte sentinel into `edge_out` / `edge_in` as the edge value (retaining the double-free guard from Bug 6). The actual properties are written later via `flush_edge_props()` → `set_edge_properties`.

The loading order constraint (add_node → add_edge → flush_node_props → flush_edge_props) remains identical.

### The API Challenge: Exposing Column-Group Benefit to Callers

This is the hardest design problem. The current virtual API:

```cpp
prop_blob get_node_properties(node_id_t id)   // returns ALL properties
prop_blob get_edge_properties(node_id_t src, node_id_t dst)
```

always reads the full row. For the benchmark to demonstrate split's advantage on queries like SR-3 ("friends sorted by creationDate" — reads only `creation_date` from every edge), the query code needs to read **only the creation_date column group**, not all columns.

Three options, from least to most invasive:

| Option | Approach | Invasiveness |
|--------|----------|--------------|
| **A. Expose typed cursors directly** | Add public getters `get_edge_props_cursor()` / `get_person_props_cursor("projection")` to `SplitEdgeKey`; query harness uses them directly. Bypasses the blob API entirely for SPLIT mode queries. | Low — no changes to `GraphBase` or `AdjList` |
| **B. Add a projection parameter** | New virtual method: `prop_blob get_node_property(node_id_t id, const char* col_name)`. Returns only the requested column's bytes. Requires implementing in every `GraphBase` subclass. | Medium |
| **C. Schema-typed getters** | `int64_t get_person_creation_date(node_id_t id)` — typed per-property methods. Natural for applications but completely non-generic. | High — defeats the point of a generic storage API |

**Recommended for the PoC:** Option A. The query harness (`ldbc_snb_queries.cpp`) already knows it's working with `SplitEdgeKey`, not a generic `GraphBase*`. Exposing named projection cursors from `SplitEdgeKey` is a clean way to demonstrate the column-group benefit without redesigning the virtual API. The tradeoff is that the performance measurement is specific to `SplitEdgeKey` and cannot be compared against `AdjList` in split mode without additional work.

### Implementation Note: SPLIT → COLUMNAR rename

The plan below used the enum value `SPLIT`. The implementation uses `COLUMNAR`
(`PropStorageMode::COLUMNAR`) to distinguish the new separate-typed-tables design
from a hypothetical "split" of the existing embedded blob. Semantically identical.
The table name for edge properties was also split per type (`knows_props`,
`likes_props`) rather than the single `edge_props` table described below.

### Summary of Work Items

| # | Work Item | Status | Commit |
|---|-----------|--------|--------|
| 1 | Create per-type typed tables + colgroups in `create_wt_tables()` | ✅ Done | `a01fb19` (+`55e754a` fix) |
| 2 | Add cursor members + `init_cursors()` + `close_all_cursors()` in SplitEdgeKey and AdjList | ✅ Done | `a01fb19` |
| 3 | `set/get_node_properties` COLUMNAR branch (both backends) | ✅ Done | `a01fb19` |
| 4 | `set/get_edge_properties` COLUMNAR branch (both backends) | ✅ Done | `a01fb19` (+`8b74203` sentinel fix) |
| 5 | Expose projection cursors from `SplitEdgeKey` | ✅ Done (via `open_colgroup_cursor`) | `a01fb19` |
| 6 | Query variants using projection cursor: R2, A2, A3 | ❌ Not done | — |
| 7 | Dual-mode timing comparison (EMBEDDED vs COLUMNAR) in `main()` | ❌ Not done | — |
| 8 | BI-1 posting summary (colgroup sequential scan) | ✅ Done | `a01fb19` |
| 9 | BI-12 slow baseline (point-lookup per post) | ✅ Done | `a01fb19` |
| 10 | BI-12 fast (two-pass sequential scan, 26.7× speedup) | ✅ Done | `8b74203` |

Items 6 and 7 are the remaining core work for the thesis comparison. See Section G.

### What the Comparison Will Measure

The benchmark runs each property-reading query twice (once per mode) and prints both:

- **EMBEDDED mode**: `get_node_properties(id)` reads the full 17-byte Person blob from `edge_out` (single B-tree read). For edge scans, reads the full 8-byte edge blob per edge.
- **COLUMNAR mode**: Full-row cursor reads all colgroup B-trees. A projection cursor for `creation_date` reads *only* the `temporal` B-tree (1 of 2–3 B-trees).

Expected outcome: R1/X1 (read ALL properties of one vertex) — EMBEDDED wins or ties. R2/A2/A3 (read ONE property from many edges) — COLUMNAR wins once edge count is large enough to amortize setup. R3/BFS (zero property reads) — negligible difference.

---

## G. Remaining Work Plan

This section describes the work items needed to complete the thesis PoC.
Items are ordered by dependency; Phase 1 must precede Phase 2.

---

### Phase 1 — Projection-cursor query variants (items 6 + 7)

**Goal**: demonstrate the actual I/O benefit of COLUMNAR mode by having R2, A2,
and A3 read *only the `temporal` B-tree* instead of deserializing the full
property blob. This is the experiment the thesis compares.

#### 1a. Add `open_projection_cursor` to `SplitEdgeKey` (and AdjList)

`open_colgroup_cursor(table, colgroup)` opens `colgroup:table:colgroup`, which
reads the entire colgroup row. WiredTiger also supports cursor projections opened
as `table:person_props(creationDate)` — this is syntactically different but gives
the same single-B-tree read. Either spelling works; the colgroup cursor is already
sufficient. No new code needed here.

#### 1b. Add COLUMNAR-aware variants of R2, A2, A3

These three queries each scan many edge or node rows reading only `creationDate`.
In EMBEDDED mode they call `get_edge_properties` / `get_node_properties` (full
blob round-trip). In COLUMNAR mode they should open the `temporal` colgroup cursor
once and iterate it directly, never touching the node/edge table.

**R2 — friends sorted by creationDate**:
- EMBEDDED: for each friend, call `get_edge_properties(person, friend)` → unpack 8-byte blob → extract creationDate. One random seek per friend.
- COLUMNAR: open `colgroup:knows_props:temporal`, range-scan from `(person_id, 0)` to `(person_id+1, 0)` — reads only the temporal B-tree for all knows edges of this person.

**A2 — count knows edges in date range**:
- EMBEDDED: same per-edge blob lookup.
- COLUMNAR: same `knows_props:temporal` range scan with date filter inline.

**A3 — count likes in date range**:
- EMBEDDED: per-likes-edge blob lookup.
- COLUMNAR: `colgroup:likes_props:temporal` range scan from `(person_id, 0)`.

Implementation: add a `_columnar` variant of each function (or add a `prop_mode`
branch inside the existing function). The colgroup cursor is opened at function
entry and closed on return — no shared cursor state needed.

Files changed: `test/ldbc_snb_queries.cpp` only (~80 lines).

#### 1c. Dual-mode comparison in `main()`

Change `main()` to accept `--mode embedded|columnar|both` (default `both`).
In `both` mode, load the graph once in EMBEDDED mode, run all property-reading
queries and record times, then reload in COLUMNAR mode and run again. Print a
two-column table:

```
Query                        EMBEDDED    COLUMNAR    speedup
r2_friends_sorted_by_date    2.2 ms      0.8 ms      2.75×
a2_knows_in_date_range       0.024 ms    0.018 ms    1.3×
a3_posts_liked_in_range      0.030 ms    0.020 ms    1.5×
bi1_posting_summary          345 ms      345 ms      1.0×   (no single-col benefit)
bi12_fast                    1250 ms     1250 ms     1.0×   (already sequential)
```

Loading twice is acceptable for the PoC — it avoids any in-process cache warming
between modes.

Files changed: `test/ldbc_snb_queries.cpp` (~40 lines in `main()`).

**Expected commit**: one commit covering 1b + 1c together.

---

### Phase 2 — Additional BI queries

**Goal**: expand the BI query set to cover more patterns relevant to the columnar
storage thesis. These were selected in the earlier discussion as the best
candidates; all work within the current 2-vertex-type PoC schema.

#### BI-2 — Tag evolution (two time windows)

Scan `post_props:temporal` twice (two date windows), group posts by tag
(requires `post_has_tag` if available, else skip tag grouping). Demonstrates
sequential scan performance on a narrow date window.

**Prerequisite**: no new schema required (if tag grouping is skipped). A
simpler version counts posts per month in two windows.

#### BI-4 / BI-5 — Top message creators / posters of a topic

Both require iterating all posts by a given person-set (friends of X) and
counting. Demonstrates `likes_props:temporal` or `post_props:temporal` scan
for a bounded range. Simpler than the full spec (no tag dimension needed for
the colgroup benefit demonstration).

**Prerequisite**: no new schema required for the date-range portion.

#### Additional IC queries within current schema

IC-1, IC-2, IC-7, IC-9 are implementable with Person + Post + knows + likes +
hasCreator:

| Query | Pattern | Columnar benefit? |
|-------|---------|-------------------|
| IC-1  | BFS + person property read | No (single-hop property fetch) |
| IC-7  | People who liked person's recent posts | Scan `likes_props:temporal` — Yes |
| IC-9  | Posts/comments by friends before date | Scan `post_props:temporal` per friend — Yes |

Files changed: `test/ldbc_snb_queries.cpp`.

---

### Phase 3 — Full schema (deferred, post-PoC)

These items require significant schema and loader changes. They are listed here
for completeness but are out of scope for the current thesis evaluation.

| Item | What it adds | Effort |
|------|-------------|--------|
| Comment vertex type | `comment_props` table + loader CSV | Medium |
| Forum vertex type | `forum_props` + hasMember edge | Medium |
| Tag / TagClass / Place / Organisation | Dimension tables + FK columns in existing props | High |
| String properties | Fixed-length `char[32]` columns in person_props | Medium |
| Multi-valued attributes | `person_email`, `person_speaks` tables | Low |
| IC-3, IC-11 workAt/studyAt | New fact tables + 2 IC queries | High |
| Full BI query set (BI-3 through BI-25) | Depends on all above | Very high |

---

### Summary

| Phase | Items | Estimated lines | Blocks thesis? |
|-------|-------|----------------|----------------|
| **Phase 1** (projection variants + comparison) | 1b + 1c | ~120 lines, queries only | **Yes — core experiment** |
| **Phase 2** (additional BI/IC queries) | BI-2, IC-7, IC-9 | ~150 lines | No — enriches results |
| **Phase 3** (full schema) | everything else | thousands | No — future work |

Phase 1 is the only remaining item required for the thesis comparison result.

---

## F. Related Work

### Comparison with the Gupta Thesis (GraphflowDB)

#### 1. Property Storage Granularity

**Gupta (GraphflowDB)** makes a *cardinality-driven* distinction:
- **Vertex columns** — when an edge type has 1-1, 1-n, or n-1 cardinality, the edge's properties are stored alongside the vertex (the "one" side), in a dense typed column per property. This avoids per-edge property storage entirely for most edge types.
- **Single-indexed property pages** — for n-n edges (e.g., `knows`), properties are stored in columnar pages indexed by `(edge_label, src_vertex_ID, page_level_offset)`. Each property field is a separate column in these pages.

The key insight Gupta exploits is that cardinality determines whether the "join" between structure and properties is free (vertex column) or requires a page lookup.

**Flexograph embedded** stores a single opaque blob per entity and reads/writes it atomically. No per-property I/O isolation is possible.

**Flexograph split (Section E plan)** would create typed property tables and column groups, which is structurally similar to Gupta's property pages, but without cardinality-driven routing. Every entity type gets a typed table regardless of whether it could be a vertex column.

**Gap**: Flexograph has no concept of vertex columns. It could not exploit the 1-n cardinality of `hasCreator` (every Post has exactly one creator) to store Post's `creation_date` adjacent to the Person row instead of in a separate lookup.

#### 2. Storage Structure (B-tree vs CSR)

**Gupta** uses **CSR (Compressed Sparse Row)** for adjacency structure — a static, sorted adjacency array. This enables dense columnar property pages with predictable offsets and null compression via Jacobson's rank index.

**Flexograph** uses **WiredTiger B-trees** for both structure and properties. This gives full update support (insert/delete edges after initial load), MVCC transactions, and crash recovery for free — none of which CSR provides. The cost is that B-tree storage is not as cache-friendly as CSR for sequential neighborhood scans, and there is no equivalent of Jacobson-compressed null columns.

The Gupta thesis reports **2.36x memory reduction** and **2.6x query speedup** vs. a row-oriented baseline. Both of these numbers are measured on a CSR + columnar system, not on a B-tree. A fair comparison against Flexograph's split mode would need to account for the B-tree's structural overhead (node pointers, page headers) that CSR doesn't have.

#### 3. Label/Type Handling

**Gupta** exploits the *edge label → vertex type* mapping at the storage layer: because GraphflowDB knows all vertex types and edge types at schema definition time, it can place vertex columns for specific label combinations. The adjacency lists are stored per edge label, so type-isolated scans are native.

**Flexograph** implements ID-space partitioning entirely in user space, with no storage-layer awareness of types. The benefit (sorted key-range scan isolates edge types) is real but is a consequence of B-tree key ordering, not a schema-level optimization. Adding a new vertex type post-load would require renumbering all existing IDs of that type — the same limitation the thesis acknowledges.

Both approaches avoid the "type column in every row" overhead. Flexograph's approach is simpler (just an integer comparison) but less powerful (cannot do per-label adjacency lists at the storage level).

#### 4. Evaluation Benchmark

Both use **LDBC SNB**, which makes direct numerical comparison possible once Flexograph's split mode is implemented. Gupta's reported numbers (2.36x, 2.6x) are thus directly relevant as a reference baseline — Section E's "what the comparison will measure" is exactly the right framing.

#### 5. Summary

| Dimension | Gupta (GraphflowDB) | Flexograph embedded | Flexograph split (planned) |
|-----------|---------------------|---------------------|---------------------------|
| **Storage structure** | CSR + columnar pages | WiredTiger B-tree | WiredTiger B-tree |
| **Property granularity** | Per-property column | Full blob per entity | Per-property typed column |
| **Cardinality routing** | Yes (vertex col / prop page) | No | No |
| **Update support** | No (static CSR) | Yes (MVCC B-tree) | Yes |
| **Null compression** | Yes (Jacobson rank) | No | No |
| **Label/type** | Per-label adjacency lists | ID-space partitioning | ID-space partitioning |
| **Benchmark** | LDBC SNB | LDBC SNB | LDBC SNB |
| **Claimed speedup** | 2.6x vs row-oriented | — (baseline) | Hypothesis: wins on scan-heavy queries |

The honest framing for the thesis is: Flexograph's split mode is structurally similar to Gupta's property pages but implemented on an updateable B-tree instead of a static CSR. The expected speedup for split over embedded (the thesis research question) will be smaller than Gupta's 2.6x number because (a) B-tree I/O is less predictable than CSR array access, and (b) Flexograph has no cardinality routing to eliminate the property lookup entirely for 1-n edges.

---

### Comparison with TuGraph (Lin et al., "Building a High-Performance Graph Storage on Top of Tree-Structured Key-Value Stores")

TuGraph is the closest architectural comparator to Flexograph: both are property graph systems built directly on top of a tree-structured KV store (TuGraph uses LMDB, Flexograph uses WiredTiger — both B+ tree families). Neither uses CSR or a purpose-built graph file format.

#### 1. Underlying KV Store

TuGraph chose **LMDB** (memory-mapped B+ tree) over RocksDB (LSM tree) because the target workload is read-heavy (20:1 read/write ratio). LMDB delivers up to 4.6x better read throughput than RocksDB in their benchmarks at the cost of a single-writer constraint (one write transaction at a time).

**WiredTiger** (Flexograph's choice) is also a B+ tree but was designed for MongoDB's concurrent write workloads — it has MVCC, checkpoint-based durability, and no single-writer limitation. For the PoC, transaction throughput is not a concern, but the architectural choice matters: Flexograph inherits WiredTiger's richer concurrency model without needing TuGraph's WAL workaround (see Update Support below).

#### 2. Edge Key Scheme

This is the sharpest divergence between the two systems.

**TuGraph's EdgeUid**: `(SrcVid, LabelId, DstVid, Eid, TemporalId)`
- `LabelId` is part of the primary key, making multi-label edge storage native: all edges of label `L` between a `(src, dst)` pair are grouped contiguously in the B-tree.
- `Eid` is a per-`(src, label, dst)` sequence number, enabling **multi-edges** (multiple edges of the same type between the same pair of vertices).
- `TemporalId` supports temporal window queries natively at the key level.
- Key ordering by `LabelId` first means a key-range scan over `(SrcVid, L, *, *, *)` retrieves all edges of label `L` in one sequential pass.

**Flexograph SplitEdgeKey**: `(MAKE_EKEY(src), MAKE_EKEY(dst))`
- No `LabelId`, no `Eid`, no `TemporalId`.
- Multi-label support is handled entirely in user space via ID-space partitioning (see Section D). The label is implicit in the destination ID range, not stored in the key.
- Multi-edges are not supported: the `(src, dst)` key is unique per table.

The consequence is that TuGraph can store all edge types in a single LMDB database and use label-prefixed key scans to isolate types, while Flexograph relies on the user-space ID range trick. TuGraph's approach is more general (works for any edge type without requiring disjoint endpoint ranges); Flexograph's works only when endpoint ID ranges partition cleanly — which is true for LDBC SNB but would break for, e.g., two edge types both connecting `Person → Person`.

#### 3. Property Storage: Both Land on Inline/Compact Packing

Despite the key scheme difference, both systems independently arrive at the same property storage conclusion: **inline (compact) packing beats separate index packing for read-heavy workloads.**

TuGraph explicitly benchmarks this: *"Observation 3: topology and properties are accessed together"*. Their compact packing is **1.5x faster** than index packing (storing properties in a separate KV pair) for vertex reads. Inline wins because traversal + property access in a single KV lookup eliminates a second random seek.

Flexograph's embedded mode is exactly this: the node sentinel row in `edge_out` stores `{in_deg | out_deg | prop_blob}` inline. Reading a vertex's properties and its degree (used for iteration bounds) is a single WiredTiger cursor lookup.

This provides direct empirical backing for Flexograph's embedded design choice from an independently published result.

#### 4. Adaptive Splitting vs. Separate Property Tables

TuGraph does have a "split" mechanism, but it operates at the **vertex/edge granularity**, not the **per-property column** level:

- When a vertex value grows beyond 4KB (large neighborhoods or many properties), TuGraph splits into three separate KV entries sharing the same `SrcVid` prefix: `VERTEX`, `OUTEDGE`, `INEDGE`. These are still B-tree siblings and accessed together via a range scan.
- There is no per-property column group. All properties of a vertex are packed together in the `VERTEX` key's value regardless of which properties a query needs.

Flexograph's planned split mode (Section E) goes further: separate typed tables with one B-tree per property column (`colgroup:person_props/cg_creation_date`). This is a stricter form of splitting that TuGraph does not implement. TuGraph's authors implicitly argue this is unnecessary given that Observation 3 makes the full-row read fast enough for their workload.

#### 5. Label/Type Handling

TuGraph stores `LabelId` in the edge key natively. Vertex labels are handled by applying the correct schema at read time based on a metadata store (the paper does not detail vertex label storage but implies per-label schema tables).

Flexograph uses ID-space partitioning (Section D): label is encoded in the numeric range of the vertex ID, with no storage-layer change. Both approaches avoid scanning all edges to find typed edges. TuGraph's approach generalizes to any label combination; Flexograph's requires disjoint ID ranges per type, which is a constraint on graph loading order and ID assignment.

#### 6. Multi-Edge Support

TuGraph supports multi-edges via `Eid`. Flexograph does not, and this was the primary reason FinBench was rejected in favor of LDBC SNB (Section A). TuGraph's `Eid` is the minimal fix for this — a sequence number appended to the key.

#### 7. Update Support

TuGraph works around LMDB's single-writer limitation with a custom **Write-Ahead Log (WAL)**: random writes become sequential log appends, achieving 21.19 MB/s with 10 concurrent writers (vs. 7.53 MB/s baseline). Log compaction replays into the B+ tree every minute.

Flexograph has no such limitation: WiredTiger handles concurrent writes natively via MVCC. The PoC does not measure write throughput, but the infrastructure is already capable of concurrent ingestion without custom WAL machinery.

#### 8. Performance Benchmark

TuGraph is ranked #1 on the **LDBC SNB Interactive benchmark** official board (12,721 QPS at SF-300). It achieves best latency on 2–6 hop K-hop queries on the Twitter 2010 graph (41.65M vertices, 1.47B edges), with competitors crashing or exceeding 2-hour limits on 6-hop.

Flexograph does not compete at this scale. The PoC targets LDBC SNB SF-0.003 (a few thousand vertices, tens of thousands of edges) and measures embedded vs. split property access, not throughput at scale. A direct QPS comparison is not meaningful at this stage.

#### 9. Summary

| Dimension | TuGraph | Flexograph |
|-----------|---------|------------|
| **KV store** | LMDB (B+ tree, memory-mapped) | WiredTiger (B+ tree, MVCC) |
| **Edge key** | `(SrcVid, LabelId, DstVid, Eid, TemporalId)` | `(src, dst)` — no label, no multi-edge |
| **Multi-edge** | Yes (Eid) | No |
| **Property storage** | Compact inline (same row as structure) | Compact inline (embedded mode) |
| **Columnar properties** | No (adaptive split at vertex level only) | Planned (per-property column groups) |
| **Label handling** | LabelId in key (native) | ID-space partitioning (user space) |
| **Update support** | Yes (WAL for concurrent writes) | Yes (WiredTiger MVCC, native) |
| **Benchmark** | LDBC SNB, #1 official board | LDBC SNB, PoC scale only |

**Key takeaway**: TuGraph validates Flexograph's core storage choice — compact inline property packing on a B+ tree KV store. Where they diverge is that TuGraph embeds label and multi-edge support natively in the edge key, while Flexograph pushes both to user space. Flexograph's planned split/columnar mode has no TuGraph equivalent, making it the genuinely novel contribution relative to both TuGraph and the Gupta thesis.
