// test_typed_id.cpp
// Checks that the bit-reservation vertex-ID scheme in common_defs.h works
// correctly and does NOT overflow when B64 is active.
//
// Failures indicate either:
//   (a) B64 was not defined at compile time (32-bit node_id_t cannot hold 56-bit shift)
//   (b) MAKE_EKEY(+1) overflows into the type bits
//
// Build: same cmake flags as ldbc_snb_queries, i.e. with -DB64=ON

#include <cassert>
#include <cstdint>
#include <cstdio>

#include "common_defs.h"

static int failures = 0;

#define CHECK(cond, msg, ...)                                      \
    do {                                                           \
        if (!(cond)) {                                             \
            fprintf(stderr, "FAIL: " msg "\n", ##__VA_ARGS__);    \
            failures++;                                            \
        }                                                          \
    } while (0)

int main()
{
    // ----------------------------------------------------------------
    // 1. node_id_t must be 64-bit for the typed-ID scheme to work.
    //    A 32-bit node_id_t cannot represent a 56-bit shift without UB.
    // ----------------------------------------------------------------
    CHECK(sizeof(node_id_t) == 8,
          "node_id_t is %zu bytes — must be 8 (compile with -DB64=ON)",
          sizeof(node_id_t));

    if (sizeof(node_id_t) != 8) {
        fprintf(stderr, "Aborting remaining checks: 64-bit IDs required\n");
        return 1;
    }

    // ----------------------------------------------------------------
    // 2. MAKE_TYPED_ID encodes the type in the top 8 bits.
    // ----------------------------------------------------------------
    node_id_t p0  = MAKE_TYPED_ID(VT_PERSON, 0);
    node_id_t p1  = MAKE_TYPED_ID(VT_PERSON, 1);
    node_id_t po0 = MAKE_TYPED_ID(VT_POST,   0);
    node_id_t po1 = MAKE_TYPED_ID(VT_POST,   1);

    CHECK(VTYPE_OF(p0)  == VT_PERSON, "VT_PERSON ID 0: type bits wrong (got %llu)",
          (unsigned long long)VTYPE_OF(p0));
    CHECK(VTYPE_OF(p1)  == VT_PERSON, "VT_PERSON ID 1: type bits wrong (got %llu)",
          (unsigned long long)VTYPE_OF(p1));
    CHECK(VTYPE_OF(po0) == VT_POST,   "VT_POST   ID 0: type bits wrong (got %llu)",
          (unsigned long long)VTYPE_OF(po0));
    CHECK(VTYPE_OF(po1) == VT_POST,   "VT_POST   ID 1: type bits wrong (got %llu)",
          (unsigned long long)VTYPE_OF(po1));

    // ----------------------------------------------------------------
    // 3. Counter bits round-trip correctly.
    // ----------------------------------------------------------------
    CHECK(VCOUNTER_OF(p0)  == 0, "Person counter 0 lost (got %llu)", (unsigned long long)VCOUNTER_OF(p0));
    CHECK(VCOUNTER_OF(po0) == 0, "Post   counter 0 lost (got %llu)", (unsigned long long)VCOUNTER_OF(po0));
    CHECK(VCOUNTER_OF(p1)  == 1, "Person counter 1 lost (got %llu)", (unsigned long long)VCOUNTER_OF(p1));
    CHECK(VCOUNTER_OF(po1) == 1, "Post   counter 1 lost (got %llu)", (unsigned long long)VCOUNTER_OF(po1));

    // ----------------------------------------------------------------
    // 4. Person ID 0 and Post ID 0 must be distinct (overflow would
    //    make them identical on a 32-bit node_id_t).
    // ----------------------------------------------------------------
    CHECK(p0 != po0,
          "COLLISION: MAKE_TYPED_ID(VT_PERSON,0)==MAKE_TYPED_ID(VT_POST,0)=%llu "
          "— type bits were dropped (overflow?)",
          (unsigned long long)p0);

    // ----------------------------------------------------------------
    // 5. MAKE_EKEY (+1) must not carry into the type bits.
    //    The highest safe Person counter is (1<<56)-2 (leaves room for +1).
    // ----------------------------------------------------------------
    node_id_t max_person_counter = ((node_id_t)1 << 56) - 2;
    node_id_t large_person = MAKE_TYPED_ID(VT_PERSON, max_person_counter);
    node_id_t ekey_large   = MAKE_EKEY(large_person);

    CHECK(VTYPE_OF(ekey_large) == VT_PERSON,
          "MAKE_EKEY on near-max Person ID overflowed into type bits (got type=%llu)",
          (unsigned long long)VTYPE_OF(ekey_large));

    // ----------------------------------------------------------------
    // 6. MAKE_EKEY on a Post ID with counter 0 must still be a Post ID.
    // ----------------------------------------------------------------
    node_id_t ekey_post0 = MAKE_EKEY(po0);
    CHECK(VTYPE_OF(ekey_post0) == VT_POST,
          "MAKE_EKEY on Post ID 0 changed type to %llu",
          (unsigned long long)VTYPE_OF(ekey_post0));

    // ----------------------------------------------------------------
    // 7. OG_KEY (MAKE_EKEY inverse) round-trips both types.
    // ----------------------------------------------------------------
    CHECK(OG_KEY(MAKE_EKEY(p0))  == p0,  "OG_KEY(MAKE_EKEY(Person 0)) != Person 0");
    CHECK(OG_KEY(MAKE_EKEY(po0)) == po0, "OG_KEY(MAKE_EKEY(Post 0))   != Post 0");
    CHECK(OG_KEY(MAKE_EKEY(p1))  == p1,  "OG_KEY(MAKE_EKEY(Person 1)) != Person 1");
    CHECK(OG_KEY(MAKE_EKEY(po1)) == po1, "OG_KEY(MAKE_EKEY(Post 1))   != Post 1");

    // ----------------------------------------------------------------
    // Summary
    // ----------------------------------------------------------------
    if (failures == 0) {
        printf("OK: all typed-ID checks passed\n");
        return 0;
    } else {
        fprintf(stderr, "FAILED: %d check(s) above\n", failures);
        return 1;
    }
}
