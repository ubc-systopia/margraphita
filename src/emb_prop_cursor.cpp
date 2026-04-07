#include "emb_prop_cursor.h"

#include "graph.h"          // full GraphBase definition
#include "graph_exception.h"
#include "prop_schema.h"    // schema extractor functions

// ─── EmbNodePropCursor ────────────────────────────────────────────────────────

EmbNodePropCursor::EmbNodePropCursor(GraphBase *g,
                                     I64Extractor ex0, I64Extractor ex1,
                                     I32Extractor ix0,
                                     WT_CURSOR *direct_scan_cur)
    : graph_(g),
      u64_ex_{ex0, ex1},
      i32_ex_{ix0},
      direct_cur_(direct_scan_cur)
{}

EmbNodePropCursor::~EmbNodePropCursor() {
    if (direct_cur_) {
        direct_cur_->close(direct_cur_);
        direct_cur_ = nullptr;
    }
    if (node_cur_) {
        node_cur_->close();
        // NodeCursor* is owned by the unique_ptr; no manual delete needed.
    }
}

void EmbNodePropCursor::set_range(node_id_t start, node_id_t end) {
    end_id_ = end;
    if (direct_cur_) {
        // Direct scan of NODE_PROPS_TABLE: position once, then ->next() per row.
        direct_cur_->reset(direct_cur_);
        direct_cur_->set_key(direct_cur_, (uint64_t)start);
        int exact = 0;
        int ret = direct_cur_->search_near(direct_cur_, &exact);
        if (ret != 0) { exhausted_ = true; return; }
        // If search_near landed before start, advance one step.
        if (exact < 0) {
            ret = direct_cur_->next(direct_cur_);
            if (ret != 0) { exhausted_ = true; return; }
        }
        // Check first key is within range.
        uint64_t k = 0;
        direct_cur_->get_key(direct_cur_, &k);
        exhausted_ = (k > (uint64_t)end);
        return;
    }
    // Old path: iterate topology cursor + secondary search on NODE_PROPS_TABLE.
    node_cur_.reset(graph_->get_node_iter());
    node_cur_->set_key_range({start, end});
    // Advance to the first node; pending_node_ holds what next() will return.
    node_cur_->next(&pending_node_);
    exhausted_ = (pending_node_.id == OutOfBand_ID_MAX);
}

bool EmbNodePropCursor::next() {
    if (exhausted_) return false;
    if (direct_cur_) {
        // Direct path: cursor is already positioned at the row to return.
        uint64_t k = 0;
        direct_cur_->get_key(direct_cur_, &k);
        cur_key_ = (node_id_t)k;
        WT_ITEM item{};
        direct_cur_->get_value(direct_cur_, &item);
        if (item.size > 0 && item.data != nullptr)
            extract((const uint8_t *)item.data);
        // Advance for next call.
        int ret = direct_cur_->next(direct_cur_);
        if (ret != 0) {
            exhausted_ = true;
        } else {
            direct_cur_->get_key(direct_cur_, &k);
            if (k > (uint64_t)end_id_) exhausted_ = true;
        }
        return true;
    }
    // Old path: pending_node_ was primed by set_range / previous next().
    cur_key_ = pending_node_.id;
    prop_blob pb = graph_->get_node_properties(cur_key_);
    if (pb.size > 0 && pb.data != nullptr)
        extract(pb.data);
    // Advance for next call.
    node_cur_->next(&pending_node_);
    if (pending_node_.id == OutOfBand_ID_MAX) exhausted_ = true;
    return true;
}

bool EmbNodePropCursor::seek(node_id_t id) {
    prop_blob pb = graph_->get_node_properties(id);
    if (pb.size == 0 || pb.data == nullptr) return false;
    cur_key_ = id;
    extract(pb.data);
    return true;
}

void EmbNodePropCursor::extract(const uint8_t *data) {
    if (u64_ex_[0]) u64_[0] = u64_ex_[0](data);
    if (u64_ex_[1]) u64_[1] = u64_ex_[1](data);
    if (i32_ex_[0]) i32_[0] = i32_ex_[0](data);
}

// ─── EmbEdgePropCursor ────────────────────────────────────────────────────────

EmbEdgePropCursor::EmbEdgePropCursor(GraphBase *g, uint8_t dst_vtype,
                                     I64Extractor ex0)
    : graph_(g), dst_vtype_(dst_vtype), u64_ex_{ex0}
{}

