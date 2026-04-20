#ifndef WT_PROP_CURSOR_H
#define WT_PROP_CURSOR_H

#include <wiredtiger.h>

#include "common_defs.h"
#include "iterator.h"

// ─── ColgroupValueLayout ──────────────────────────────────────────────────────
// WiredTiger get_value() arg count must match the colgroup value format exactly.
// These three layouts cover all current temporal colgroups.
//
//  CVL_Q   — value_format="Q"   e.g. knows/likes/hasmember/forum temporal
//  CVL_QQ  — value_format="QQ"  e.g. person_props temporal (cDate + birthday)
//  CVL_Qi  — value_format="Qi"  e.g. post/comment_props temporal (cDate + length)
enum ColgroupValueLayout { CVL_Q, CVL_QQ, CVL_Qi };

// Select the layout for a given (table, colgroup) pair.
inline ColgroupValueLayout layout_for(const std::string &table,
                                       const std::string &cg)
{
    if (cg == CG_TEMPORAL) {
        if (table == POST_PROPS_TABLE || table == COMMENT_PROPS_TABLE)
            return CVL_Qi;
        if (table == PERSON_PROPS_TABLE)
            return CVL_QQ;
        // knows, likes, hasmember, forum, studyat, workat
        return CVL_Q;
    }
    // Other colgroups (name, contact, content, info) — default to CVL_Q.
    // Extend if these colgroups are ever scanned via NodePropCursor.
    return CVL_Q;
}

// ─── WTNodePropCursor ─────────────────────────────────────────────────────────
// NodePropCursor backed by a WiredTiger colgroup cursor (COLUMNAR mode).
// Destructor calls cur_->close(); no separate close() needed.
class WTNodePropCursor : public NodePropCursor {
public:
    WTNodePropCursor(WT_CURSOR *c, ColgroupValueLayout layout)
        : cur_(c), layout_(layout) {}

    ~WTNodePropCursor() override {
        if (cur_) cur_->close(cur_);
    }

    // Position at first row >= start; stop before end.
    void set_range(node_id_t start, node_id_t end) override {
        range_end_ = end;
        exhausted_ = false;
        cur_->set_key(cur_, (uint64_t)start);
        int cmp = 0;
        int ret  = cur_->search_near(cur_, &cmp);
        if (ret != 0) { exhausted_ = true; return; }
        if (cmp < 0) {
            ret = cur_->next(cur_);
            if (ret != 0) { exhausted_ = true; return; }
        }
        // Peek at the key to detect an already-out-of-range position.
        uint64_t k;
        cur_->get_key(cur_, &k);
        if ((node_id_t)k >= range_end_) exhausted_ = true;
    }

    // Return the current row to the caller, then advance the WT cursor.
    bool next() override {
        if (exhausted_) return false;
        load_current();
        int ret = cur_->next(cur_);
        if (ret != 0) {
            exhausted_ = true;
        } else {
            uint64_t k;
            cur_->get_key(cur_, &k);
            if ((node_id_t)k >= range_end_) exhausted_ = true;
        }
        return true;
    }

    bool seek(node_id_t id) override {
        cur_->set_key(cur_, (uint64_t)id);
        int ret = cur_->search(cur_);
        if (ret != 0) return false;
        load_current();
        return true;
    }

    node_id_t key()             const override { return cur_key_; }
    uint64_t  get_uint64(int n) const override { return u64_[n]; }
    int32_t   get_int32 (int n) const override { return i32_[n]; }

private:
    WT_CURSOR          *cur_      = nullptr;
    ColgroupValueLayout layout_;
    node_id_t           range_end_ = OutOfBand_ID_MAX;
    bool                exhausted_ = true;

    // Current-row cache (populated by load_current)
    node_id_t cur_key_  = 0;
    uint64_t  u64_[2]   = {};
    int32_t   i32_[1]   = {};

    void load_current() {
        uint64_t k;
        cur_->get_key(cur_, &k);
        cur_key_ = (node_id_t)k;
        if      (layout_ == CVL_Q)  cur_->get_value(cur_, &u64_[0]);
        else if (layout_ == CVL_QQ) cur_->get_value(cur_, &u64_[0], &u64_[1]);
        else /* CVL_Qi */           cur_->get_value(cur_, &u64_[0], &i32_[0]);
        // Note: use int32_t*, not int*, to match WiredTiger's "i" format
        // (sizeof(int) may differ from 4 on some platforms).
    }
};

// ─── WTEdgePropCursor ─────────────────────────────────────────────────────────
// EdgePropCursor backed by a WiredTiger colgroup cursor (COLUMNAR mode).
//
// layout:     controls how many value columns are read (same enum as WTNodePropCursor)
// node_keyed: true  → table key_format=Q  (post_props, comment_props)
//             false → table key_format=QQ (knows_props, likes_props, hasmember_props, …)
//
// Why node_keyed exists:
// Queries like X3 traverse hasCreator edges and then need temporal properties
// from the destination vertex table (post_props).  Rather than switching from
// EdgePropCursor to NodePropCursor mid-query — which would mean juggling two
// cursor interfaces with different signatures — the node_keyed flag lets this
// class adapt to single-key (Q) vertex tables transparently.  The query code
// stays uniform: set_src(), next(), get_uint64() regardless of whether the
// underlying table is (src,dst)-keyed or (node_id)-keyed.
// When node_keyed is true: set_src() does not pin (caller manages stop
// condition via VTYPE_OF), dst() returns OutOfBand_ID_MIN, and set_key_seek()
// passes a single key argument.
class WTEdgePropCursor : public EdgePropCursor {
public:
    explicit WTEdgePropCursor(WT_CURSOR *c,
                               ColgroupValueLayout layout = CVL_Q,
                               bool node_keyed = false)
        : cur_(c), layout_(layout), node_keyed_(node_keyed) {}

