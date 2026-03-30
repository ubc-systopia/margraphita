# Full LDBC SNB Schema — Implementation Plan

**Branch:** `full_snb_schema`
**Date:** 2026-03-30
**Starting point:** commit `eaf4a74` (property_storage, Phase 1 complete)

---

## Goal

Expand from the 2-type PoC (Person, Post) to the full LDBC SNB schema:
11 vertex types, 17 edge types, ~25 CSV input files. Implement the corresponding
IC and BI query set, focusing on queries that expose new chokepoints in the
Flexograph-vs-NeuG comparison.

---

## Vertex Types

| Enum | Value | Status | Notes |
|------|-------|--------|-------|
| `VT_PERSON` | 0 | ✅ existing | |
| `VT_POST` | 1 | ✅ existing | |
| `VT_COMMENT` | 2 | new | Same Message interface as Post |
| `VT_FORUM` | 3 | new | |
| `VT_TAG` | 4 | new | Dimension type |
| `VT_TAGCLASS` | 5 | new | Dimension type |
| `VT_CITY` | 6 | new | Subtype of Place |
| `VT_COUNTRY` | 7 | new | Subtype of Place |
| `VT_CONTINENT` | 8 | new | Subtype of Place |
| `VT_COMPANY` | 9 | new | Subtype of Organisation |
| `VT_UNIVERSITY` | 10 | new | Subtype of Organisation |

**Why fine-grained Place/Organisation subtypes:** `workAt` (Person→Company) and
`studyAt` (Person→University) both connect Person to an Organisation subtype. If
Company and University shared one `VT_ORGANISATION` enum value, `(Person,
Organisation)` would be ambiguous between two edge types. Splitting them gives
unique `(src_type, dst_type)` pairs for every edge.

**`hasModerator` resolution:** Forum→Person would collide with `hasMember`
(same `(VT_FORUM, VT_PERSON)` pair). Since a Forum has at most one moderator,
store `moderator_id` as a field in `SNBForumSchema` rather than as an edge.
This eliminates the collision.

---

## Edge Types

| Edge | src type | dst type | Properties | WT props table | Colgroup |
|------|----------|----------|------------|----------------|---------|
| knows | Person | Person | creationDate | `knows_props` ✅ | temporal |
| likes | Person | Post | creationDate | `likes_props` ✅ | temporal |
| likes | Person | **Comment** | creationDate | `likes_props` (same table) | temporal |
| hasCreator | Post | Person | — | — | — |
| hasCreator | **Comment** | Person | — | — | — |
| containerOf | Forum | Post | — | — | — |
| **hasMember** | Forum | Person | creationDate | `hasmember_props` | temporal |
| hasTag | Post | Tag | — | — | — |
| hasTag | Comment | Tag | — | — | — |
| hasTag | Forum | Tag | — | — | — |
| hasInterest | Person | Tag | — | — | — |
| hasType | Tag | TagClass | — | — | — |
| isSubclassOf | TagClass | TagClass | — | — | — |
| isLocatedIn | Person | City | — | — | — |
| isLocatedIn | Post/Comment | Country | — | — | — |
| isLocatedIn | Company/University | Place | — | — | — |
| isPartOf | City | Country | — | — | — |
| isPartOf | Country | Continent | — | — | — |
| replyOf | Comment | Post | — | — | — |
| replyOf | Comment | Comment | — | — | — |
| **studyAt** | Person | University | classYear (int32) | `studyat_props` | — |
| **workAt** | Person | Company | workFrom (int32) | `workat_props` | — |

All property-free edges use the existing 1-byte sentinel value (Bug 6 guard).

`likes_props` already uses `(src Q, dst Q)` key where dst encodes vertex type in
its top 8 bits. Likes to a Comment (dst VT_COMMENT=2) will have a higher key
than likes to a Post (VT_POST=1), so they co-exist in the same table without
any structural change to `likes_props`.

---

## Property Schemas (additions to `src/prop_schema.h`)

### String width decisions

| Field context | Max spec length | Storage |
|---|---|---|
| Person firstName/lastName/browserUsed/locationIP | ~32 chars in practice | `32s` ✅ existing |
| Tag.name, TagClass.name, Place.name, Org.name | String = 80 Unicode chars | `80s` fixed |
| Tag.url, TagClass.url, Place.url, Org.url | String = 80 chars | `80s` fixed |
| Forum.title | Long String = 256 chars | variable `S` column |
| Comment.content | Text = 2000 chars | variable `S` column (same as Post) |

