#pragma once

// snb_id_map.h — compact sorted-vector map from LDBC int64 IDs to Flexograph typed node_id_t.
//
// RAM advantage over std::unordered_map:
//   16 bytes/entry (8B key + 8B value) vs ~40–50 bytes/entry with bucket overhead.
//   For SF-10 comments (~15 M entries): ~240 MB vs ~600 MB.
//
// Usage contract:
//   Build phase  (single thread): call insert(), then sort() once.
//   Query phase  (many threads):  call lookup() concurrently — safe because the
//                                  vector is read-only after sort().

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

#include "common_defs.h"

class SnbIdMap {
public:
    // Sentinel returned when an LDBC ID is not found.
    static constexpr node_id_t INVALID = static_cast<node_id_t>(UINT64_MAX);

    void reserve(size_t n) { entries_.reserve(n); }

    // Append one mapping. Not thread-safe; call from the single build thread only.
    void insert(int64_t ldbc_id, node_id_t typed_id) {
        entries_.push_back({ldbc_id, typed_id});
    }

    // Sort by LDBC key. Must be called once after all insert() calls.
    void sort() {
        std::sort(entries_.begin(), entries_.end(),
                  [](const Entry& a, const Entry& b) { return a.first < b.first; });
    }

    // Binary search. Thread-safe after sort() has returned.
    node_id_t lookup(int64_t ldbc_id) const {
        auto it = std::lower_bound(entries_.begin(), entries_.end(),
                                   Entry{ldbc_id, 0},
                                   [](const Entry& a, const Entry& b) {
                                       return a.first < b.first;
                                   });
        if (it != entries_.end() && it->first == ldbc_id)
            return it->second;
        return INVALID;
    }

    size_t size()  const { return entries_.size(); }
    bool   empty() const { return entries_.empty(); }

private:
    using Entry = std::pair<int64_t, node_id_t>;
    std::vector<Entry> entries_;
};
