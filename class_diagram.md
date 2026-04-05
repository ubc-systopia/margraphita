# Flexograph Class Diagrams

Three diagrams covering the full class hierarchy:
1. **Graph layer** — GraphEngine, GraphBase, AdjList, SplitEdgeKey
2. **Topology cursors** — table_iterator hierarchy and concrete implementations
3. **Property cursors** — NodePropCursor/EdgePropCursor hierarchy

---

## 1. Graph Layer

```mermaid
classDiagram
    direction TB

    class graph_opts {
        +bool read_only
        +bool create_new
        +bool read_optimize
        +bool is_directed
        +bool is_weighted
        +string db_name
        +string db_dir
        +GraphType type
        +node_id_t num_nodes
        +uint64_t num_edges
        +int num_threads
        +PropStorageMode prop_mode
        +bool has_node_props
        +bool has_edge_props
    }

    class PartitionStrategy {
        <<enumeration>>
        NODE_COUNT
        EDGE_AWARE
        NODE_COUNT_FINE
    }

    class GraphEngine {
        +int num_threads
        #graph_opts opts
        #WT_CONNECTION* conn
        #vector~node_id_t~ node_ranges
        #PartitionStrategy partition_strategy
        +GraphEngine(num_threads, opts)
        +create_graph_handle() GraphBase*
        +create_ro_graph_handle(checkpoint) GraphBase*
        +calculate_thread_offsets(make_edge)
        +get_key_range(thread_id) key_range
        +get_edge_range(thread_id) edge_range
        +get_work_chunks(chunk_size) vector~key_range~
        +make_checkpoint() string
        +close_graph()
        +set_partition_strategy(PartitionStrategy)
        +get_total_partitions() int
    }

    class GraphBase {
        <<abstract>>
        #graph_opts opts
        #WT_CONNECTION* connection
        #WT_SESSION* session
        #WT_CURSOR* metadata_cursor
        +add_node(node, is_bulk)* int
        +delete_node(node_id_t)* int
        +has_node(node_id_t)* bool
        +get_node(node_id_t)* node
        +get_random_node()* node
        +add_edge(edge, is_bulk)* int
        +delete_edge(src, dst)* int
        +has_edge(src, dst)* bool
        +get_edge(src, dst)* edge
        +update_edge(edge)* bool
        +get_out_degree(node_id_t)* degree_t
        +get_in_degree(node_id_t)* degree_t
        +get_out_nodes_id(node_id_t)* vector~node_id_t~
        +get_in_nodes_id(node_id_t)* vector~node_id_t~
        +get_out_edges(node_id_t)* vector~edge~
        +get_in_edges(node_id_t)* vector~edge~
        +set_node_properties(id, data, size)*
        +get_node_properties(id)* prop_blob
        +set_edge_properties(src, dst, data, size)*
        +get_edge_properties(src, dst)* prop_blob
        +get_node_prop_cursor(table, cg)* unique_ptr~NodePropCursor~
        +get_edge_prop_cursor(table, cg)* unique_ptr~EdgePropCursor~
        +get_outnbd_iter()* OutCursor*
        +get_innbd_iter()* InCursor*
        +get_node_iter()* NodeCursor*
        +get_edge_iter()* EdgeCursor*
        +add_person_email(id, idx, email)
        +add_person_language(id, idx, lang)
        +get_person_emails(id) vector~string~
        +get_person_languages(id) vector~string~
        +get_num_nodes() node_id_t
        +get_num_edges() edge_id_t
        +get_max_node_id()* node_id_t
        +get_min_node_id()* node_id_t
        +close(synchronize)
    }

    class AdjList {
        -WT_CURSOR* node_cursor
        -WT_CURSOR* edge_cursor
        -WT_CURSOR* in_adjlist_cursor
        -WT_CURSOR* out_adjlist_cursor
        -WT_CURSOR* node_props_cursor
        -WT_CURSOR* person_props_cursor
        -WT_CURSOR* post_props_cursor
        -WT_CURSOR* knows_props_cursor
        -WT_CURSOR* likes_props_cursor
        -WT_CURSOR* hasmember_props_cursor
        +AdjList(opts, connection)
        +create_wt_tables(opts, conn)$
        +get_node_iter() NodeCursor*
        +get_outnbd_iter() OutCursor*
        +get_innbd_iter() InCursor*
        +get_edge_iter() EdgeCursor*
        +get_node_prop_cursor(table, cg) unique_ptr~NodePropCursor~
        +get_edge_prop_cursor(table, cg) unique_ptr~EdgePropCursor~
    }

    class SplitEdgeKey {
        -WT_CURSOR* out_edge_cursor
        -WT_CURSOR* in_edge_cursor
        -WT_CURSOR* rd_out_cursor
        -WT_CURSOR* rd_in_cursor
        -WT_CURSOR* degree_cursor
        -WT_CURSOR* person_props_cursor
        -WT_CURSOR* post_props_cursor
        -WT_CURSOR* knows_props_cursor
        -WT_CURSOR* likes_props_cursor
        -WT_CURSOR* hasmember_props_cursor
        +SplitEdgeKey(opts, connection)
        +create_wt_tables(opts, conn)$
        +get_node_iter() NodeCursor*
        +get_outnbd_iter() OutCursor*
        +get_innbd_iter() InCursor*
        +get_edge_iter() EdgeCursor*
        +get_node_prop_cursor(table, cg) unique_ptr~NodePropCursor~
        +get_edge_prop_cursor(table, cg) unique_ptr~EdgePropCursor~
    }

    GraphEngine *-- graph_opts
    GraphEngine ..> GraphBase : creates
    GraphEngine --> PartitionStrategy

    GraphBase *-- graph_opts
    GraphBase <|-- AdjList
    GraphBase <|-- SplitEdgeKey
```

