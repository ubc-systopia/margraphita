# Table Layouts — Embedded vs Columnar Property Storage

LDBC SNB subset: Person + Post vertices, knows + hasCreator + likes edges.

Property schemas (from prop_schema.h):
  Person  → creationDate:Q(8B)  birthday:Q(8B)  gender:b(1B)
            firstName:32s(32B)  lastName:32s(32B)
            browserUsed:32s(32B)  locationIP:32s(32B)   = 145B
  Post    → creationDate:Q(8B)  length:i(4B)  tag:b(1B)
            content:S(variable)               = 13B + strlen(content) + 1
  knows   → creationDate:Q(8B)                          = 8B
  likes   → creationDate:Q(8B)                          = 8B
  hasCreator → (no properties)                          = 0B

WT format codes: u=raw blob  Q=uint64  q=int64  i=int32  b=int8
                 S=null-terminated string  32s=fixed 32-byte char array
Keys in SplitEdgeKey are stored big-endian (bswap32) inside u columns for
correct sort order.  MAKE_EKEY(id) = id+1 (reserves 0 as the sentinel dst).
Typed node IDs: top 8 bits = vertex type (VTYPE_SHIFT=56), bottom 56 bits = counter.

───────────────────────────────────────────────────────────────────────────────
1.  SPLITEDGEKEY — EMBEDDED
───────────────────────────────────────────────────────────────────────────────

All structural data AND property bytes live in the same two tables.
Node properties are appended after the degree bytes in the node sentinel row.

┌─────────────────────────────────────────────────────────────────────────────┐
│ TABLE: edge_out                                                              │
│ key_format=uu   value_format=u                                               │
│ columns=(src, dst, attr)                                                     │
├────────────────────────────┬────────────────────────────────────────────────┤
│ KEY                        │ VALUE  (single u blob)                         │
│ (src:u, dst:u)             │                                                │
├────────────────────────────┼────────────────────────────────────────────────┤
│ (MAKE_EKEY(id),  0)        │ [ in_deg:4B | out_deg:4B | prop_blob:145B ]    │
│  node sentinel             │                                                │
│  (Person id)               │   prop_blob = creationDate:8B                 │
│                            │             + birthday:8B                     │
│                            │             + gender:1B                       │
│                            │             + firstName:32B                   │
│                            │             + lastName:32B                    │
│                            │             + browserUsed:32B                 │
│                            │             + locationIP:32B                  │
│                            │                              total = 153B     │
├────────────────────────────┼────────────────────────────────────────────────┤
│ (MAKE_EKEY(id),  0)        │ [ in_deg:4B | out_deg:4B | prop_blob:var ]     │
│  node sentinel             │                                                │
│  (Post id)                 │   prop_blob = creationDate:8B                 │
│                            │             + length:4B                       │
│                            │             + tag:1B                          │
│                            │             + content:variable                │
│                            │         total = 21B + strlen(content)         │
├────────────────────────────┼────────────────────────────────────────────────┤
│ (MAKE_EKEY(src),           │ [ prop_blob:8B ]                               │
│  MAKE_EKEY(dst))           │                                                │
│  knows or likes edge       │   prop_blob = creationDate:8B    total = 8B   │
├────────────────────────────┼────────────────────────────────────────────────┤
│ (MAKE_EKEY(src),           │ [ 0x00 ]                                       │
│  MAKE_EKEY(dst))           │                                                │
│  hasCreator edge           │   1-byte sentinel (Bug 6 guard: never pass     │
│                            │   size=0 to WiredTiger)          total = 1B   │
└────────────────────────────┴────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ TABLE: edge_in    (directed graphs only)                                     │
│ key_format=uu   value_format=u                                               │
│ columns=(dst, src, attr)                                                     │
├────────────────────────────┬────────────────────────────────────────────────┤
│ KEY                        │ VALUE  (single u blob)                         │
│ (dst:u, src:u)             │                                                │
├────────────────────────────┼────────────────────────────────────────────────┤
│ (MAKE_EKEY(dst),           │ [ prop_blob:8B ]                               │
│  MAKE_EKEY(src))           │                                                │
│  reverse edge row          │   identical prop bytes to edge_out entry       │
│  (no node sentinels here)  │                              total = 8B        │
└────────────────────────────┴────────────────────────────────────────────────┘

Note: node properties exist only in edge_out (degree_cursor points to
edge_out; set_node_properties reads/rewrites the sentinel row there only).
Every add_edge call that bumps degrees must rewrite the full sentinel blob
including all appended property bytes — this is the write-amplification cost.