### New schemas

```
SNBCommentSchema  — identical layout to SNBPostSchema
  | creationDate(Q) | length(i) | tag(b) | content(S) |
  TOTAL_SIZE = 13 minimum (variable)
  WT: value_format=QibS, colgroups: temporal=(creationDate,length), content=(tag,content)
  Note: tag=0 always for Comments (no imageFile); keep field for schema uniformity.

SNBForumSchema
  | creationDate(Q) | moderator_id(Q) | title(S) |
  TOTAL_SIZE = 16 minimum (variable)
  WT: value_format=QQS, colgroups: temporal=(creationDate), info=(moderator_id,title)

SNBTagSchema
  | name(80s) | url(80s) |
  TOTAL_SIZE = 160 bytes (fixed)
  WT: value_format=80s80s, columns=(tag_id,name,url)
  No colgroup — dimension type used only for point lookups, not scanned in aggregate.

SNBTagClassSchema
  | name(80s) | url(80s) |
  TOTAL_SIZE = 160 bytes (fixed)
  WT: value_format=80s80s

SNBPlaceSchema
  | name(80s) | url(80s) | place_type(b) |
  TOTAL_SIZE = 161 bytes (fixed)
  place_type: 0=city, 1=country, 2=continent
  WT: value_format=80s80sb

SNBOrganisationSchema
  | name(80s) | url(80s) | org_type(b) |
  TOTAL_SIZE = 161 bytes (fixed)
  org_type: 0=company, 1=university
  WT: value_format=80s80sb

SNBHasMemberSchema
  | creationDate(Q) |
  TOTAL_SIZE = 8 bytes
  WT: value_format=Q, colgroup: temporal=(creationDate)

SNBStudyAtSchema
  | classYear(i) |
  TOTAL_SIZE = 4 bytes
  WT: value_format=i  (no colgroup — single column, colgroup is trivial)

SNBWorkAtSchema
  | workFrom(i) |
  TOTAL_SIZE = 4 bytes
  WT: value_format=i
```

---

## Files Changed Per Phase

```
Phase A  — Storage layer
  src/common_defs.h         VertexType enum, new table-name constants
  src/prop_schema.h         6 new namespaces
  src/edgekey_split.cpp     create_wt_tables, init_cursors, close_all_cursors,
                            set/get_node_properties, set/get_edge_properties
  src/edgekey_split.h       10 new cursor members
  src/adj_list.cpp          same 4 functions (parallel to SplitEdgeKey)
  src/adj_list.h            10 new cursor members

Phase B  — Loader
  test/ldbc_snb_loader.h    new vertex loaders, new edge loaders, ID maps,
                            loading order

Phase C  — Queries
  test/ldbc_snb_queries.cpp new query functions, QueryTimes fields, main() calls

Phase D  — Tests & build
  test/test_columnar_full.cpp   new COLUMNAR unit tests for all new types
  test/CMakeLists.txt           add test_columnar_full target
```

---

## Phase A — Storage Layer

### A1: VertexType enum  (`src/common_defs.h`)

Add to the `VertexType` enum:

```cpp
enum VertexType : uint8_t {
    VT_PERSON     = 0,  // existing
    VT_POST       = 1,  // existing
    VT_COMMENT    = 2,
    VT_FORUM      = 3,
    VT_TAG        = 4,
    VT_TAGCLASS   = 5,
    VT_CITY       = 6,
    VT_COUNTRY    = 7,
    VT_CONTINENT  = 8,
    VT_COMPANY    = 9,
    VT_UNIVERSITY = 10,
};
```

Add new table-name constants below the existing ones:

```cpp
const std::string COMMENT_PROPS_TABLE      = "comment_props";
const std::string FORUM_PROPS_TABLE        = "forum_props";
const std::string TAG_PROPS_TABLE          = "tag_props";
const std::string TAGCLASS_PROPS_TABLE     = "tagclass_props";
const std::string PLACE_PROPS_TABLE        = "place_props";
const std::string ORGANISATION_PROPS_TABLE = "organisation_props";
const std::string HASMEMBER_PROPS_TABLE    = "hasmember_props";
const std::string STUDYAT_PROPS_TABLE      = "studyat_props";
const std::string WORKAT_PROPS_TABLE       = "workat_props";
const std::string CG_INFO                  = "info";   // forum moderator+title colgroup
```