---

## 2. Topology Cursors

```mermaid
classDiagram
    direction TB

    class table_iterator {
        #WT_CURSOR* cursor
        #WT_SESSION* session
        #bool is_first
        #bool has_next
        #bool directed
        #bool read_opt
        +has_more() bool
        +reset()
        +close()
    }

    class OutCursor {
        <<abstract>>
        #key_range keys
        #node_id_t num_nodes
        +set_key_range(key_range)*
        +set_num_nodes(uint32_t)
        +next(adjlist*)*
        +next(adjlist*, node_id_t)*
    }

    class InCursor {
        <<abstract>>
        #key_range keys
        #node_id_t num_nodes
        +set_key_range(key_range)*
        +set_num_nodes(node_id_t)
        +next(adjlist*)*
        +next(adjlist*, node_id_t)*
    }

    class NodeCursor {
        <<abstract>>
        #key_range keys
        +set_key_range(key_range)*
        +next(node*)*
        +next(node*, node_id_t)*
    }

    class EdgeCursor {
        <<abstract>>
        #key_pair start_edge
        #key_pair end_edge
        #bool get_weight
        +set_key_range(edge_range)*
        +next(edge*)*
        +dump_range()
    }

    class AdjOutCursor {
        -bool all_nodes
        +AdjOutCursor(cursor, session)
        +AdjOutCursor(cursor, session, directed, read_opt)
        +setAllNodes(bool)
        +set_key_range(key_range)
        +next(adjlist*)
        +next(adjlist*, node_id_t)
    }

    class AdjInCursor {
        -bool all_nodes
        +AdjInCursor(cursor, session)
        +AdjInCursor(cursor, session, directed, read_opt)
        +setAllNodes(bool)
        +set_key_range(key_range)
        +next(adjlist*)
        +next(adjlist*, node_id_t)
    }

    class AdjNodeCursor {
        -WT_CURSOR* in_cur
        +AdjNodeCursor(cursor, session)
        +AdjNodeCursor(cursor, session, directed, read_opt)
        +set_key_range(key_range)
        +next(node*)
        +next(node*, node_id_t)
    }

    class AdjEdgeCursor {
        -adjlist current_adjlist
        -int pos
        +AdjEdgeCursor(cursor, session)
        +AdjEdgeCursor(cursor, session, directed, read_opt)
        +set_key_range(edge_range)
        +next(edge*)
    }

    class SplitEKeyOutCursor {
        +SplitEKeyOutCursor(cursor, session, directed, read_opt)
        +set_key_range(key_range)
        +next(adjlist*)
        +next(adjlist*, node_id_t)
    }

    class SplitEkeyInCursor {
        +SplitEkeyInCursor(cursor, session, directed, read_opt)
        +set_key_range(key_range)
        +next(adjlist*)
        +next(adjlist*, node_id_t)
    }

    class SplitEKeyNodeCursor {
        +SplitEKeyNodeCursor(cursor, session)
        +set_key_range(key_range)
        +next(node*)
        +next(node*, node_id_t)
    }

    class SplitEKeyEdgeCursor {
        +SplitEKeyEdgeCursor(cursor, session, directed, read_opt)
        +set_key_range(edge_range)
        +next(edge*)
    }

    table_iterator <|-- OutCursor
    table_iterator <|-- InCursor
    table_iterator <|-- NodeCursor
    table_iterator <|-- EdgeCursor

    OutCursor  <|-- AdjOutCursor
    InCursor   <|-- AdjInCursor
    NodeCursor <|-- AdjNodeCursor
    EdgeCursor <|-- AdjEdgeCursor

    OutCursor  <|-- SplitEKeyOutCursor
    InCursor   <|-- SplitEkeyInCursor
    NodeCursor <|-- SplitEKeyNodeCursor
    EdgeCursor <|-- SplitEKeyEdgeCursor
```