───────────────────────────────────────────────────────────────────────────────
2.  SPLITEDGEKEY — COLUMNAR  (column groups)
───────────────────────────────────────────────────────────────────────────────

Structural tables are untouched.  Property bytes move to dedicated typed
tables where properties are grouped into column groups (separate B-trees).
Reading only creationDate from a million edges touches only the temporal B-tree.

┌─────────────────────────────────────────────────────────────────────────────┐
│ TABLE: edge_out    (STRUCTURAL — unchanged from embedded)                    │
│ key_format=uu   value_format=u                                               │
│ columns=(src, dst, attr)                                                     │
├────────────────────────────┬────────────────────────────────────────────────┤
│ KEY                        │ VALUE  (u blob — degrees only, no prop bytes)  │
├────────────────────────────┼────────────────────────────────────────────────┤
│ (MAKE_EKEY(id),  0)        │ [ in_deg:4B | out_deg:4B ]         total = 8B  │
│  node sentinel             │                                                │
├────────────────────────────┼────────────────────────────────────────────────┤
│ (MAKE_EKEY(src),           │ [ 0x00 ]                                       │
│  MAKE_EKEY(dst))           │  1-byte sentinel (no prop bytes here)  = 1B   │
│  edge row                  │                                                │
└────────────────────────────┴────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ TABLE: edge_in    (STRUCTURAL — directed only, unchanged)                    │
│ key_format=uu   value_format=u                                               │
├────────────────────────────┬────────────────────────────────────────────────┤
│ (MAKE_EKEY(dst),           │ [ 0x00 ]                           total = 1B  │
│  MAKE_EKEY(src))           │                                                │
└────────────────────────────┴────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ TABLE: person_props                                                          │
│ key_format=Q    value_format=QQb32s32s32s32s                                 │
│ columns=(vid, creationDate, birthday, gender,                                │
│          firstName, lastName, browserUsed, locationIP)                       │
│ colgroups=(temporal, name, contact)                                          │
├───────────────┬─────────────────────┬──────────────────┬────────────────────┤
│               │ colgroup:temporal   │ colgroup:name    │ colgroup:contact   │
│               │ (creationDate,      │ (firstName,      │ (browserUsed,      │
│ KEY           │  birthday)          │  lastName,       │  locationIP)       │
│ (vid:Q)       │  ← B-tree 1 ───────►│  gender)         │  ← B-tree 3 ──────►│
│               │                    │  ← B-tree 2 ─────►│                    │
├───────────────┼─────────────────────┼──────────────────┼────────────────────┤
│ person_id     │ epoch_ms:8B         │ str:32B          │ str:32B            │
│               │ epoch_ms:8B         │ str:32B          │ str:32B            │
│               │                    │ 0/1:1B           │                    │
└───────────────┴─────────────────────┴──────────────────┴────────────────────┘
  open_colgroup_cursor("person_props", "temporal") reads B-tree 1 only (16B/row).
  open_colgroup_cursor("person_props", "name") reads B-tree 2 only (65B/row).

┌─────────────────────────────────────────────────────────────────────────────┐
│ TABLE: post_props                                                            │
│ key_format=Q    value_format=QibS                                            │
│ columns=(vid, creationDate, length, tag, content)                            │
│ colgroups=(temporal, content)                                                │
├───────────────┬──────────────────────────┬─────────────────────────────────┤
│               │ colgroup:temporal        │ colgroup:content                │
│ KEY           │ (creationDate, length)   │ (tag, content)                  │
│ (vid:Q)       │  ← B-tree 1 ────────────►│  ← B-tree 2 ───────────────────►│
├───────────────┼──────────────────────────┼─────────────────────────────────┤
│ post_id       │  epoch_ms:8B             │  0/1:1B  (0=text, 1=imageFile)  │
│               │  byte_count:4B           │  variable-length string         │
└───────────────┴──────────────────────────┴─────────────────────────────────┘
  BI-1 / BI-12 scan only B-tree 1 (12B/row), never loading content strings.

┌─────────────────────────────────────────────────────────────────────────────┐
│ TABLE: knows_props                                                           │
│ key_format=QQ   value_format=Q                                               │
│ columns=(src, dst, creationDate)                                             │
│ colgroups=(temporal)                                                         │
├───────────────────────────┬────────────────────────────────────────────────┤
│ KEY                       │ colgroup:temporal  (creationDate)               │
│ (src:Q, dst:Q)            │  ← B-tree 1 ──────────────────────────────────►│
├───────────────────────────┼────────────────────────────────────────────────┤
│ (person_id, person_id)    │  epoch_ms:8B                                   │
└───────────────────────────┴────────────────────────────────────────────────┘
  R2/A2 range-scan from (person_id, 0): reads only 8B per knows edge.