### A2: Property schemas  (`src/prop_schema.h`)

Add 6 new namespaces as described in the schema section above.

**Do not modify** existing SNBPersonSchema/SNBPostSchema — their layouts and unit
tests must stay green throughout.

### A3: `create_wt_tables` additions  (`src/edgekey_split.cpp`, `src/adj_list.cpp`)

In the COLUMNAR branch, after the existing `likes_props` block, create:

```
comment_props    value_format=QibS  columns=(node_id,creation_date,length,tag,content)
                 colgroups: temporal=(creation_date,length)  content=(tag,content)

forum_props      value_format=QQS   columns=(node_id,creation_date,moderator_id,title)
                 colgroups: temporal=(creation_date)  info=(moderator_id,title)

tag_props        value_format=80s80s  columns=(node_id,name,url)
                 (no colgroup)

tagclass_props   value_format=80s80s  columns=(node_id,name,url)
                 (no colgroup)

place_props      value_format=80s80sb  columns=(node_id,name,url,place_type)
                 (no colgroup)

organisation_props  value_format=80s80sb  columns=(node_id,name,url,org_type)
                 (no colgroup)

hasmember_props  value_format=Q   columns=(src,dst,creation_date)  key_format=QQ
                 colgroup: temporal=(creation_date)

studyat_props    value_format=i   columns=(src,dst,class_year)  key_format=QQ
                 (no colgroup)

workat_props     value_format=i   columns=(src,dst,work_from)  key_format=QQ
                 (no colgroup)
```

For EMBEDDED mode, all new types store raw blobs inline (same code path as
existing Person/Post EMBEDDED handling). Only the COLUMNAR branch adds the typed
tables above.

### A4: New cursor members  (`src/edgekey_split.h`, `src/adj_list.h`)

Add in the COLUMNAR cursor section (same pattern as existing):

```cpp
WT_CURSOR *comment_props_cursor       = nullptr;
WT_CURSOR *forum_props_cursor         = nullptr;
WT_CURSOR *tag_props_cursor           = nullptr;
WT_CURSOR *tagclass_props_cursor      = nullptr;
WT_CURSOR *place_props_cursor         = nullptr;
WT_CURSOR *organisation_props_cursor  = nullptr;
WT_CURSOR *hasmember_props_cursor     = nullptr;
WT_CURSOR *studyat_props_cursor       = nullptr;
WT_CURSOR *workat_props_cursor        = nullptr;
```

Open them in `init_cursors()` under the COLUMNAR block; close them in
`close_all_cursors()` with nullptr guards (same pattern as existing cursors).

**`graph_opts` field rule:** if `graph_opts` grows any new fields to support full
schema options (e.g., per-type node counts), every new field must appear in all
4 places: declaration, default ctor init list, `operator=`, copy ctor.

### A5: `set_node_properties` / `get_node_properties` routing

Both functions dispatch on `VTYPE_OF(id)`. Extend the switch to cover new types:

```cpp
switch (VTYPE_OF(id)) {
    case VT_PERSON:     // existing
    case VT_POST:       // existing
    case VT_COMMENT:    → comment_props_cursor (same layout as post_props)
    case VT_FORUM:      → forum_props_cursor
    case VT_TAG:        → tag_props_cursor
    case VT_TAGCLASS:   → tagclass_props_cursor
    case VT_CITY:
    case VT_COUNTRY:
    case VT_CONTINENT:  → place_props_cursor
    case VT_COMPANY:
    case VT_UNIVERSITY: → organisation_props_cursor
}
```

For EMBEDDED mode, all new types pack into the existing opaque blob path —
no routing change needed there, just the prop_schema pack/unpack helpers.

### A6: `set_edge_properties` / `get_edge_properties` routing

The existing `route_edge_cursor` selects between `knows_props_cursor` and
`likes_props_cursor` based on `(src_type, dst_type)`. Extend it:

```cpp
static WT_CURSOR *route_edge_cursor(node_id_t src, node_id_t dst, ...) {
    uint8_t st = VTYPE_OF(src), dt = VTYPE_OF(dst);
    if (st == VT_PERSON && dt == VT_PERSON)     return knows_props_cursor;
    if (st == VT_PERSON && (dt == VT_POST || dt == VT_COMMENT))
                                                return likes_props_cursor;
    if (st == VT_FORUM  && dt == VT_PERSON)     return hasmember_props_cursor;
    if (st == VT_PERSON && dt == VT_UNIVERSITY) return studyat_props_cursor;
    if (st == VT_PERSON && dt == VT_COMPANY)    return workat_props_cursor;
    return nullptr;  // structural-only edge (no property table)
}
```

`set_edge_properties` / `get_edge_properties` must handle `nullptr` return
(no-op for property-free edges).

---

## Phase B — Loader  (`test/ldbc_snb_loader.h`)

### B1: New ID maps

Add one `unordered_map<int64_t, node_id_t>` per new vertex type:

```cpp
std::unordered_map<int64_t, node_id_t> comment_id_map;
std::unordered_map<int64_t, node_id_t> forum_id_map;
std::unordered_map<int64_t, node_id_t> tag_id_map;
std::unordered_map<int64_t, node_id_t> tagclass_id_map;
std::unordered_map<int64_t, node_id_t> place_id_map;   // city + country + continent
std::unordered_map<int64_t, node_id_t> organisation_id_map;  // company + university
```

Place and Organisation are heterogeneous (city/country/continent and
company/university share one map each) — the compact ID encodes the specific
subtype in its top-8-bits via the correct VT_ enum value.

### B2: New vertex CSV loaders

Each follows the existing `load_persons` / `load_posts` pattern:
read header, iterate rows, `add_node(MAKE_TYPED_ID(VT_X, counter++), false)`,
enqueue pending prop write. Critical: enqueue all nodes before any edges.

| Method | CSV (dynamic/) | Prop schema |
|---|---|---|
| `load_comments()` | `comment_0_0.csv` | SNBCommentSchema |
| `load_forums()` | `forum_0_0.csv` | SNBForumSchema (moderator_id from hasModerator CSV, filled in pass 2) |
| `load_tags()` | `static/tag_0_0.csv` | SNBTagSchema |
| `load_tagclasses()` | `static/tagclass_0_0.csv` | SNBTagClassSchema |
| `load_places()` | `static/place_0_0.csv` | SNBPlaceSchema (place_type from `type` column: city/country/continent) |
| `load_organisations()` | `static/organisation_0_0.csv` | SNBOrganisationSchema (org_type from `type` column) |

**Forum.moderator_id loading:** `forum_hasModerator_person_0_0.csv` is a separate
CSV. The moderator is stored as a field in the forum's property blob, not as an
edge. Load hasModerator CSV after forums are inserted; update the pending forum
prop to include the moderator_id before `flush_node_props()` is called.

### B3: New edge CSV loaders

Property-free edges only need `add_edge`. Edges with properties also enqueue a
pending edge prop write.

| Method | CSV (dynamic/ unless noted) | Props |
|---|---|---|
| `load_likes_comments()` | `person_likes_comment_0_0.csv` | SNBLikesSchema (same as person_likes_post) |
| `load_comment_has_creator()` | `comment_hasCreator_person_0_0.csv` | none |
| `load_reply_of()` | `comment_replyOf_post_0_0.csv` + `comment_replyOf_comment_0_0.csv` | none |
| `load_container_of()` | `forum_containerOf_post_0_0.csv` | none |
| `load_has_member()` | `forum_hasMember_person_0_0.csv` | SNBHasMemberSchema |
| `load_post_has_tag()` | `post_hasTag_tag_0_0.csv` | none |
| `load_comment_has_tag()` | `comment_hasTag_tag_0_0.csv` | none |
| `load_forum_has_tag()` | `forum_hasTag_tag_0_0.csv` | none |
| `load_has_interest()` | `person_hasInterest_tag_0_0.csv` | none |
| `load_has_type()` | `tag_hasType_tagclass_0_0.csv` | none |
| `load_is_subclass_of()` | `static/tagclass_isSubclassOf_tagclass_0_0.csv` | none |
| `load_person_is_located_in()` | `person_isLocatedIn_place_0_0.csv` | none |
| `load_message_is_located_in()` | `post_isLocatedIn_place_0_0.csv` + `comment_isLocatedIn_place_0_0.csv` | none |
| `load_org_is_located_in()` | `static/organisation_isLocatedIn_place_0_0.csv` | none |
| `load_is_part_of()` | `static/place_isPartOf_place_0_0.csv` | none |
| `load_study_at()` | `person_studyAt_university_0_0.csv` | SNBStudyAtSchema |
| `load_work_at()` | `person_workAt_company_0_0.csv` | SNBWorkAtSchema |