---

## 3. Property Cursors

```mermaid
classDiagram
    direction TB

    class NodePropCursor {
        <<abstract>>
        +set_range(start, end)*
        +next()* bool
        +seek(node_id_t)* bool
        +key()* node_id_t
        +get_uint64(n)* uint64_t
        +get_int32(n)* int32_t
    }

    class EdgePropCursor {
        <<abstract>>
        +set_src(src)*
        +set_src_range(start, end)*
        +next()* bool
        +seek(src, dst)* bool
        +src()* node_id_t
        +dst()* node_id_t
        +get_uint64(n)* uint64_t
    }

    class ColgroupValueLayout {
        <<enumeration>>
        CVL_Q
        CVL_QQ
        CVL_Qi
    }

    class WTNodePropCursor {
        -WT_CURSOR* cur_
        -ColgroupValueLayout layout_
        -node_id_t range_end_
        -bool exhausted_
        -node_id_t cur_key_
        -uint64_t u64_[2]
        -int32_t i32_[1]
        +WTNodePropCursor(cursor, layout)
        +set_range(start, end)
        +next() bool
        +seek(node_id_t) bool
        +key() node_id_t
        +get_uint64(n) uint64_t
        +get_int32(n) int32_t
    }

    class WTEdgePropCursor {
        -WT_CURSOR* cur_
        -node_id_t pinned_src_
        -node_id_t src_range_end_
        -bool exhausted_
        -node_id_t cur_src_
        -node_id_t cur_dst_
        -uint64_t u64_[1]
        +WTEdgePropCursor(cursor)
        +set_src(src)
        +set_src_range(start, end)
        +next() bool
        +seek(src, dst) bool
        +src() node_id_t
        +dst() node_id_t
        +get_uint64(n) uint64_t
    }

    class EmbNodePropCursor {
        -GraphBase* graph_
        -I64Extractor u64_ex_[2]
        -I32Extractor i32_ex_[1]
        -unique_ptr~NodeCursor~ node_cur_
        -node pending_node_
        -bool exhausted_
        -node_id_t cur_key_
        +EmbNodePropCursor(graph, ex0, ex1, ix0)
        +set_range(start, end)
        +next() bool
        +seek(node_id_t) bool
        +key() node_id_t
        +get_uint64(n) uint64_t
        +get_int32(n) int32_t
    }

    class EmbEdgePropCursor {
        -GraphBase* graph_
        -uint8_t dst_vtype_
        -I64Extractor u64_ex_[1]
        -node_id_t pinned_src_
        -vector~node_id_t~ neighbors_
        -size_t idx_
        -bool exhausted_
        +EmbEdgePropCursor(graph, dst_vtype, ex0)
        +set_src(src)
        +set_src_range(start, end)
        +next() bool
        +seek(src, dst) bool
        +src() node_id_t
        +dst() node_id_t
        +get_uint64(n) uint64_t
    }

    NodePropCursor <|-- WTNodePropCursor
    NodePropCursor <|-- EmbNodePropCursor
    EdgePropCursor <|-- WTEdgePropCursor
    EdgePropCursor <|-- EmbEdgePropCursor

    WTNodePropCursor --> ColgroupValueLayout
    WTEdgePropCursor ..> ColgroupValueLayout : layout_for() helper

    EmbNodePropCursor o-- GraphBase
    EmbEdgePropCursor o-- GraphBase

    class GraphBase {
        <<abstract>>
        +get_node_prop_cursor(table, cg)* unique_ptr~NodePropCursor~
        +get_edge_prop_cursor(table, cg)* unique_ptr~EdgePropCursor~
    }

    GraphBase ..> NodePropCursor : factory creates
    GraphBase ..> EdgePropCursor : factory creates
```