┌─────────────────────────────────────────────────────────────────────────────┐
│ TABLE: likes_props                                                           │
│ key_format=QQ   value_format=Q                                               │
│ columns=(src, dst, creationDate)                                             │
│ colgroups=(temporal)                                                         │
├───────────────────────────┬────────────────────────────────────────────────┤
│ KEY                       │ colgroup:temporal  (creationDate)               │
│ (src:Q, dst:Q)            │  ← B-tree 1 ──────────────────────────────────►│
├───────────────────────────┼────────────────────────────────────────────────┤
│ (person_id, post_id)      │  epoch_ms:8B                                   │
└───────────────────────────┴────────────────────────────────────────────────┘
  A3 range-scan from (person_id, 0): reads only 8B per likes edge.

┌─────────────────────────────────────────────────────────────────────────────┐
│ TABLE: person_email    (secondary — multi-valued attribute)                  │
│ key_format=QQ   value_format=S                                               │
├───────────────────────────┬────────────────────────────────────────────────┤
│ KEY                       │ VALUE                                           │
│ (person_id:Q, idx:Q)      │  email address string                          │
├───────────────────────────┼────────────────────────────────────────────────┤
│ (pid, 0)                  │  "alice@example.com"                           │
│ (pid, 1)                  │  "alice@work.com"                              │
│  ...                      │  ...                                           │
└───────────────────────────┴────────────────────────────────────────────────┘
  get_person_emails: search_near(pid,0) + forward scan while key.pid matches.

┌─────────────────────────────────────────────────────────────────────────────┐
│ TABLE: person_speaks    (secondary — multi-valued attribute)                 │
│ key_format=QQ   value_format=S                                               │
├───────────────────────────┬────────────────────────────────────────────────┤
│ KEY                       │ VALUE                                           │
│ (person_id:Q, idx:Q)      │  language code string                          │
├───────────────────────────┼────────────────────────────────────────────────┤
│ (pid, 0)                  │  "en"                                          │
│ (pid, 1)                  │  "fr"                                          │
└───────────────────────────┴────────────────────────────────────────────────┘

───────────────────────────────────────────────────────────────────────────────
3.  ADJLIST — EMBEDDED
───────────────────────────────────────────────────────────────────────────────

The adjlist tables are FROZEN: value_format=u with no columns= is required for
WT_MODIFY support (O(1) neighbor append).  Column groups cannot be added to
tables with no named columns.  Properties are already in separate tables by
necessity — they cannot be embedded in the adjlist rows.

┌─────────────────────────────────────────────────────────────────────────────┐
│ TABLE: adjlistout                                                            │
│ key_format=u    value_format=u    (NO columns= — frozen for WT_MODIFY)       │
├────────────────────────────┬────────────────────────────────────────────────┤
│ KEY                        │ VALUE  (single u blob)                         │
│ (node_id:u)                │                                                │
├────────────────────────────┼────────────────────────────────────────────────┤
│ any node id                │ [ degree:4B | dst_0:4B | dst_1:4B | ... ]      │
│                            │  variable length; appended via WT_MODIFY       │
└────────────────────────────┴────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ TABLE: adjlistin    (directed only)                                          │
│ key_format=u    value_format=u    (NO columns= — frozen)                     │
├────────────────────────────┬────────────────────────────────────────────────┤
│ any node id                │ [ degree:4B | src_0:4B | src_1:4B | ... ]      │
└────────────────────────────┴────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ TABLE: node_props                                                            │
│ key_format=u    value_format=u                                               │
├────────────────────────────┬────────────────────────────────────────────────┤
│ KEY                        │ VALUE  (single u blob)                         │
│ (node_id:u)                │                                                │
├────────────────────────────┼────────────────────────────────────────────────┤
│ person_id (Person range)   │ [ creationDate:8B | birthday:8B | gender:1B    │
│                            │ | firstName:32B | lastName:32B                 │
│                            │ | browserUsed:32B | locationIP:32B ]           │
│                            │   schema decoded by VTYPE_OF(id)               │
│                            │                              total = 145B      │
├────────────────────────────┼────────────────────────────────────────────────┤
│ post_id   (Post range)     │ [ creationDate:8B | length:4B | tag:1B         │
│                            │ | content:variable ]                           │
│                            │         total = 13B + strlen(content)          │
└────────────────────────────┴────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ TABLE: edge_props    (inline blob, keyed by (src, dst))                      │
│ key_format=uu   value_format=u                                               │
├────────────────────────────┬────────────────────────────────────────────────┤
│ KEY                        │ VALUE  (single u blob)                         │
│ (src:u, dst:u)             │                                                │
├────────────────────────────┼────────────────────────────────────────────────┤
│ knows / likes (src, dst)   │ [ creationDate:8B ]              total = 8B    │
│                            │   edge type decoded by endpoint ID ranges      │
├────────────────────────────┼────────────────────────────────────────────────┤
│ hasCreator (post, person)  │  [ 0x00 ]  1-byte sentinel        total = 1B  │
└────────────────────────────┴────────────────────────────────────────────────┘