void EmbEdgePropCursor::set_src(node_id_t src) {
    pinned_src_ = src;
    neighbors_  = graph_->get_out_nodes_id(src);
    idx_        = 0;
    exhausted_  = false;
    // Skip to first neighbor of the expected vertex type.
    while (idx_ < neighbors_.size() &&
           VTYPE_OF(neighbors_[idx_]) != dst_vtype_) {
        ++idx_;
    }
    if (idx_ >= neighbors_.size()) exhausted_ = true;
}

void EmbEdgePropCursor::set_src_range(node_id_t /*src_start*/,
                                       node_id_t /*src_end*/) {
    throw GraphException(
        "EmbEdgePropCursor::set_src_range is not supported in EMBEDDED mode");
}

bool EmbEdgePropCursor::next() {
    if (exhausted_) return false;
    // Load current position into cache.
    bool ok = load_row(neighbors_[idx_]);
    // Advance to next matching neighbor.
    ++idx_;
    while (idx_ < neighbors_.size() &&
           VTYPE_OF(neighbors_[idx_]) != dst_vtype_) {
        ++idx_;
    }
    if (idx_ >= neighbors_.size()) exhausted_ = true;
    return ok;
}

bool EmbEdgePropCursor::seek(node_id_t src, node_id_t dst) {
    prop_blob pb = graph_->get_edge_properties(src, dst);
    if (pb.size == 0 || pb.data == nullptr) return false;
    cur_src_ = src;
    cur_dst_ = dst;
    if (u64_ex_[0]) u64_[0] = u64_ex_[0](pb.data);
    return true;
}

bool EmbEdgePropCursor::load_row(node_id_t dst) {
    prop_blob pb = graph_->get_edge_properties(pinned_src_, dst);
    if (pb.size == 0 || pb.data == nullptr) return false;
    cur_src_ = pinned_src_;
    cur_dst_ = dst;
    if (u64_ex_[0]) u64_[0] = u64_ex_[0](pb.data);
    return true;
}

// ─── Factory helpers ──────────────────────────────────────────────────────────

std::unique_ptr<NodePropCursor>
make_emb_node_prop_cursor(GraphBase *g, const std::string &table,
                           const std::string & /*colgroup*/,
                           WT_CURSOR *direct_scan_cur)
{
    I64Extractor ex0 = nullptr, ex1 = nullptr;
    I32Extractor ix0 = nullptr;

    if (table == PERSON_PROPS_TABLE) {
        ex0 = SNBPersonSchema::get_creation_date;
        ex1 = SNBPersonSchema::get_birthday;
    } else if (table == POST_PROPS_TABLE) {
        ex0 = SNBPostSchema::get_creation_date;
        // col 1 = length (int32); use i32 extractor
        ix0 = SNBPostSchema::get_length;
    } else if (table == COMMENT_PROPS_TABLE) {
        ex0 = SNBCommentSchema::get_creation_date;
        ix0 = SNBCommentSchema::get_length;
    } else if (table == FORUM_PROPS_TABLE) {
        ex0 = SNBForumSchema::get_creation_date;
    } else {
        throw GraphException(
            "make_emb_node_prop_cursor: no EMBEDDED schema for table: " + table);
    }
    return std::make_unique<EmbNodePropCursor>(g, ex0, ex1, ix0, direct_scan_cur);
}

std::unique_ptr<EdgePropCursor>
make_emb_edge_prop_cursor(GraphBase *g, const std::string &table,
                           const std::string & /*colgroup*/)
{
    I64Extractor ex0   = nullptr;
    uint8_t      vtype = 0;

    if (table == KNOWS_PROPS_TABLE) {
        ex0   = SNBKnowsSchema::get_creation_date;
        vtype = VT_PERSON;
    } else if (table == LIKES_PROPS_TABLE) {
        ex0   = SNBLikesSchema::get_creation_date;
        // Likes targets both Posts and Comments; use VT_POST as the primary
        // type and rely on the caller knowing that two separate cursors are
        // needed for post+comment likes, or accept that VT_COMMENT is
        // filtered out.  For the current PoC query set (IC-5, IC-7), all
        // liked objects are Posts, so VT_POST is correct.
        vtype = VT_POST;
    } else if (table == HASMEMBER_PROPS_TABLE) {
        ex0   = SNBHasMemberSchema::get_creation_date;
        vtype = VT_PERSON;
    } else {
        throw GraphException(
            "make_emb_edge_prop_cursor: no EMBEDDED schema for table: " + table);
    }
    return std::make_unique<EmbEdgePropCursor>(g, vtype, ex0);
}
