// test_IIu_schema.cpp
//
// Minimal standalone WiredTiger experiment:
//   key_format  = u  (raw byte string — uint32_t stored big-endian for sort order)
//   value_format = IIu  (uint32_t, uint32_t, raw byte blob of fixed size PROP_BLOB_SIZE)
//
// Tests: insert, full scan, point lookup, update.
// All blobs are exactly PROP_BLOB_SIZE bytes (simulates a fixed property schema).

#include <wiredtiger.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>

// All property blobs in this test are this many bytes.
// Simulate a single int64_t property (e.g., a creationDate epoch-ms timestamp).
static constexpr size_t PROP_BLOB_SIZE = 8;

// ---- Key helpers -----------------------------------------------------------
//
// IMPORTANT: set_key_u32 uses thread_local static storage for the bswap buffer.
//
// WiredTiger's cursor->set_key stores cursor->key.data = item->data — a raw
// pointer, not a copy.  The actual key bytes are consumed only when the next
// cursor operation (insert/search/update) executes.  If `be` were a plain
// local variable, it would go out of scope before that operation runs, leaving
// a dangling pointer.  `inline` is not sufficient: at -O0 the compiler does
// not guarantee inlining, so the stack frame is still released on return.
//
// Using `static thread_local` makes `be` persist in static storage (per
// thread) for the lifetime of the program.  Since we always follow
// set_key_u32 with exactly one cursor operation before the next set_key_u32
// call, there is no aliasing hazard.
//
// Flexograph's CommonUtil::set_key sidesteps this by living in a header and
// being a true inlined expression — the key variable lives in the caller's
// frame.  This thread_local approach is the equivalent for a standalone .cpp.

// Store uint32 keys big-endian in the u-format key column so that WiredTiger's
// raw-byte sort order matches numeric order.
static void set_key_u32(WT_CURSOR *cursor, uint32_t id)
{
    static thread_local uint32_t be;
    be = __builtin_bswap32(id);
    WT_ITEM k = { &be, sizeof(be) };
    cursor->set_key(cursor, &k);
}

static inline uint32_t get_key_u32(WT_CURSOR *cursor)
{
    WT_ITEM k{};
    cursor->get_key(cursor, &k);
    uint32_t be;
    memcpy(&be, k.data, sizeof(be));
    return __builtin_bswap32(be);
}

// ---- Value helpers ---------------------------------------------------------
//
// set_value_IIu: same thread_local treatment for the WT_ITEM blob.
//
// WiredTiger's cursor->set_value for a `u` field stores a pointer to the
// WT_ITEM, which in turn holds a pointer to blob_data.  The integers (I, I)
// are packed by value immediately, so they are safe.  The `u` WT_ITEM is read
// on the subsequent cursor operation, so `blob` must not go out of scope.
// thread_local static keeps it alive without heap allocation.
//
// blob_data itself (the caller's property byte array) must also stay alive
// through the cursor operation — this is satisfied because it lives in main's
// stack frame or in a vector<uint8_t>.
//
// value_format = IIu:
//   a    → uint32_t (packed by value immediately, no pointer hazard)
//   b    → uint32_t
//   blob → WT_ITEM* (pointer stored; must outlive cursor operation)
//
// After get_value, WT_ITEM.data points into WiredTiger's internal buffer.
// Copy blob->data before the next cursor operation if you need it longer.

static void set_value_IIu(WT_CURSOR *cursor,
                           uint32_t a,
                           uint32_t b,
                           const uint8_t *blob_data)
{
    static thread_local WT_ITEM blob;
    blob.data = blob_data;
    blob.size = PROP_BLOB_SIZE;
    cursor->set_value(cursor, a, b, &blob);
}

static inline void get_value_IIu(WT_CURSOR *cursor,
                                  uint32_t *a,
                                  uint32_t *b,
                                  WT_ITEM *blob)
{
    cursor->get_value(cursor, a, b, blob);
}

// ---- Main ------------------------------------------------------------------