    ~WTEdgePropCursor() override {
        if (cur_) cur_->close(cur_);
    }

    // Pin to a single src; next() stops when src changes.
    // For node-keyed tables, pinning is suppressed: the caller checks
    // VTYPE_OF(cur->src()) to stop the scan at a type boundary.
    void set_src(node_id_t src) override {
        src_range_end_ = OutOfBand_ID_MAX;
        exhausted_     = false;
        set_key_seek(src, 0);
        int cmp = 0;
        int ret  = cur_->search_near(cur_, &cmp);
        if (ret != 0) { exhausted_ = true; return; }
        if (cmp < 0) {
            ret = cur_->next(cur_);
            if (ret != 0) { exhausted_ = true; return; }
        }
        if (node_keyed_) {
            // Don't pin: scan freely from src; caller manages stop condition.
            pinned_src_ = OutOfBand_ID_MAX;
        } else {
            pinned_src_ = src;
            if (read_current_src() != src) exhausted_ = true;
        }
    }

    // Scan all rows with src in [src_start, src_end).
    void set_src_range(node_id_t src_start, node_id_t src_end) override {
        pinned_src_    = OutOfBand_ID_MAX;
        src_range_end_ = src_end;
        exhausted_     = false;
        set_key_seek(src_start, 0);
        int cmp = 0;
        int ret  = cur_->search_near(cur_, &cmp);
        if (ret != 0) { exhausted_ = true; return; }
        if (cmp < 0) {
            ret = cur_->next(cur_);
            if (ret != 0) { exhausted_ = true; return; }
        }
        if (read_current_src() >= src_range_end_) exhausted_ = true;
    }

    bool next() override {
        if (exhausted_) return false;
        load_current();
        int ret = cur_->next(cur_);
        if (ret != 0) {
            exhausted_ = true;
        } else {
            node_id_t ns = read_current_src();
            if (pinned_src_ != OutOfBand_ID_MAX && ns != pinned_src_)
                exhausted_ = true;
            else if (ns >= src_range_end_)
                exhausted_ = true;
        }
        return true;
    }

    bool seek(node_id_t src, node_id_t dst) override {
        set_key_seek(src, dst);
        int ret = cur_->search(cur_);
        if (ret != 0) return false;
        load_current();
        return true;
    }

    node_id_t src()             const override { return cur_src_; }
    node_id_t dst()             const override { return cur_dst_; }
    uint64_t  get_uint64(int n) const override { return u64_[n]; }

private:
    WT_CURSOR          *cur_           = nullptr;
    ColgroupValueLayout layout_;
    bool                node_keyed_;
    node_id_t           pinned_src_    = OutOfBand_ID_MAX;
    node_id_t           src_range_end_ = OutOfBand_ID_MAX;
    bool                exhausted_     = true;

    // Current-row cache
    node_id_t cur_src_ = 0;
    node_id_t cur_dst_ = 0;
    uint64_t  u64_[2]  = {};
    int32_t   i32_[1]  = {};

    // Set the seek key; dst is ignored for node-keyed (1-key) tables.
    void set_key_seek(node_id_t src, node_id_t dst) {
        if (node_keyed_)
            cur_->set_key(cur_, (uint64_t)src);
        else
            cur_->set_key(cur_, (uint64_t)src, (uint64_t)dst);
    }

    // Read only the src component of the current cursor key.
    node_id_t read_current_src() {
        uint64_t s;
        if (node_keyed_) {
            cur_->get_key(cur_, &s);
        } else {
            uint64_t d;
            cur_->get_key(cur_, &s, &d);
        }
        return (node_id_t)s;
    }

    void load_current() {
        uint64_t s;
        if (node_keyed_) {
            cur_->get_key(cur_, &s);
            cur_src_ = (node_id_t)s;
            cur_dst_ = OutOfBand_ID_MIN;
        } else {
            uint64_t d;
            cur_->get_key(cur_, &s, &d);
            cur_src_ = (node_id_t)s;
            cur_dst_ = (node_id_t)d;
        }
        if      (layout_ == CVL_Q)  cur_->get_value(cur_, &u64_[0]);
        else if (layout_ == CVL_QQ) cur_->get_value(cur_, &u64_[0], &u64_[1]);
        else /* CVL_Qi */ {
            cur_->get_value(cur_, &u64_[0], &i32_[0]);
            u64_[1] = (uint64_t)(uint32_t)i32_[0];  // get_uint64(1) castable back to int32_t
        }
    }
};

#endif  // WT_PROP_CURSOR_H
