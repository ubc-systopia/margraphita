#ifndef EMB_PROP_CURSOR_H
#define EMB_PROP_CURSOR_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "common_defs.h"
#include "iterator.h"

// Forward declaration — avoids circular include.
// (graph.h includes edgekey_split.h/adj_list.h which include this header;
// including graph.h here would create a cycle.)
// Full definition is only needed in emb_prop_cursor.cpp.
class GraphBase;

// Function pointers for extracting typed fields from EMBEDDED property blobs.
// The signature matches the schema accessor functions in prop_schema.h.
using I64Extractor = int64_t (*)(const uint8_t *);
using I32Extractor = int32_t (*)(const uint8_t *);

// ─── EmbNodePropCursor ────────────────────────────────────────────────────────
// NodePropCursor backed by get_node_properties() (EMBEDDED mode).
// Sequential scan (set_range + next) uses GraphBase::get_node_iter().
// Point lookup (seek) calls get_node_properties() directly.
class EmbNodePropCursor : public NodePropCursor {
public:
    // Construct with schema extractors.  Up to 2 uint64 columns and 1 int32.
    // Pass nullptr for unused extractor slots.
    EmbNodePropCursor(GraphBase *g,
                      I64Extractor ex0, I64Extractor ex1,
                      I32Extractor ix0);
    ~EmbNodePropCursor() override;

    void      set_range(node_id_t start, node_id_t end) override;
    bool      next()                                    override;
    bool      seek(node_id_t id)                        override;

    node_id_t key()             const override { return cur_key_; }
    uint64_t  get_uint64(int n) const override { return (uint64_t)u64_[n]; }
    int32_t   get_int32 (int n) const override { return i32_[n]; }

private:
    GraphBase    *graph_;
    I64Extractor  u64_ex_[2];
    I32Extractor  i32_ex_[1];

    // Sequential scan state
    std::unique_ptr<NodeCursor> node_cur_;
    node    pending_node_{};  // next node to return (filled by set_range / next)
    bool    exhausted_ = true;

    // Current-row cache
    node_id_t cur_key_ = 0;
    int64_t   u64_[2]  = {};
    int32_t   i32_[1]  = {};

    void extract(const uint8_t *data);
};

// ─── EmbEdgePropCursor ────────────────────────────────────────────────────────
// EdgePropCursor backed by get_out_nodes_id() + get_edge_properties() (EMBEDDED).
// set_src() pre-fetches the full neighbor list and filters by expected dst vtype.
// next() issues one get_edge_properties() call per neighbor.
class EmbEdgePropCursor : public EdgePropCursor {
public:
    // dst_vtype: expected vertex type byte for the destination node.
    //            Neighbors with VTYPE_OF(dst) != dst_vtype are skipped.
    EmbEdgePropCursor(GraphBase *g, uint8_t dst_vtype, I64Extractor ex0);
    ~EmbEdgePropCursor() override = default;

    void      set_src(node_id_t src)                              override;
    void      set_src_range(node_id_t src_start, node_id_t src_end) override;
    bool      next()                                              override;
    bool      seek(node_id_t src, node_id_t dst)                 override;

    node_id_t src()             const override { return cur_src_; }
    node_id_t dst()             const override { return cur_dst_; }
    uint64_t  get_uint64(int n) const override { return (uint64_t)u64_[n]; }

private:
    GraphBase              *graph_;
    uint8_t                 dst_vtype_;
    I64Extractor            u64_ex_[1];

    // Iteration state
    node_id_t               pinned_src_ = 0;
    std::vector<node_id_t>  neighbors_;
    size_t                  idx_      = 0;
    bool                    exhausted_ = true;

    // Current-row cache
    node_id_t cur_src_ = 0;
    node_id_t cur_dst_ = 0;
    int64_t   u64_[1]  = {};

    bool load_row(node_id_t dst);
};

// ─── Factory helpers ──────────────────────────────────────────────────────────
// Select the right schema extractors for the given table and construct the
// appropriate EMBEDDED cursor.  Called from the factory method bodies in
// edgekey_split.cpp / adj_list.cpp.
// Implemented in emb_prop_cursor.cpp (which can #include "graph.h" freely).

std::unique_ptr<NodePropCursor>
make_emb_node_prop_cursor(GraphBase *g, const std::string &table,
                           const std::string &colgroup);

std::unique_ptr<EdgePropCursor>
make_emb_edge_prop_cursor(GraphBase *g, const std::string &table,
                           const std::string &colgroup);

#endif  // EMB_PROP_CURSOR_H