int main()
{
    const std::string db_dir = "/tmp/test_IIu_wt";
    std::filesystem::remove_all(db_dir);
    std::filesystem::create_directories(db_dir);

    WT_CONNECTION *conn    = nullptr;
    WT_SESSION   *session  = nullptr;
    WT_CURSOR    *cursor   = nullptr;
    int ret;

    // ---- Open connection ---------------------------------------------------
    ret = wiredtiger_open(db_dir.c_str(), nullptr, "create", &conn);
    if (ret != 0) {
        std::cerr << "wiredtiger_open: " << wiredtiger_strerror(ret) << "\n";
        return 1;
    }

    ret = conn->open_session(conn, nullptr, nullptr, &session);
    assert(ret == 0);

    // ---- Create table ------------------------------------------------------
    // Named columns make the schema self-documenting and are required later if
    // we want to add column groups (colgroup:test_IIu/cg_prop_blob etc.).
    ret = session->create(session,
                          "table:test_IIu",
                          "key_format=u,"
                          "value_format=IIu,"
                          "columns=(node_id,in_deg,out_deg,prop_blob)");
    assert(ret == 0);

    // ---- Open cursor -------------------------------------------------------
    ret = session->open_cursor(session, "table:test_IIu",
                               nullptr, nullptr, &cursor);
    assert(ret == 0);

    // ---- Insert N records --------------------------------------------------
    const int N = 8;
    std::cout << "Inserting " << N << " records...\n";
    for (int i = 0; i < N; i++) {
        uint32_t id      = static_cast<uint32_t>(i);
        uint32_t in_deg  = id * 2;
        uint32_t out_deg = id * 3;

        // Property blob: a single int64_t simulating a timestamp (epoch ms).
        int64_t  ts = static_cast<int64_t>(i) * 1'000'000LL;
        uint8_t  blob[PROP_BLOB_SIZE];
        memcpy(blob, &ts, sizeof(ts));

        set_key_u32(cursor, id);
        set_value_IIu(cursor, in_deg, out_deg, blob);
        ret = cursor->insert(cursor);
        assert(ret == 0);
    }

    // ---- Full scan + verify ------------------------------------------------
    cursor->reset(cursor);
    std::cout << "\nFull scan:\n";
    int count = 0;
    while ((ret = cursor->next(cursor)) == 0) {
        uint32_t id = get_key_u32(cursor);

        uint32_t in_deg, out_deg;
        WT_ITEM  blob{};
        get_value_IIu(cursor, &in_deg, &out_deg, &blob);

        assert(blob.size == PROP_BLOB_SIZE);
        int64_t ts;
        memcpy(&ts, blob.data, sizeof(ts));

        // Verify values match what was inserted.
        assert(in_deg  == id * 2);
        assert(out_deg == id * 3);
        assert(ts      == static_cast<int64_t>(id) * 1'000'000LL);

        std::cout << "  id=" << id
                  << "  in_deg=" << in_deg
                  << "  out_deg=" << out_deg
                  << "  ts=" << ts << "\n";
        count++;
    }
    assert(ret == WT_NOTFOUND && "scan terminated unexpectedly");
    assert(count == N);
    std::cout << "Scan OK (" << count << " records).\n";

    // ---- Point lookup: id = 5 ----------------------------------------------
    std::cout << "\nPoint lookup id=5:\n";
    cursor->reset(cursor);
    set_key_u32(cursor, 5);
    ret = cursor->search(cursor);
    assert(ret == 0 && "lookup id=5 failed");
    {
        uint32_t in_deg, out_deg;
        WT_ITEM  blob{};
        get_value_IIu(cursor, &in_deg, &out_deg, &blob);
        int64_t ts;
        memcpy(&ts, blob.data, sizeof(ts));

        std::cout << "  in_deg=" << in_deg
                  << "  out_deg=" << out_deg
                  << "  ts=" << ts << "\n";

        assert(in_deg  == 10);
        assert(out_deg == 15);
        assert(ts      == 5'000'000LL);
    }
    std::cout << "Lookup OK.\n";

    // ---- Update id = 3 -----------------------------------------------------
    // Change in_deg, out_deg, and the blob while the key stays the same.
    std::cout << "\nUpdating id=3...\n";
    set_key_u32(cursor, 3);
    ret = cursor->search(cursor);
    assert(ret == 0);
    {
        int64_t new_ts = 9'999'999LL;
        uint8_t blob[PROP_BLOB_SIZE];
        memcpy(blob, &new_ts, sizeof(new_ts));
        set_value_IIu(cursor, 100, 200, blob);
        ret = cursor->update(cursor);
        assert(ret == 0);
    }

    // Read back and verify the update.
    set_key_u32(cursor, 3);
    ret = cursor->search(cursor);
    assert(ret == 0);
    {
        uint32_t in_deg, out_deg;
        WT_ITEM  blob{};
        get_value_IIu(cursor, &in_deg, &out_deg, &blob);
        int64_t ts;
        memcpy(&ts, blob.data, sizeof(ts));

        std::cout << "  in_deg=" << in_deg
                  << "  out_deg=" << out_deg
                  << "  ts=" << ts << "\n";

        assert(in_deg  == 100);
        assert(out_deg == 200);
        assert(ts      == 9'999'999LL);
    }
    std::cout << "Update OK.\n";

    // ---- Cleanup -----------------------------------------------------------
    cursor->close(cursor);
    session->close(session, nullptr);
    conn->close(conn, nullptr);

    std::cout << "\nAll assertions passed.\n";
    return 0;
}