───────────────────────────────────────────────────────────────────────────────
4.  ADJLIST — COLUMNAR  (column groups)
───────────────────────────────────────────────────────────────────────────────

The adjlist tables are IDENTICAL to embedded mode — they cannot change.
Only the property tables change: node_props and edge_props are replaced by
per-type typed tables with column groups, identical in schema to SplitEdgeKey
COLUMNAR mode.

┌─────────────────────────────────────────────────────────────────────────────┐
│ TABLE: adjlistout   — UNCHANGED                                              │
│ TABLE: adjlistin    — UNCHANGED  (directed only)                             │
└─────────────────────────────────────────────────────────────────────────────┘

Tables person_props, post_props, knows_props, likes_props, person_email,
person_speaks are identical in schema to SplitEdgeKey COLUMNAR (Section 2).
The only difference is that the key for person_props/post_props uses the
typed node_id_t directly (MAKE_TYPED_ID encodes type in top 8 bits), so
VTYPE_OF(key) determines which property schema applies.

───────────────────────────────────────────────────────────────────────────────
Summary: table count per mode
───────────────────────────────────────────────────────────────────────────────

                     SplitEdgeKey                  AdjList
                 ┌──────────┬──────────┐       ┌──────────┬──────────┐
                 │ EMBEDDED │ COLUMNAR │       │ EMBEDDED │ COLUMNAR │
─────────────────┼──────────┼──────────┤       ├──────────┼──────────┤
Structural       │ edge_out │ edge_out │       │adjlistout│adjlistout│
                 │ edge_in  │ edge_in  │       │ adjlistin│ adjlistin│
─────────────────┼──────────┼──────────┤       ├──────────┼──────────┤
Node props       │ (inline  │person_   │       │node_props│person_  │
                 │  in      │  props   │       │  (blob)  │  props  │
                 │ edge_out)│post_props│       │          │post_props│
─────────────────┼──────────┼──────────┤       ├──────────┼──────────┤
Edge props       │ (inline  │knows_    │       │edge_props│knows_   │
                 │  in      │  props   │       │  (blob)  │  props  │
                 │ edge_out │likes_    │       │          │likes_   │
                 │ edge_in) │  props   │       │          │  props  │
─────────────────┼──────────┼──────────┤       ├──────────┼──────────┤
Secondary        │    —     │person_   │       │    —     │person_  │
                 │          │  email   │       │          │  email  │
                 │          │person_   │       │          │person_  │
                 │          │  speaks  │       │          │  speaks │
─────────────────┼──────────┼──────────┤       ├──────────┼──────────┤
Total tables     │    2     │    8     │       │   3–4    │  8–9 *  │
─────────────────┴──────────┴──────────┘       └──────────┴──────────┘
  * lower bound = undirected; directed adds adjlistin

Key structural differences:
  SplitEdgeKey EMBEDDED — node props share rows with degree data in edge_out;
                          every degree update rewrites the full 153B person blob.
  SplitEdgeKey COLUMNAR — edge_out carries degrees only (8B sentinel);
                          degree updates never touch property files.
                          knows_props and likes_props are separate tables
                          (not a shared edge_props) enabling per-type scans.
  AdjList EMBEDDED      — node_props and edge_props are separate opaque blob
                          tables; adjlist rows contain only neighbor IDs.
  AdjList COLUMNAR      — same typed tables as SplitEdgeKey COLUMNAR;
                          per-field B-tree projection reads identical to
                          SplitEdgeKey, making the comparison controlled.
