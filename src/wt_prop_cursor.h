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
// Edge prop colgroups for temporal data use value_format="Q" (one uint64).
class WTEdgePropCursor : public EdgePropCursor {
public:
    explicit WTEdgePropCursor(WT_CURSOR *c) : cur_(c) {}

    ~WTEdgePropCursor() override {
        if (cur_) cur_->close(cur_);
    }

    // Pin to a single src; next() stops when src changes.
    void set_src(node_id_t src) override {
        pinned_src_    = src;
        src_range_end_ = OutOfBand_ID_MAX;
        exhausted_     = false;
        cur_->set_key(cur_, (uint64_t)src, (uint64_t)0);
        int cmp = 0;
        int ret  = cur_->search_near(cur_, &cmp);
        if (ret != 0) { exhausted_ = true; return; }
        if (cmp < 0) {
            ret = cur_->next(cur_);
            if (ret != 0) { exhausted_ = true; return; }
        }
        uint64_t s, d;
        cur_->get_key(cur_, &s, &d);
        if ((node_id_t)s != src) exhausted_ = true;
    }

    // Scan all edges with src in [src_start, src_end).
    void set_src_range(node_id_t src_start, node_id_t src_end) override {
        pinned_src_    = OutOfBand_ID_MAX;
        src_range_end_ = src_end;
        exhausted_     = false;
        cur_->set_key(cur_, (uint64_t)src_start, (uint64_t)0);
        int cmp = 0;
        int ret  = cur_->search_near(cur_, &cmp);
        if (ret != 0) { exhausted_ = true; return; }
        if (cmp < 0) {
            ret = cur_->next(cur_);
            if (ret != 0) { exhausted_ = true; return; }
        }
        uint64_t s, d;
        cur_->get_key(cur_, &s, &d);
        if ((node_id_t)s >= src_range_end_) exhausted_ = true;
    }

    bool next() override {
        if (exhausted_) return false;
        load_current();
        int ret = cur_->next(cur_);
        if (ret != 0) {
            exhausted_ = true;
        } else {
            uint64_t s, d;
            cur_->get_key(cur_, &s, &d);
            node_id_t ns = (node_id_t)s;
            if (pinned_src_ != OutOfBand_ID_MAX && ns != pinned_src_)
                exhausted_ = true;
            else if (ns >= src_range_end_)
                exhausted_ = true;
        }
        return true;
    }

    bool seek(node_id_t src, node_id_t dst) override {
        cur_->set_key(cur_, (uint64_t)src, (uint64_t)dst);
        int ret = cur_->search(cur_);
        if (ret != 0) return false;
        load_current();
        return true;
    }

    node_id_t src()             const override { return cur_src_; }
    node_id_t dst()             const override { return cur_dst_; }
    uint64_t  get_uint64(int n) const override { return u64_[n]; }

private:
    WT_CURSOR *cur_           = nullptr;
    node_id_t  pinned_src_    = OutOfBand_ID_MAX;
    node_id_t  src_range_end_ = OutOfBand_ID_MAX;
    bool       exhausted_     = true;

    // Current-row cache
    node_id_t cur_src_ = 0;
    node_id_t cur_dst_ = 0;
    uint64_t  u64_[1]  = {};

    void load_current() {
        uint64_t s, d;
        cur_->get_key(cur_, &s, &d);
        cur_src_ = (node_id_t)s;
        cur_dst_ = (node_id_t)d;
        cur_->get_value(cur_, &u64_[0]);
    }
};

#endif  // WT_PROP_CURSOR_H