### B4: Loading order

The SplitEdgeKey constraint (add_node → add_edge → flush_node_props →
flush_edge_props) still applies. With multiple vertex types the order is:

```
1. load static vertices:
   load_tagclasses(), load_tags(), load_places(), load_organisations()
   (no dependencies — static dimension types)

2. load dynamic vertices:
   load_persons(), load_posts(), load_comments(), load_forums()
   (persons first — comments/forums/posts reference them via hasCreator/hasModerator)

3. load hasModerator CSV → update pending forum prop (fill moderator_id field)

4. load all edges (order within this step is free):
   load_knows(), load_post_has_creator(), load_likes_posts(), load_likes_comments(),
   load_comment_has_creator(), load_reply_of(), load_container_of(),
   load_has_member(), load_post_has_tag(), load_comment_has_tag(),
   load_forum_has_tag(), load_has_interest(), load_has_type(),
   load_is_subclass_of(), load_person_is_located_in(),
   load_message_is_located_in(), load_org_is_located_in(),
   load_is_part_of(), load_study_at(), load_work_at()

5. flush_node_props()   — writes all pending vertex property blobs
6. flush_edge_props()   — writes hasmember_props, studyat_props, workat_props
                          (and existing knows_props, likes_props)
```

---

## Phase C — Query Harness  (`test/ldbc_snb_queries.cpp`)

Query implementation is organized by dependency, not by IC/BI numbering.

### Chokepoint Coverage Map

Four distinct Flexograph-vs-NeuG chokepoints were identified when designing the
full-schema query set. The table below maps each chokepoint to the minimum query
that exercises it and the group it belongs to.

| CP | Chokepoint | Description | Minimum query | Group |
|----|-----------|-------------|--------------|-------|
| CP-1 | replyOf chain traversal depth | Comment→Post/Comment chains of variable depth; raw adjacency-list hop cost, no property benefit | **IC-8** | C1 |
| CP-2 | 3–4 hop cross-type join | Person→knows→Person→knows→Person + isLocatedIn + isPartOf filter; largest join tree in the schema | **IC-3** | C4 |
| CP-3 | High-degree Forum fan-out | Forums have O(10 K) members; IC-5 iterates hasMember adjacency in full to build a sorted membership list | **IC-5** | C2 |
| CP-4 | Dimension FK join + temporal scan | Temporal range scan on post/comment props followed by a point-lookup into Tag/Place/TagClass; tests whether WiredTiger colgroup B-tree join is competitive with NeuG's edge-type index | **BI-2** | C1 |

**Minimum set for full chokepoint coverage (7 queries):**

| Query | Chokepoints | Notes |
|-------|------------|-------|
| IC-7 | CP-4 (temporal scan only — no FK join) | Primary columnar-benefit demo; pairs with IC-9 |
| IC-9 | CP-4 (temporal scan per friend) | Cross-type temporal scan; pairs with IC-7 |
| **IC-8** | **CP-1** | Only query that exercises replyOf chain depth |
| BI-2 | CP-4 (temporal + hasTag FK join) | Full CP-4: colgroup scan + dimension table join |
| IC-5 | **CP-3** | Only query that exercises Forum fan-out at scale |
| IC-3 | **CP-2** | Only query that exercises the 3-hop cross-type join |
| IC-3 also covers | — | isLocatedIn + isPartOf, required for CP-2 correctness |

IC-7, IC-9, IC-8, BI-2 are all in group C1 (need only Person+Post+Comment).
IC-5 requires group C2 (Forum). IC-3 requires group C4 (Place/Organisation).
Implementing C1 first gives 3 of the 4 chokepoints before loading the full schema.

---

### C1: Comment type (Comment is loaded, likes→Comment works)

These are Phase 2 queries from `impl_rationale.md` — implementable with just
Person + Post + Comment:

| Query ID | Pattern | Columnar benefit | New edge types used |
|---|---|---|---|
| IC-7 | People who liked person's recent posts | `likes_props:temporal` scan | likes→Post existing |
| IC-9 | Posts/Comments by friends before date | `post_props:temporal` + `comment_props:temporal` per friend | hasCreator→Comment |
| IC-8 | Latest replies to a person's posts | `replyOf` chain traversal | replyOf |
| BI-2 | Tag evolution in two time windows (simplified: count by month, no tag grouping) | `post_props:temporal` + `comment_props:temporal` sequential scan | none beyond Comment |

IC-7 and IC-9 are the primary COLUMNAR-benefit queries (CP-4 partial).
BI-2 completes CP-4 by adding the hasTag FK join after the temporal scan.
IC-8 covers CP-1 (replyOf chain depth) — the only query that does so.

### C2: Forum type

| Query ID | Pattern | New edge types used |
|---|---|---|
| IC-5 | Forums where person's friends are members (top N by membership date) | hasMember, containerOf, hasCreator |
| BI-4 | Top message creators per forum (count posts+comments, sort by count) | hasMember, containerOf, hasCreator |

IC-5 covers CP-3 (high-degree Forum fan-out) — the only query that does so.

### C3: Tag and TagClass

| Query ID | Pattern | New edge types used |
|---|---|---|
| IC-4 | Tags person posted about in date range (new tags only, not pre-existing) | hasTag, hasCreator |
| IC-6 | Tag co-occurrence with a given tag (top N co-tags in friends' posts) | hasTag |
| IC-10 | Common interests between persons (posts with same tags as friends, near-birthday) | hasInterest, hasTag, hasCreator |
| IC-12 | Person's commented-on post count by TagClass (expert search) | replyOf, hasTag, hasType, isSubclassOf |
| BI-7 | Reply count per tag (scan all comment_hasTag, count replyOf targets) | hasTag, replyOf |
| BI-2 (full) | Tag evolution with actual tag grouping (upgrade from C1 simplified version) | hasTag, hasType, isSubclassOf |

IC-12 requires isSubclassOf hierarchy traversal — the TagClass subgraph is a
DAG that may need BFS or DFS for ancestor lookup.

### C4: Organisation and Place

| Query ID | Pattern | New edge types used |
|---|---|---|
| IC-1 | Person profile with universities + companies + home city | studyAt, workAt, isLocatedIn, isPartOf |
| IC-3 | Friends of friends NOT in same country as person X | isLocatedIn, isPartOf |
| IC-11 | Person's workAt in a specific country (year filter) | workAt, isLocatedIn, isPartOf |
| BI-3 | Forum message count by moderator's country | hasModerator (via forum.moderator_id), isLocatedIn |

IC-3 covers CP-2 (3-hop cross-type join) — the most complex join in the schema
and the only query that exercises the full Person→FoF + isLocatedIn + isPartOf
filter chain. Required for the primary Flexograph-vs-NeuG measurement.

### C5: Remaining BI queries

| Query ID | Pattern |
|---|---|
| BI-5 | Top poster per tag (posts+comments by person, with given tag, sort by count) |
| BI-6 | Like/message score per person for given tag (complex scoring) |
| BI-8 | Score: person's tagged-message count + friends' similar-tag count |
| BI-9 | Thread starters: persons who start threads (first message in replyOf tree) |

These are lower priority — implement after C1–C4 are confirmed working.

### QueryTimes additions

Extend the `QueryTimes` struct with a field for each new query listed above.
Add corresponding `double *out_ms = nullptr` parameter to each new query
function and `fprintf(stderr, "%s,%s,%.3f\n", ...)` CSV output entries.

---

## Phase D — Tests  (`test/test_columnar_full.cpp`)

New file: `test/test_columnar_full.cpp`

Cover the most important new types with insertion + retrieval round-trips in
both EMBEDDED and COLUMNAR modes. Minimum test set:

| Test # | Description |
|---|---|
| T1 | Comment insert + get_node_properties round-trip (EMBEDDED) |
| T2 | Comment insert + get_node_properties round-trip (COLUMNAR) |
| T3 | Forum insert + get_node_properties including moderator_id (COLUMNAR) |
| T4 | Tag insert + get_node_properties name/url (COLUMNAR) |
| T5 | hasMember edge insert + get_edge_properties (COLUMNAR) |
| T6 | studyAt edge insert + get_edge_properties classYear (COLUMNAR) |
| T7 | workAt edge insert + get_edge_properties workFrom (COLUMNAR) |
| T8 | replyOf edge (Comment→Post) insert; get_out_nodes_id returns correct dst |
| T9 | replyOf edge (Comment→Comment) insert; get_out_nodes_id returns correct dst |
| T10 | likes edge to Comment; verify likes_props:temporal cursor sees it |

CMakeLists.txt addition:

```cmake
ADD_EXECUTABLE(test_columnar_full "${PATH_TEST}/test_columnar_full.cpp")
TARGET_INCLUDE_DIRECTORIES(test_columnar_full PRIVATE ${PATH_SRC} ${PATH_TEST})
TARGET_LINK_LIBRARIES(test_columnar_full PUBLIC ${NAME_LIB} ${wt_shared_lib})
```

---

## Work Item Table

Legend: ✅ done · 🎯 minimum chokepoint set · TODO

| # | Phase | Item | Files | Status |
|---|-------|------|-------|--------|
| 1 | A1 | VertexType enum + table-name constants | common_defs.h | ✅ commit 6dd8e90 |
| 2 | A2 | SNBCommentSchema + SNBForumSchema | prop_schema.h | ✅ commit 6dd8e90 |
| 3 | A2 | SNBTagSchema, SNBTagClassSchema, SNBPlaceSchema, SNBOrganisationSchema | prop_schema.h | ✅ commit 6dd8e90 |
| 4 | A2 | SNBHasMemberSchema, SNBStudyAtSchema, SNBWorkAtSchema | prop_schema.h | ✅ commit 6dd8e90 |
| 5 | A3 | create_wt_tables: 9 new typed tables + colgroups (SplitEdgeKey) | edgekey_split.cpp | ✅ commit 6dd8e90 |
| 6 | A3 | create_wt_tables: same 9 tables (AdjList) | adj_list.cpp | ✅ commit 6dd8e90 |
| 7 | A4 | 9 new cursor members + init + close (SplitEdgeKey) | edgekey_split.h/.cpp | ✅ commit 6dd8e90 |
| 8 | A4 | 9 new cursor members + init + close (AdjList) | adj_list.h/.cpp | ✅ commit 6dd8e90 |
| 9 | A5 | set/get_node_properties: 9 new VTYPE_OF cases (SplitEdgeKey) | edgekey_split.cpp | ✅ commit 6dd8e90 |
| 10 | A5 | set/get_node_properties: 9 new VTYPE_OF cases (AdjList) | adj_list.cpp | ✅ commit 6dd8e90 |
| 11 | A6 | route_edge_cursor: hasMember + studyAt + workAt + likes→Comment | edgekey_split.cpp | ✅ commit 6dd8e90 |
| 12 | A6 | same routing (AdjList) | adj_list.cpp | ✅ commit 6dd8e90 |
| 13 | B1 | ID maps for 6 new types | ldbc_snb_loader.h | ✅ commit bc5ca5f |
| 14 | B2 | load_comments, load_forums (+ moderator_id backfill) | ldbc_snb_loader.h | ✅ commit bc5ca5f |
| 15 | B2 | load_tags, load_tagclasses, load_places, load_organisations | ldbc_snb_loader.h | ✅ commit bc5ca5f |
| 16 | B3 | load_likes_comments, load_comment_has_creator, load_reply_of | ldbc_snb_loader.h | ✅ commit bc5ca5f |
| 17 | B3 | load_container_of, load_has_member | ldbc_snb_loader.h | ✅ commit bc5ca5f |
| 18 | B3 | load_post/comment/forum_has_tag, load_has_interest | ldbc_snb_loader.h | ✅ commit bc5ca5f |
| 19 | B3 | load_has_type, load_is_subclass_of, load_is_located_in (3 variants) | ldbc_snb_loader.h | ✅ commit bc5ca5f |
| 20 | B3 | load_is_part_of, load_study_at, load_work_at | ldbc_snb_loader.h | ✅ commit bc5ca5f |
| 21 | B4 | Loading order in main() with full flush sequencing | ldbc_snb_queries.cpp | TODO |
| 22 | C1 🎯 | IC-7 (likes:temporal scan) — CP-4 partial | ldbc_snb_queries.cpp | TODO |
| 23 | C1 🎯 | IC-9 (friends' posts/comments by date) — CP-4 partial | ldbc_snb_queries.cpp | TODO |
| 24 | C1 🎯 | IC-8 (replyOf chain) — **CP-1** | ldbc_snb_queries.cpp | TODO |
| 25 | C1 🎯 | BI-2 simplified (message count by month, two windows) — **CP-4** | ldbc_snb_queries.cpp | TODO |
| 26 | C2 🎯 | IC-5 (forums where friends are members) — **CP-3** | ldbc_snb_queries.cpp | TODO |
| 27 | C2 | BI-4 (top message creators per forum) | ldbc_snb_queries.cpp | TODO |
| 28 | C3 | IC-4 (new tags in date range) | ldbc_snb_queries.cpp | TODO |
| 29 | C3 | IC-6 (tag co-occurrence) | ldbc_snb_queries.cpp | TODO |
| 30 | C3 | IC-10 (common tag interests near birthday) | ldbc_snb_queries.cpp | TODO |
| 31 | C3 | IC-12 (expert search by TagClass hierarchy) | ldbc_snb_queries.cpp | TODO |
| 32 | C3 | BI-7 (reply count per tag) | ldbc_snb_queries.cpp | TODO |
| 33 | C4 | IC-1 (full person profile with orgs + cities) | ldbc_snb_queries.cpp | TODO |
| 34 | C4 🎯 | IC-3 (friends of friends by country) — **CP-2** | ldbc_snb_queries.cpp | TODO |
| 35 | C4 | IC-11 (workAt by country + year) | ldbc_snb_queries.cpp | TODO |
| 36 | C4 | BI-3 (forum messages by moderator's country) | ldbc_snb_queries.cpp | TODO |
| 37 | C5 | BI-5, BI-6, BI-8, BI-9 | ldbc_snb_queries.cpp | TODO |
| 38 | D | test_columnar_full_ekey/adj (10 tests each, both backends) | test/ | ✅ commit 40262d7 |
| 39 | D | CMakeLists.txt: add test_columnar_full targets | test/CMakeLists.txt | ✅ commit 40262d7 |

---

## Implementation Order Recommendation

```
A1 → A2 → A3+A4+A5+A6 (storage layer, SplitEdgeKey first, AdjList mirrors)
         ↓
B1 → B2 (vertices) → B3 (edges) → B4 (order in main)
         ↓
D (test_columnar_full, run after each storage batch — fail fast)
         ↓
C1 (IC-7, IC-9, IC-8, BI-2)   ← covers CP-1 + CP-4; 3 of 4 chokepoints
         ↓
C2 (IC-5, BI-4)                ← covers CP-3
         ↓
C3 (IC-4, IC-6, IC-10, IC-12, BI-7)
         ↓
C4 (IC-1, IC-3, IC-11, BI-3)  ← covers CP-2; completes all 4 chokepoints
         ↓
C5 (BI-5, BI-6, BI-8, BI-9)   ← optional enrichment
```

**Minimum chokepoint-complete stopping point: end of C2.**
After C1+C2 (6 queries: IC-7, IC-9, IC-8, BI-2, IC-5, BI-4), three of the four
chokepoints are covered (CP-1, CP-3, CP-4). Adding IC-3 from C4 completes CP-2
and gives the full 7-query minimum set. C3 and C5 add breadth but no new
chokepoints.

If the primary goal is the chokepoint experiment rather than query completeness,
implement C4/IC-3 immediately after C1, before C2 and C3 — IC-3 only requires
Person + City/Country + isLocatedIn + isPartOf, which can be loaded in isolation.

---

## Design Decisions Deferred

| Decision | Why deferred |
|---|---|
| Variable-length title truncation policy for Forum (>256 chars in data) | In practice SNB data titles are short; log a warning and truncate at 256 |
| Whether to expose comment_props:temporal colgroup on AdjList | AdjList doesn't support `open_colgroup_cursor` yet; same limitation as post_props — add if needed for comparison |
| Parallel BI variants for new queries | Add after correctness is confirmed; same pattern as bi1/bi12 parallel |
| Delete operations | Out of scope (noted in impl_rationale.md §B) |
