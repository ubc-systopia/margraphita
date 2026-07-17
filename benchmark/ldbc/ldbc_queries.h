// ldbc_queries.h — All LDBC SNB query implementations for Flexograph
//
// Six sections:
//   A — Helpers (type tests, ID encoding, property decode)
//   B — JSON run_* functions (18 queries, used by --validate mode)
//   C — Timing query functions (adapted from ldbc_snb_queries.cpp)
//   D — NeighborCache (in-memory CSR topology cache)
//   E — OPT-1 cached variants (5 functions, topology from cache)
//   F — Parallel BI variants (3 functions, require <omp.h>)
//
// Included once by fg_ldbc_bench.cpp.
//
// Dependencies: common_defs.h, graph_engine.h, prop_schema.h,
//               adj_list.h, edgekey_split.h (for parallel variant constructors)

#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "adj_list.h"
#include "common_defs.h"
#include "edgekey_split.h"
#include "graph_engine.h"
#include "prop_schema.h"

// ============================================================
// Section A — Helpers
// ============================================================

static inline bool is_person (node_id_t id) { return VTYPE_OF(id) == VT_PERSON; }
static inline bool is_post   (node_id_t id) { return VTYPE_OF(id) == VT_POST; }
static inline bool is_comment(node_id_t id) { return VTYPE_OF(id) == VT_COMMENT; }
static inline bool is_forum  (node_id_t id) { return VTYPE_OF(id) == VT_FORUM; }
static inline bool is_message(node_id_t id) { return is_post(id) || is_comment(id); }

// Sentinel for "not found" used by ldbc_snb_queries-style functions.
static constexpr node_id_t ID_NOT_FOUND = (node_id_t)UINT64_MAX;

// ─── ID encoding ─────────────────────────────────────────────────────────────
// Encodes a typed vertex ID as a compact string ("p<N>", "q<N>", etc.)
// used in JSON output for --validate mode.

static std::string enc(node_id_t id)
{
    char buf[48];
    uint64_t c = VCOUNTER_OF(id);
    switch (VTYPE_OF(id)) {
        case VT_PERSON:  snprintf(buf, sizeof(buf), "p%llu", (unsigned long long)c); break;
        case VT_POST:    snprintf(buf, sizeof(buf), "q%llu", (unsigned long long)c); break;
        case VT_COMMENT: snprintf(buf, sizeof(buf), "c%llu", (unsigned long long)c); break;
        case VT_FORUM:   snprintf(buf, sizeof(buf), "f%llu", (unsigned long long)c); break;
        default:         snprintf(buf, sizeof(buf), "x%llu_t%u",
                                  (unsigned long long)c, (unsigned)VTYPE_OF(id)); break;
    }
    return buf;
}

// ─── JSON string escape ───────────────────────────────────────────────────────

static std::string jstr(const char *s)
{
    if (!s) return "null";
    std::string out = "\"";
    for (; *s; ++s) {
        switch (*s) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += *s;     break;
        }
    }
    out += '"';
    return out;
}
static std::string jstr(const std::string &s) { return jstr(s.c_str()); }
static std::string jenc(node_id_t id)          { return jstr(enc(id)); }

// ─── Property decode helpers ──────────────────────────────────────────────────

struct DecodedPerson {
    int64_t creationDate = 0, birthday = 0;
    int8_t  gender = 0;
    char firstName[SNBPersonSchema::STR_LEN] = {};
    char lastName [SNBPersonSchema::STR_LEN] = {};
    char browser  [SNBPersonSchema::STR_LEN] = {};
    char locIP    [SNBPersonSchema::STR_LEN] = {};
    bool valid = false;
};

static DecodedPerson decode_person_blob(const prop_blob &pb)
{
    DecodedPerson r;
    if (!pb.data || pb.size < SNBPersonSchema::TOTAL_SIZE) return r;
    r.creationDate = SNBPersonSchema::get_creation_date(pb.data);
    r.birthday     = SNBPersonSchema::get_birthday(pb.data);
    r.gender       = SNBPersonSchema::get_gender(pb.data);
    std::memcpy(r.firstName, SNBPersonSchema::get_first_name(pb.data),   SNBPersonSchema::STR_LEN);
    std::memcpy(r.lastName,  SNBPersonSchema::get_last_name(pb.data),    SNBPersonSchema::STR_LEN);
    std::memcpy(r.browser,   SNBPersonSchema::get_browser_used(pb.data), SNBPersonSchema::STR_LEN);
    std::memcpy(r.locIP,     SNBPersonSchema::get_location_ip(pb.data),  SNBPersonSchema::STR_LEN);
    r.valid = true;
    return r;
}

struct DecodedPost {
    int64_t     creationDate = 0;
    int32_t     length = 0;
    int8_t      tag = 0;
    std::string content;
    bool valid = false;
};

static DecodedPost decode_post_blob(const prop_blob &pb)
{
    DecodedPost r;
    if (!pb.data || pb.size < SNBPostSchema::TOTAL_SIZE) return r;
    r.creationDate = SNBPostSchema::get_creation_date(pb.data);
    r.length       = SNBPostSchema::get_length(pb.data);
    r.tag          = SNBPostSchema::get_tag(pb.data);
    r.content      = SNBPostSchema::get_content(pb.data, pb.size);
    r.valid = true;
    return r;
}

// Resolve a Person's country: Person→City (isLocatedIn)→Country (isPartOf).
// Returns Country node_id or OutOfBand_ID_MAX if not found.
static node_id_t resolve_person_country_graph(GraphBase &g, node_id_t person_id)
{
    for (node_id_t city : g.get_out_nodes_id(person_id)) {
        if (VTYPE_OF(city) == VT_COUNTRY) return city;
        if (VTYPE_OF(city) != VT_CITY) continue;
        for (node_id_t country : g.get_out_nodes_id(city))
            if (VTYPE_OF(country) == VT_COUNTRY) return country;
    }
    return OutOfBand_ID_MAX;
}

// ============================================================
// Section B — JSON run_* functions (18 queries)
// Verbatim from test/validate_queries.cpp (with ic3 forward-only FoF fix applied).
// Output JSON lines to stdout; timing/debug to stderr.
// ============================================================

// R1: Person profile (SR-1)
// Returns: firstName, lastName, gender, birthday, creationDate, locationIP,
//          browserUsed, cityId (via isLocatedIn→City), emails[], speaks[].
// cityId: walk outgoing neighbors of pid and pick the first VT_CITY node.
// emails/speaks: populated only in COLUMNAR mode (empty in EMBEDDED).
static void run_r1(GraphBase &g, node_id_t pid)
{
    prop_blob pb = g.get_node_properties(pid);
    DecodedPerson p = decode_person_blob(pb);
    auto emails = g.get_person_emails(pid);
    auto langs  = g.get_person_languages(pid);

    // cityId: first VT_CITY in outgoing neighbors (isLocatedIn edge)
    node_id_t city_id = OutOfBand_ID_MAX;
    for (node_id_t nb : g.get_out_nodes_id(pid))
        if (VTYPE_OF(nb) == VT_CITY) { city_id = nb; break; }

    printf("{\"query\":\"r1\",\"pid\":%llu", (unsigned long long)VCOUNTER_OF(pid));
    if (!p.valid) { printf(",\"error\":\"no_props\"}\n"); return; }
    printf(",\"firstName\":%s",      jstr(p.firstName).c_str());
    printf(",\"lastName\":%s",       jstr(p.lastName).c_str());
    printf(",\"gender\":%d",         (int)p.gender);
    printf(",\"birthday\":%lld",     (long long)p.birthday);
    printf(",\"creationDate\":%lld", (long long)p.creationDate);
    printf(",\"locationIP\":%s",     jstr(p.locIP).c_str());
    printf(",\"browserUsed\":%s",    jstr(p.browser).c_str());
    printf(",\"cityId\":%s",         jenc(city_id).c_str());
    printf(",\"emails\":[");
    for (size_t i = 0; i < emails.size(); i++) {
        if (i) printf(",");
        printf("%s", jstr(emails[i]).c_str());
    }
    printf("],\"languages\":[");
    for (size_t i = 0; i < langs.size(); i++) {
        if (i) printf(",");
        printf("%s", jstr(langs[i]).c_str());
    }
    printf("]}\n");
}

// R2: Friends sorted by creationDate DESC
static void run_r2(GraphBase &g, node_id_t pid, bool has_props)
{
    std::vector<std::pair<node_id_t, int64_t>> result;

    if (has_props) {
        auto cur = g.get_edge_prop_cursor(KNOWS_PROPS_TABLE, CG_TEMPORAL);
        cur->set_src(pid);
        while (cur->next())
            result.emplace_back(cur->dst(), (int64_t)cur->get_uint64(0));
        for (node_id_t nb : g.get_in_nodes_id(pid)) {
            if (!is_person(nb)) continue;
            prop_blob pb = g.get_edge_properties(nb, pid);
            int64_t cDate = (pb.data && pb.size >= SNBKnowsSchema::TOTAL_SIZE)
                            ? SNBKnowsSchema::get_creation_date(pb.data) : -1LL;
            result.emplace_back(nb, cDate);
        }
    } else {
        for (node_id_t nb : g.get_out_nodes_id(pid)) {
            if (!is_person(nb)) continue;
            prop_blob pb = g.get_edge_properties(pid, nb);
            int64_t cDate = (pb.data && pb.size >= SNBKnowsSchema::TOTAL_SIZE)
                            ? SNBKnowsSchema::get_creation_date(pb.data) : -1LL;
            result.emplace_back(nb, cDate);
        }
        for (node_id_t nb : g.get_in_nodes_id(pid)) {
            if (!is_person(nb)) continue;
            prop_blob pb = g.get_edge_properties(nb, pid);
            int64_t cDate = (pb.data && pb.size >= SNBKnowsSchema::TOTAL_SIZE)
                            ? SNBKnowsSchema::get_creation_date(pb.data) : -1LL;
            result.emplace_back(nb, cDate);
        }
    }

    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b){ return a.second > b.second; });

    printf("{\"query\":\"r2\",\"pid\":%llu,\"friends\":[",
           (unsigned long long)VCOUNTER_OF(pid));
    for (size_t i = 0; i < result.size(); i++) {
        if (i) printf(",");
        printf("{\"fid\":%s,\"creationDate\":%lld}",
               jenc(result[i].first).c_str(), (long long)result[i].second);
    }
    printf("]}\n");
}

// R3: BFS shortest path (undirected knows traversal)
static void run_r3(GraphBase &g, node_id_t src, node_id_t dst)
{
    int hops = -1;
    if (src == dst) {
        hops = 0;
    } else {
        std::unordered_map<node_id_t,int> dist;
        std::deque<node_id_t> q;
        dist[src] = 0;
        q.push_back(src);
        while (!q.empty() && hops < 0) {
            node_id_t u = q.front(); q.pop_front();
            std::vector<node_id_t> nbrs = g.get_out_nodes_id(u);
            for (node_id_t v : g.get_in_nodes_id(u))
                if (is_person(v)) nbrs.push_back(v);
            for (node_id_t v : nbrs) {
                if (!is_person(v) || dist.count(v)) continue;
                dist[v] = dist[u] + 1;
                if (v == dst) { hops = dist[v]; break; }
                q.push_back(v);
            }
        }
    }
    printf("{\"query\":\"r3\",\"src\":%llu,\"dst\":%llu,\"hops\":%d}\n",
           (unsigned long long)VCOUNTER_OF(src),
           (unsigned long long)VCOUNTER_OF(dst), hops);
}

// X1: Post profile
static void run_x1(GraphBase &g, node_id_t post_id)
{
    prop_blob pb = g.get_node_properties(post_id);
    DecodedPost p = decode_post_blob(pb);
    printf("{\"query\":\"x1\",\"post\":%llu", (unsigned long long)VCOUNTER_OF(post_id));
    if (!p.valid) { printf(",\"error\":\"no_props\"}\n"); return; }
    printf(",\"creationDate\":%lld", (long long)p.creationDate);
    printf(",\"length\":%d",         p.length);
    printf(",\"tag\":%d",            (int)p.tag);
    printf(",\"content\":%s",        jstr(p.content).c_str());
    printf("}\n");
}

// X2: Post → author
static void run_x2(GraphBase &g, node_id_t post_id)
{
    node_id_t author = OutOfBand_ID_MAX;
    for (node_id_t nb : g.get_out_nodes_id(post_id))
        if (is_person(nb)) { author = nb; break; }
    if (author == OutOfBand_ID_MAX)
        printf("{\"query\":\"x2\",\"post\":%llu,\"author\":null}\n",
               (unsigned long long)VCOUNTER_OF(post_id));
    else
        printf("{\"query\":\"x2\",\"post\":%llu,\"author\":%s}\n",
               (unsigned long long)VCOUNTER_OF(post_id), jenc(author).c_str());
}

// X3: IC-2 — friends' posts before cutoff, top-10 DESC
static void run_x3(GraphBase &g, node_id_t pid, int64_t cutoff_ms, bool has_props)
{
    std::vector<node_id_t> friends;
    for (node_id_t nb : g.get_out_nodes_id(pid))
        if (is_person(nb)) friends.push_back(nb);
    for (node_id_t nb : g.get_in_nodes_id(pid))
        if (is_person(nb)) friends.push_back(nb);

    std::vector<std::pair<int64_t, node_id_t>> cands;

    if (has_props) {
        std::vector<node_id_t> posts;
        for (node_id_t f : friends)
            for (node_id_t nb : g.get_in_nodes_id(f))
                if (is_post(nb)) posts.push_back(nb);
        std::sort(posts.begin(), posts.end());
        posts.erase(std::unique(posts.begin(), posts.end()), posts.end());

        auto cur = g.get_edge_prop_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
        for (node_id_t p2 : posts) {
            cur->set_src(p2);
            if (cur->next()) {
                int64_t cDate = (int64_t)cur->get_uint64(0);
                if (cDate < cutoff_ms) cands.emplace_back(cDate, p2);
            }
        }
    } else {
        for (node_id_t f : friends) {
            for (node_id_t nb : g.get_in_nodes_id(f)) {
                if (!is_post(nb)) continue;
                prop_blob pb = g.get_node_properties(nb);
                DecodedPost p2 = decode_post_blob(pb);
                if (p2.valid && p2.creationDate < cutoff_ms)
                    cands.emplace_back(p2.creationDate, nb);
            }
        }
    }

    std::sort(cands.begin(), cands.end(),
              [](const auto &a, const auto &b){ return a.first > b.first; });
    if (cands.size() > 10) cands.resize(10);

    printf("{\"query\":\"x3\",\"pid\":%llu,\"posts\":[",
           (unsigned long long)VCOUNTER_OF(pid));
    for (size_t i = 0; i < cands.size(); i++) {
        if (i) printf(",");
        printf("{\"pid\":%s,\"creationDate\":%lld}",
               jenc(cands[i].second).c_str(), (long long)cands[i].first);
    }
    printf("]}\n");
}

// X5: Count likes on a post in date range [lo_ms, hi_ms]
static void run_x5(GraphBase &g, node_id_t post_id, int64_t lo_ms, int64_t hi_ms)
{
    int64_t count = 0;
    for (node_id_t liker : g.get_in_nodes_id(post_id)) {
        if (!is_person(liker)) continue;
        prop_blob pb = g.get_edge_properties(liker, post_id);
        if (pb.data && pb.size >= SNBLikesSchema::TOTAL_SIZE) {
            int64_t cDate = SNBLikesSchema::get_creation_date(pb.data);
            if (cDate >= lo_ms && cDate <= hi_ms) count++;
        }
    }
    printf("{\"query\":\"x5\",\"post\":%llu,\"count\":%lld}\n",
           (unsigned long long)VCOUNTER_OF(post_id), (long long)count);
}

// A1: Out-degree and in-degree
static void run_a1(GraphBase &g, node_id_t pid)
{
    degree_t out = g.get_out_degree(pid);
    degree_t in  = g.get_in_degree(pid);
    printf("{\"query\":\"a1\",\"pid\":%llu,\"out_degree\":%u,\"in_degree\":%u}\n",
           (unsigned long long)VCOUNTER_OF(pid), out, in);
}

// A2: Count knows edges in date range
static void run_a2(GraphBase &g, node_id_t pid, int64_t lo_ms, int64_t hi_ms, bool has_props)
{
    int64_t count = 0;
    if (has_props) {
        auto cur = g.get_edge_prop_cursor(KNOWS_PROPS_TABLE, CG_TEMPORAL);
        cur->set_src(pid);
        while (cur->next()) {
            int64_t cDate = (int64_t)cur->get_uint64(0);
            if (cDate >= lo_ms && cDate <= hi_ms) count++;
        }
        for (node_id_t nb : g.get_in_nodes_id(pid)) {
            if (!is_person(nb)) continue;
            prop_blob pb = g.get_edge_properties(nb, pid);
            if (pb.data && pb.size >= SNBKnowsSchema::TOTAL_SIZE) {
                int64_t cDate = SNBKnowsSchema::get_creation_date(pb.data);
                if (cDate >= lo_ms && cDate <= hi_ms) count++;
            }
        }
    } else {
        for (node_id_t nb : g.get_out_nodes_id(pid)) {
            if (!is_person(nb)) continue;
            prop_blob pb = g.get_edge_properties(pid, nb);
            if (pb.data && pb.size >= SNBKnowsSchema::TOTAL_SIZE) {
                int64_t cDate = SNBKnowsSchema::get_creation_date(pb.data);
                if (cDate >= lo_ms && cDate <= hi_ms) count++;
            }
        }
        for (node_id_t nb : g.get_in_nodes_id(pid)) {
            if (!is_person(nb)) continue;
            prop_blob pb = g.get_edge_properties(nb, pid);
            if (pb.data && pb.size >= SNBKnowsSchema::TOTAL_SIZE) {
                int64_t cDate = SNBKnowsSchema::get_creation_date(pb.data);
                if (cDate >= lo_ms && cDate <= hi_ms) count++;
            }
        }
    }
    printf("{\"query\":\"a2\",\"pid\":%llu,\"count\":%lld}\n",
           (unsigned long long)VCOUNTER_OF(pid), (long long)count);
}

// A3: Count posts liked in date range
static void run_a3(GraphBase &g, node_id_t pid, int64_t lo_ms, int64_t hi_ms, bool has_props)
{
    int64_t count = 0;
    if (has_props) {
        auto cur = g.get_edge_prop_cursor(LIKES_PROPS_TABLE, CG_TEMPORAL);
        cur->set_src(pid);
        while (cur->next()) {
            if (!is_post(cur->dst())) continue;
            int64_t cDate = (int64_t)cur->get_uint64(0);
            if (cDate >= lo_ms && cDate <= hi_ms) count++;
        }
    } else {
        for (node_id_t post_id : g.get_out_nodes_id(pid)) {
            if (!is_post(post_id)) continue;
            prop_blob pb = g.get_edge_properties(pid, post_id);
            if (pb.data && pb.size >= SNBLikesSchema::TOTAL_SIZE) {
                int64_t cDate = SNBLikesSchema::get_creation_date(pb.data);
                if (cDate >= lo_ms && cDate <= hi_ms) count++;
            }
        }
    }
    printf("{\"query\":\"a3\",\"pid\":%llu,\"count\":%lld}\n",
           (unsigned long long)VCOUNTER_OF(pid), (long long)count);
}

// BI-1: Posting summary — (year, length_cat) → (count, sum_len)
static void run_bi1(GraphBase &g, bool has_props)
{
    std::map<std::pair<int,int>, std::pair<int64_t,int64_t>> groups;

    if (has_props) {
        auto cur = g.get_edge_prop_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
        cur->set_src(MAKE_TYPED_ID(VT_POST, 0));
        while (cur->next()) {
            node_id_t pid2 = cur->src();
            if (VTYPE_OF(pid2) != VT_POST) break;
            int64_t cDate  = (int64_t)cur->get_uint64(0);
            int32_t length = (int32_t)cur->get_uint64(1);
            int year = (int)(cDate / 31557600000LL) + 1970;
            int cat  = length < 40 ? 0 : length < 80 ? 1 : length < 255 ? 2 : 3;
            auto &g2 = groups[{year,cat}];
            g2.first++;
            g2.second += length;
        }
    } else {
        for (uint64_t c = 0; ; c++) {
            node_id_t nid = MAKE_TYPED_ID(VT_POST, c);
            prop_blob pb  = g.get_node_properties(nid);
            if (!pb.data || pb.size < SNBPostSchema::TOTAL_SIZE) break;
            int64_t cDate  = SNBPostSchema::get_creation_date(pb.data);
            int32_t length = SNBPostSchema::get_length(pb.data);
            int year = (int)(cDate / 31557600000LL) + 1970;
            int cat  = length < 40 ? 0 : length < 80 ? 1 : length < 255 ? 2 : 3;
            auto &g3 = groups[{year,cat}];
            g3.first++;
            g3.second += length;
        }
    }

    printf("{\"query\":\"bi1\",\"rows\":[");
    bool first = true;
    for (auto &kv : groups) {
        if (!first) printf(",");
        printf("{\"year\":%d,\"cat\":%d,\"count\":%lld,\"sum_len\":%lld}",
               kv.first.first, kv.first.second,
               (long long)kv.second.first, (long long)kv.second.second);
        first = false;
    }
    printf("]}\n");
}

// BI-12: Top-10 post creators by post count
static void run_bi12(GraphBase &g)
{
    std::unordered_map<node_id_t,int64_t> cnt;
    for (uint64_t c = 0; ; c++) {
        node_id_t nid = MAKE_TYPED_ID(VT_POST, c);
        prop_blob pb  = g.get_node_properties(nid);
        if (!pb.data || pb.size < SNBPostSchema::TOTAL_SIZE) break;
        for (node_id_t nb : g.get_out_nodes_id(nid))
            if (is_person(nb)) { cnt[nb]++; break; }
    }
    std::vector<std::pair<int64_t,node_id_t>> rows;
    for (auto &kv : cnt) rows.emplace_back(kv.second, kv.first);
    std::sort(rows.begin(), rows.end(), [](const auto &a, const auto &b){
        if (a.first != b.first) return a.first > b.first;
        return a.second < b.second;
    });
    if (rows.size() > 10) rows.resize(10);
    printf("{\"query\":\"bi12\",\"rows\":[");
    for (size_t i = 0; i < rows.size(); i++) {
        if (i) printf(",");
        printf("{\"pid\":%s,\"count\":%lld}",
               jenc(rows[i].second).c_str(), (long long)rows[i].first);
    }
    printf("]}\n");
}

// BI-2: Message count in two date windows
static void run_bi2(GraphBase &g, bool has_props)
{
    const int64_t W1_LO = 1325376000000LL, W1_HI = 1341100800000LL;
    const int64_t W2_LO = 1356998400000LL, W2_HI = 1372636800000LL;
    int64_t pw1=0, pw2=0, cw1=0, cw2=0;

    auto count_vttype = [&](VertexType vt, const std::string &table, int64_t &w1, int64_t &w2)
    {
        if (has_props) {
            auto cur = g.get_edge_prop_cursor(table, CG_TEMPORAL);
            cur->set_src(MAKE_TYPED_ID(vt, 0));
            while (cur->next()) {
                if (VTYPE_OF(cur->src()) != vt) break;
                int64_t cDate = (int64_t)cur->get_uint64(0);
                if (cDate >= W1_LO && cDate < W1_HI) w1++;
                if (cDate >= W2_LO && cDate < W2_HI) w2++;
            }
        } else {
            for (uint64_t c = 0; ; c++) {
                node_id_t nid = MAKE_TYPED_ID(vt, c);
                prop_blob pb  = g.get_node_properties(nid);
                if (!pb.data) break;
                int64_t cDate = -1;
                if (vt == VT_POST && pb.size >= SNBPostSchema::TOTAL_SIZE)
                    cDate = SNBPostSchema::get_creation_date(pb.data);
                else if (pb.size >= 8)
                    cDate = *reinterpret_cast<const int64_t*>(pb.data);
                if (cDate >= W1_LO && cDate < W1_HI) w1++;
                if (cDate >= W2_LO && cDate < W2_HI) w2++;
            }
        }
    };

    count_vttype(VT_POST,    POST_PROPS_TABLE,    pw1, pw2);
    count_vttype(VT_COMMENT, COMMENT_PROPS_TABLE, cw1, cw2);

    printf("{\"query\":\"bi2\",\"posts_w1\":%lld,\"posts_w2\":%lld,"
           "\"comments_w1\":%lld,\"comments_w2\":%lld}\n",
           (long long)pw1, (long long)pw2,
           (long long)cw1, (long long)cw2);
}

// IC-7: Likers of person P's messages, top-20 DESC by like date
static void run_ic7(GraphBase &g, node_id_t pid, bool /*has_props*/)
{
    std::vector<node_id_t> msgs;
    for (node_id_t nb : g.get_in_nodes_id(pid))
        if (is_message(nb)) msgs.push_back(nb);

    struct Row { node_id_t liker, msg; int64_t cDate; };
    std::vector<Row> rows;

    for (node_id_t msg : msgs) {
        for (node_id_t liker : g.get_in_nodes_id(msg)) {
            if (!is_person(liker)) continue;
            int64_t cDate = -1;
            prop_blob pb = g.get_edge_properties(liker, msg);
            if (pb.data && pb.size >= SNBLikesSchema::TOTAL_SIZE)
                cDate = SNBLikesSchema::get_creation_date(pb.data);
            rows.push_back({liker, msg, cDate});
        }
    }

    std::sort(rows.begin(), rows.end(),
              [](const Row &a, const Row &b){ return a.cDate > b.cDate; });
    if (rows.size() > 20) rows.resize(20);

    printf("{\"query\":\"ic7\",\"pid\":%llu,\"rows\":[",
           (unsigned long long)VCOUNTER_OF(pid));
    for (size_t i = 0; i < rows.size(); i++) {
        if (i) printf(",");
        const char *mtype = is_post(rows[i].msg) ? "post" : "comment";
        printf("{\"liker\":%s,\"msg\":%s,\"msg_type\":\"%s\",\"creationDate\":%lld}",
               jenc(rows[i].liker).c_str(), jenc(rows[i].msg).c_str(),
               mtype, (long long)rows[i].cDate);
    }
    printf("]}\n");
}

// IC-8: Latest replies to P's messages, top-20 DESC by reply date
static void run_ic8(GraphBase &g, node_id_t pid, bool /*has_props*/)
{
    struct Row { node_id_t reply, creator; int64_t cDate; };
    std::vector<Row> rows;

    for (node_id_t msg : g.get_in_nodes_id(pid)) {
        if (!is_message(msg)) continue;
        for (node_id_t reply : g.get_in_nodes_id(msg)) {
            if (!is_comment(reply)) continue;
            int64_t cDate = -1;
            prop_blob pb = g.get_node_properties(reply);
            if (pb.data && pb.size >= 8)
                cDate = *reinterpret_cast<const int64_t*>(pb.data);
            node_id_t creator = OutOfBand_ID_MAX;
            for (node_id_t nb : g.get_out_nodes_id(reply))
                if (is_person(nb)) { creator = nb; break; }
            rows.push_back({reply, creator, cDate});
        }
    }

    std::sort(rows.begin(), rows.end(),
              [](const Row &a, const Row &b){ return a.cDate > b.cDate; });
    if (rows.size() > 20) rows.resize(20);

    printf("{\"query\":\"ic8\",\"pid\":%llu,\"rows\":[",
           (unsigned long long)VCOUNTER_OF(pid));
    for (size_t i = 0; i < rows.size(); i++) {
        if (i) printf(",");
        printf("{\"reply\":%s,\"creationDate\":%lld,\"creator\":%s}",
               jenc(rows[i].reply).c_str(), (long long)rows[i].cDate,
               rows[i].creator == OutOfBand_ID_MAX ? "null" : jenc(rows[i].creator).c_str());
    }
    printf("]}\n");
}

// IC-9: Friends' messages before cutoff, top-20 DESC by message date
static void run_ic9(GraphBase &g, node_id_t pid, int64_t cutoff_ms, bool /*has_props*/)
{
    std::vector<node_id_t> friends;
    for (node_id_t nb : g.get_out_nodes_id(pid))
        if (is_person(nb)) friends.push_back(nb);
    for (node_id_t nb : g.get_in_nodes_id(pid))
        if (is_person(nb)) friends.push_back(nb);

    struct Row { node_id_t msg, creator; int64_t cDate; };
    std::vector<Row> rows;

    for (node_id_t f : friends) {
        for (node_id_t msg : g.get_in_nodes_id(f)) {
            if (!is_message(msg)) continue;
            int64_t cDate = -1;
            prop_blob pb = g.get_node_properties(msg);
            if (pb.data && pb.size >= 8)
                cDate = *reinterpret_cast<const int64_t*>(pb.data);
            if (cDate < cutoff_ms)
                rows.push_back({msg, f, cDate});
        }
    }

    std::sort(rows.begin(), rows.end(),
              [](const Row &a, const Row &b){ return a.cDate > b.cDate; });
    if (rows.size() > 20) rows.resize(20);

    printf("{\"query\":\"ic9\",\"pid\":%llu,\"rows\":[",
           (unsigned long long)VCOUNTER_OF(pid));
    for (size_t i = 0; i < rows.size(); i++) {
        if (i) printf(",");
        const char *mtype = is_post(rows[i].msg) ? "post" : "comment";
        printf("{\"msg\":%s,\"msg_type\":\"%s\",\"creationDate\":%lld,\"creator\":%s}",
               jenc(rows[i].msg).c_str(), mtype, (long long)rows[i].cDate,
               jenc(rows[i].creator).c_str());
    }
    printf("]}\n");
}

// IC-5: Forums where friends joined since date, top-20 DESC by member_count
static void run_ic5(GraphBase &g, node_id_t pid, int64_t since_ms, bool /*has_props*/)
{
    std::vector<node_id_t> friends;
    for (node_id_t nb : g.get_out_nodes_id(pid))
        if (is_person(nb)) friends.push_back(nb);
    for (node_id_t nb : g.get_in_nodes_id(pid))
        if (is_person(nb)) friends.push_back(nb);

    std::unordered_map<node_id_t,int64_t> forum_cnt;

    for (node_id_t f : friends) {
        for (node_id_t nb : g.get_in_nodes_id(f)) {
            if (!is_forum(nb)) continue;
            int64_t joinDate = -1;
            prop_blob pb = g.get_edge_properties(nb, f);
            if (pb.data && pb.size >= 8)
                joinDate = *reinterpret_cast<const int64_t*>(pb.data);
            if (joinDate >= since_ms)
                forum_cnt[nb]++;
        }
    }

    std::vector<std::pair<int64_t,node_id_t>> rows;
    for (auto &kv : forum_cnt) rows.emplace_back(kv.second, kv.first);
    std::sort(rows.begin(), rows.end(), [](const auto &a, const auto &b){
        if (a.first != b.first) return a.first > b.first;
        return a.second < b.second;
    });
    if (rows.size() > 20) rows.resize(20);

    printf("{\"query\":\"ic5\",\"pid\":%llu,\"rows\":[",
           (unsigned long long)VCOUNTER_OF(pid));
    for (size_t i = 0; i < rows.size(); i++) {
        if (i) printf(",");
        printf("{\"forum\":%s,\"member_count\":%lld}",
               jenc(rows[i].second).c_str(), (long long)rows[i].first);
    }
    printf("]}\n");
}

// IC-3: FoF of P in country_x but not country_y, top-10 DESC by common friends
// FoF expansion uses forward direction only (friend)-[:knows]->(fof) to match NeuG Cypher.
static void run_ic3(GraphBase &g, node_id_t pid,
                    node_id_t country_x_id, node_id_t country_y_id)
{
    std::unordered_set<node_id_t> direct;
    for (node_id_t nb : g.get_out_nodes_id(pid))
        if (is_person(nb)) direct.insert(nb);
    for (node_id_t nb : g.get_in_nodes_id(pid))
        if (is_person(nb)) direct.insert(nb);

    // Forward direction only — matches NeuG Cypher (friend)-[:knows]->(fof)
    std::unordered_map<node_id_t,int64_t> common_cnt;
    for (node_id_t f : direct) {
        for (node_id_t fof : g.get_out_nodes_id(f)) {
            if (!is_person(fof)) continue;
            if (fof == pid || direct.count(fof)) continue;
            common_cnt[fof]++;
        }
    }

    struct Row { node_id_t fof; int64_t common; };
    std::vector<Row> rows;
    for (auto &kv : common_cnt) {
        node_id_t fof = kv.first;
        node_id_t country = resolve_person_country_graph(g, fof);
        if (country == OutOfBand_ID_MAX) continue;
        if (country_x_id != OutOfBand_ID_MAX && country != country_x_id) continue;
        if (country_y_id != OutOfBand_ID_MAX && country == country_y_id) continue;
        rows.push_back({fof, kv.second});
    }

    std::sort(rows.begin(), rows.end(), [](const Row &a, const Row &b){
        if (a.common != b.common) return a.common > b.common;
        return a.fof < b.fof;
    });
    if (rows.size() > 10) rows.resize(10);

    printf("{\"query\":\"ic3\",\"pid\":%llu,\"country_x\":%s,\"rows\":[",
           (unsigned long long)VCOUNTER_OF(pid), jenc(country_x_id).c_str());
    for (size_t i = 0; i < rows.size(); i++) {
        if (i) printf(",");
        printf("{\"fof\":%s,\"common\":%lld}",
               jenc(rows[i].fof).c_str(), (long long)rows[i].common);
    }
    printf("]}\n");
}

// ============================================================
// Section C — Timing query functions
// Adapted from test/ldbc_snb_queries.cpp.
// Each function takes double *out_ms = nullptr for elapsed time capture.
// Output: debug info to stderr; no JSON to stdout.
// ============================================================

using Clock_t = std::chrono::steady_clock;
using Ms_t    = std::chrono::duration<double, std::milli>;

#define TQ_START(label)   auto _tq0_##label = Clock_t::now(); \
    fprintf(stderr, "[QUERY] running %-40s ... ", #label);
#define TQ_END(label)     { double _ms = Ms_t(Clock_t::now() - _tq0_##label).count(); \
    fprintf(stderr, "%.3f ms\n", _ms); }
#define TQ_END_CAP(label, ms_out) { (ms_out) = Ms_t(Clock_t::now() - _tq0_##label).count(); \
    fprintf(stderr, "%.3f ms\n", (ms_out)); }

static void tq_r1_person_profile(GraphBase &g, node_id_t pid, bool has_props,
                                  double *out_ms = nullptr)
{
    TQ_START(r1_person_profile)
    prop_blob pb = g.get_node_properties(pid);
    DecodedPerson p = decode_person_blob(pb);
    (void)p;
    if (out_ms) { TQ_END_CAP(r1_person_profile, *out_ms) }
    else        { TQ_END(r1_person_profile) }
}

static void tq_r2_friends_sorted_by_date(GraphBase &g, node_id_t pid,
                                          bool has_props, double *out_ms = nullptr)
{
    TQ_START(r2_friends_sorted_by_date)
    std::vector<std::pair<node_id_t, int64_t>> result;
    if (has_props) {
        auto cur = g.get_edge_prop_cursor(KNOWS_PROPS_TABLE, CG_TEMPORAL);
        cur->set_src(pid);
        while (cur->next())
            result.emplace_back(cur->dst(), (int64_t)cur->get_uint64(0));
    } else {
        for (node_id_t nb : g.get_out_nodes_id(pid)) {
            if (!is_person(nb)) continue;
            prop_blob pb = g.get_edge_properties(pid, nb);
            int64_t cDate = (pb.data && pb.size >= SNBKnowsSchema::TOTAL_SIZE)
                            ? SNBKnowsSchema::get_creation_date(pb.data) : 0LL;
            result.emplace_back(nb, cDate);
        }
    }
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b){ return a.second > b.second; });
    if (out_ms) { TQ_END_CAP(r2_friends_sorted_by_date, *out_ms) }
    else        { TQ_END(r2_friends_sorted_by_date) }
}

static void tq_r3_bfs_shortest_path(GraphBase &g, node_id_t src, node_id_t dst,
                                     double *out_ms = nullptr)
{
    TQ_START(r3_bfs_shortest_path)
    if (src != dst) {
        std::unordered_map<node_id_t, int> dist;
        std::deque<node_id_t> q;
        dist[src] = 0;
        q.push_back(src);
        int found = -1;
        while (!q.empty() && found < 0) {
            node_id_t u = q.front(); q.pop_front();
            std::vector<node_id_t> nbrs = g.get_out_nodes_id(u);
            for (node_id_t v : g.get_in_nodes_id(u))
                if (is_person(v)) nbrs.push_back(v);
            for (node_id_t v : nbrs) {
                if (!is_person(v) || dist.count(v)) continue;
                dist[v] = dist[u] + 1;
                if (v == dst) { found = dist[v]; break; }
                q.push_back(v);
            }
        }
    }
    if (out_ms) { TQ_END_CAP(r3_bfs_shortest_path, *out_ms) }
    else        { TQ_END(r3_bfs_shortest_path) }
}

static void tq_x1_post_profile(GraphBase &g, node_id_t post_id, double *out_ms = nullptr)
{
    TQ_START(x1_post_profile)
    prop_blob pb = g.get_node_properties(post_id);
    DecodedPost p = decode_post_blob(pb);
    (void)p;
    if (out_ms) { TQ_END_CAP(x1_post_profile, *out_ms) }
    else        { TQ_END(x1_post_profile) }
}

static void tq_x2_post_author(GraphBase &g, node_id_t post_id, double *out_ms = nullptr)
{
    TQ_START(x2_post_author)
    node_id_t author = OutOfBand_ID_MAX;
    for (node_id_t nb : g.get_out_nodes_id(post_id))
        if (is_person(nb)) { author = nb; break; }
    (void)author;
    if (out_ms) { TQ_END_CAP(x2_post_author, *out_ms) }
    else        { TQ_END(x2_post_author) }
}

static void tq_x3_ic2_friends_recent_posts(GraphBase &g, node_id_t pid,
                                            int64_t cutoff_ms, bool has_props,
                                            double *out_ms = nullptr)
{
    TQ_START(x3_ic2_friends_recent_posts)
    std::vector<node_id_t> friends = g.get_out_nodes_id(pid);
    std::vector<node_id_t> candidate_posts;
    for (node_id_t f : friends) {
        if (!is_person(f)) continue;
        for (node_id_t p2 : g.get_in_nodes_id(f))
            if (is_post(p2)) candidate_posts.push_back(p2);
    }
    std::sort(candidate_posts.begin(), candidate_posts.end());
    candidate_posts.erase(std::unique(candidate_posts.begin(), candidate_posts.end()),
                          candidate_posts.end());
    std::vector<std::pair<node_id_t, int64_t>> result;
    if (has_props && !candidate_posts.empty()) {
        auto cg = g.get_node_prop_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
        for (node_id_t p2 : candidate_posts) {
            if (!cg->seek(p2)) continue;
            uint64_t cDate = cg->get_uint64(0);
            if ((int64_t)cDate < cutoff_ms) result.emplace_back(p2, (int64_t)cDate);
        }
    } else {
        for (node_id_t p2 : candidate_posts) {
            prop_blob pb = g.get_node_properties(p2);
            DecodedPost dp = decode_post_blob(pb);
            if (dp.valid && dp.creationDate < cutoff_ms)
                result.emplace_back(p2, dp.creationDate);
        }
    }
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b){ return a.second > b.second; });
    if (result.size() > 10) result.resize(10);
    if (out_ms) { TQ_END_CAP(x3_ic2_friends_recent_posts, *out_ms) }
    else        { TQ_END(x3_ic2_friends_recent_posts) }
}

static void tq_x5_count_likes_in_range(GraphBase &g, node_id_t post_id,
                                        int64_t lo_ms, int64_t hi_ms,
                                        double *out_ms = nullptr)
{
    TQ_START(x5_count_likes_in_range)
    int64_t count = 0;
    for (node_id_t liker : g.get_in_nodes_id(post_id)) {
        if (!is_person(liker)) continue;
        prop_blob pb = g.get_edge_properties(liker, post_id);
        int64_t cd = (pb.data && pb.size >= SNBLikesSchema::TOTAL_SIZE)
                     ? SNBLikesSchema::get_creation_date(pb.data) : -1LL;
        if (cd >= lo_ms && cd <= hi_ms) count++;
    }
    (void)count;
    if (out_ms) { TQ_END_CAP(x5_count_likes_in_range, *out_ms) }
    else        { TQ_END(x5_count_likes_in_range) }
}

static void tq_a1_degree_count(GraphBase &g, node_id_t id, double *out_ms = nullptr)
{
    TQ_START(a1_degree_count)
    degree_t out = g.get_out_degree(id);
    degree_t in  = g.get_in_degree(id);
    (void)out; (void)in;
    if (out_ms) { TQ_END_CAP(a1_degree_count, *out_ms) }
    else        { TQ_END(a1_degree_count) }
}

static void tq_a2_knows_in_date_range(GraphBase &g, node_id_t pid,
                                       int64_t lo_ms, int64_t hi_ms,
                                       bool has_props, double *out_ms = nullptr)
{
    TQ_START(a2_knows_in_date_range)
    int64_t count = 0;
    if (has_props) {
        auto cur = g.get_edge_prop_cursor(KNOWS_PROPS_TABLE, CG_TEMPORAL);
        cur->set_src(pid);
        while (cur->next()) {
            int64_t cDate = (int64_t)cur->get_uint64(0);
            if (cDate >= lo_ms && cDate <= hi_ms) count++;
        }
    } else {
        for (node_id_t nb : g.get_out_nodes_id(pid)) {
            if (!is_person(nb)) continue;
            prop_blob pb = g.get_edge_properties(pid, nb);
            if (pb.data && pb.size >= SNBKnowsSchema::TOTAL_SIZE) {
                int64_t cDate = SNBKnowsSchema::get_creation_date(pb.data);
                if (cDate >= lo_ms && cDate <= hi_ms) count++;
            }
        }
    }
    (void)count;
    if (out_ms) { TQ_END_CAP(a2_knows_in_date_range, *out_ms) }
    else        { TQ_END(a2_knows_in_date_range) }
}

static void tq_a3_posts_liked_in_range(GraphBase &g, node_id_t pid,
                                        int64_t lo_ms, int64_t hi_ms,
                                        bool has_props, double *out_ms = nullptr)
{
    TQ_START(a3_posts_liked_in_range)
    int64_t count = 0;
    if (has_props) {
        auto cur = g.get_edge_prop_cursor(LIKES_PROPS_TABLE, CG_TEMPORAL);
        cur->set_src(pid);
        while (cur->next()) {
            int64_t cDate = (int64_t)cur->get_uint64(0);
            if (cDate >= lo_ms && cDate <= hi_ms) count++;
        }
    } else {
        for (node_id_t post_id : g.get_out_nodes_id(pid)) {
            if (!is_post(post_id)) continue;
            prop_blob pb = g.get_edge_properties(pid, post_id);
            if (pb.data && pb.size >= SNBLikesSchema::TOTAL_SIZE) {
                int64_t cDate = SNBLikesSchema::get_creation_date(pb.data);
                if (cDate >= lo_ms && cDate <= hi_ms) count++;
            }
        }
    }
    (void)count;
    if (out_ms) { TQ_END_CAP(a3_posts_liked_in_range, *out_ms) }
    else        { TQ_END(a3_posts_liked_in_range) }
}

static void tq_bi1_posting_summary(GraphBase &g, node_id_t total_posts,
                                    double *out_ms = nullptr)
{
    TQ_START(bi1_posting_summary)
    auto year_of = [](uint64_t e) -> int { return (int)(e / 31557600000ULL) + 1970; };
    auto cat_of  = [](int32_t l) -> int {
        return l < 40 ? 0 : l < 80 ? 1 : l < 255 ? 2 : 3; };
    std::map<std::pair<int,int>, std::pair<int64_t,int64_t>> groups;
    auto cur = g.get_node_prop_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
    cur->set_range(0, OutOfBand_ID_MAX);
    while (cur->next()) {
        uint64_t cDate = cur->get_uint64(0);
        int32_t  length = cur->get_int32(1);
        auto &s = groups[{year_of(cDate), cat_of(length)}];
        s.first++; s.second += length;
    }
    (void)total_posts; (void)groups;
    if (out_ms) { TQ_END_CAP(bi1_posting_summary, *out_ms) }
    else        { TQ_END(bi1_posting_summary) }
}

static void tq_bi2_message_count_two_windows(GraphBase &g,
                                              int64_t lo1, int64_t hi1,
                                              int64_t lo2, int64_t hi2,
                                              double *out_ms = nullptr)
{
    TQ_START(bi2_message_count_two_windows)
    int64_t pw1=0, pw2=0, cw1=0, cw2=0;
    {
        auto cur = g.get_node_prop_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
        cur->set_range(0, OutOfBand_ID_MAX);
        while (cur->next()) {
            int64_t t = (int64_t)cur->get_uint64(0);
            if (t >= lo1 && t < hi1) pw1++;
            if (t >= lo2 && t < hi2) pw2++;
        }
    }
    {
        auto cur = g.get_node_prop_cursor(COMMENT_PROPS_TABLE, CG_TEMPORAL);
        cur->set_range(0, OutOfBand_ID_MAX);
        while (cur->next()) {
            int64_t t = (int64_t)cur->get_uint64(0);
            if (t >= lo1 && t < hi1) cw1++;
            if (t >= lo2 && t < hi2) cw2++;
        }
    }
    (void)pw1; (void)pw2; (void)cw1; (void)cw2;
    if (out_ms) { TQ_END_CAP(bi2_message_count_two_windows, *out_ms) }
    else        { TQ_END(bi2_message_count_two_windows) }
}

static void tq_bi12_message_distribution_fast(GraphBase &g,
                                               node_id_t total_posts,
                                               int64_t max_date = INT64_MAX,
                                               int32_t min_length = 0,
                                               double *out_ms = nullptr)
{
    TQ_START(bi12_message_distribution_fast)
    bool no_filter = (max_date == INT64_MAX && min_length == 0);
    std::unordered_set<node_id_t> qualifying;
    qualifying.reserve(total_posts);
    if (!no_filter) {
        auto prop_cur = g.get_node_prop_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
        prop_cur->set_range(0, OutOfBand_ID_MAX);
        while (prop_cur->next()) {
            node_id_t vid = prop_cur->key();
            uint64_t cDate = prop_cur->get_uint64(0);
            int32_t length = prop_cur->get_int32(1);
            if ((int64_t)cDate <= max_date && length >= min_length)
                qualifying.insert(vid);
        }
    }
    std::unordered_map<node_id_t, int64_t> creator_count;
    EdgeCursor *ec = g.get_edge_iter();
    ec->set_key_range({{MAKE_TYPED_ID(VT_POST, 0), 1},
                       {OutOfBand_ID_MAX, OutOfBand_ID_MAX}});
    edge found;
    ec->next(&found);
    while (found.src_id != OutOfBand_ID_MAX) {
        if (VTYPE_OF(found.src_id) != VT_POST) break;
        if (VTYPE_OF(found.dst_id) == VT_PERSON) {
            if (no_filter || qualifying.count(found.src_id))
                creator_count[found.dst_id]++;
        }
        ec->next(&found);
    }
    delete ec;
    (void)creator_count;
    if (out_ms) { TQ_END_CAP(bi12_message_distribution_fast, *out_ms) }
    else        { TQ_END(bi12_message_distribution_fast) }
}

static void tq_ic7_message_likes(GraphBase &g, node_id_t pid, bool has_props,
                                  double *out_ms = nullptr)
{
    TQ_START(ic7_message_likes)
    std::vector<node_id_t> messages;
    for (node_id_t m : g.get_in_nodes_id(pid)) {
        uint8_t vt = VTYPE_OF(m);
        if (vt == VT_POST || vt == VT_COMMENT) messages.push_back(m);
    }
    std::vector<std::pair<node_id_t, node_id_t>> liker_msg;
    for (node_id_t msg : messages)
        for (node_id_t n : g.get_in_nodes_id(msg))
            if (VTYPE_OF(n) == VT_PERSON) liker_msg.emplace_back(n, msg);
    std::sort(liker_msg.begin(), liker_msg.end());
    std::vector<std::tuple<node_id_t, node_id_t, int64_t>> result;
    if (has_props && !liker_msg.empty()) {
        auto cg = g.get_edge_prop_cursor(LIKES_PROPS_TABLE, CG_TEMPORAL);
        for (auto &[liker, msg] : liker_msg) {
            if (!cg->seek(liker, msg)) continue;
            result.emplace_back(liker, msg, (int64_t)cg->get_uint64(0));
        }
    }
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b){ return std::get<2>(a) > std::get<2>(b); });
    if (result.size() > 20) result.resize(20);
    if (out_ms) { TQ_END_CAP(ic7_message_likes, *out_ms) }
    else        { TQ_END(ic7_message_likes) }
}

static void tq_ic8_latest_replies(GraphBase &g, node_id_t pid, bool has_props,
                                   double *out_ms = nullptr)
{
    TQ_START(ic8_latest_replies)
    std::vector<node_id_t> messages;
    for (node_id_t m : g.get_in_nodes_id(pid)) {
        uint8_t vt = VTYPE_OF(m);
        if (vt == VT_POST || vt == VT_COMMENT) messages.push_back(m);
    }
    std::vector<node_id_t> replies;
    for (node_id_t msg : messages)
        for (node_id_t n : g.get_in_nodes_id(msg))
            if (VTYPE_OF(n) == VT_COMMENT) replies.push_back(n);
    std::sort(replies.begin(), replies.end());
    std::vector<std::tuple<node_id_t, int64_t, node_id_t>> result;
    if (has_props && !replies.empty()) {
        auto cg = g.get_node_prop_cursor(COMMENT_PROPS_TABLE, CG_TEMPORAL);
        for (node_id_t cid : replies) {
            if (!cg->seek(cid)) continue;
            result.emplace_back(cid, (int64_t)cg->get_uint64(0), (node_id_t)0);
        }
    } else {
        for (node_id_t cid : replies)
            result.emplace_back(cid, 0LL, (node_id_t)0);
    }
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b){ return std::get<1>(a) > std::get<1>(b); });
    if (result.size() > 20) result.resize(20);
    for (auto &[cid, cdate, creator] : result)
        for (node_id_t n : g.get_out_nodes_id(cid))
            if (VTYPE_OF(n) == VT_PERSON) { creator = n; break; }
    if (out_ms) { TQ_END_CAP(ic8_latest_replies, *out_ms) }
    else        { TQ_END(ic8_latest_replies) }
}

static void tq_ic9_friends_messages_before(GraphBase &g, node_id_t pid,
                                            int64_t cutoff_ms, bool has_props,
                                            double *out_ms = nullptr)
{
    TQ_START(ic9_friends_messages_before)
    std::vector<node_id_t> friends;
    for (node_id_t f : g.get_out_nodes_id(pid))
        if (VTYPE_OF(f) == VT_PERSON) friends.push_back(f);
    std::vector<std::pair<node_id_t, node_id_t>> msg_creator;
    for (node_id_t f : friends)
        for (node_id_t m : g.get_in_nodes_id(f)) {
            uint8_t vt = VTYPE_OF(m);
            if (vt == VT_POST || vt == VT_COMMENT) msg_creator.emplace_back(m, f);
        }
    std::sort(msg_creator.begin(), msg_creator.end());
    std::vector<std::tuple<node_id_t, int64_t, node_id_t>> result;
    if (has_props && !msg_creator.empty()) {
        auto post_cg    = g.get_node_prop_cursor(POST_PROPS_TABLE,    CG_TEMPORAL);
        auto comment_cg = g.get_node_prop_cursor(COMMENT_PROPS_TABLE, CG_TEMPORAL);
        for (auto &[msg, creator] : msg_creator) {
            auto &cg = (VTYPE_OF(msg) == VT_POST) ? post_cg : comment_cg;
            if (!cg->seek(msg)) continue;
            uint64_t cDate = cg->get_uint64(0);
            if ((int64_t)cDate < cutoff_ms)
                result.emplace_back(msg, (int64_t)cDate, creator);
        }
    }
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b){ return std::get<1>(a) > std::get<1>(b); });
    if (result.size() > 20) result.resize(20);
    if (out_ms) { TQ_END_CAP(ic9_friends_messages_before, *out_ms) }
    else        { TQ_END(ic9_friends_messages_before) }
}

static void tq_ic5_forums_by_friend_membership(GraphBase &g, node_id_t pid,
                                                int64_t /*since_ms*/, bool /*has_props*/,
                                                double *out_ms = nullptr)
{
    TQ_START(ic5_forums_by_friend_membership)
    std::vector<node_id_t> friends;
    for (node_id_t f : g.get_out_nodes_id(pid))
        if (VTYPE_OF(f) == VT_PERSON) friends.push_back(f);
    std::unordered_map<node_id_t, int32_t> forum_count;
    for (node_id_t f : friends)
        for (node_id_t n : g.get_in_nodes_id(f))
            if (VTYPE_OF(n) == VT_FORUM) forum_count[n]++;
    std::vector<std::pair<node_id_t, int32_t>> result(forum_count.begin(), forum_count.end());
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b){ return a.second > b.second; });
    if (result.size() > 20) result.resize(20);
    if (out_ms) { TQ_END_CAP(ic5_forums_by_friend_membership, *out_ms) }
    else        { TQ_END(ic5_forums_by_friend_membership) }
}

static void tq_ic3_fof_by_country(GraphBase &g, node_id_t pid,
                                   node_id_t country_x, node_id_t country_y,
                                   double *out_ms = nullptr)
{
    TQ_START(ic3_fof_by_country)
    std::unordered_set<node_id_t> friends_set;
    for (node_id_t f : g.get_out_nodes_id(pid))
        if (VTYPE_OF(f) == VT_PERSON) friends_set.insert(f);
    std::unordered_map<node_id_t, int32_t> fof_common;
    for (node_id_t f : friends_set)
        for (node_id_t ff : g.get_out_nodes_id(f)) {
            if (VTYPE_OF(ff) != VT_PERSON) continue;
            if (ff == pid || friends_set.count(ff)) continue;
            fof_common[ff]++;
        }
    std::vector<std::pair<node_id_t, int32_t>> result;
    for (auto &[candidate, common] : fof_common) {
        node_id_t c = resolve_person_country_graph(g, candidate);
        if (c == ID_NOT_FOUND) continue;
        if (country_x != ID_NOT_FOUND && c != country_x) continue;
        if (country_y != ID_NOT_FOUND && c == country_y) continue;
        result.emplace_back(candidate, common);
    }
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b){ return a.second > b.second; });
    if (result.size() > 10) result.resize(10);
    if (out_ms) { TQ_END_CAP(ic3_fof_by_country, *out_ms) }
    else        { TQ_END(ic3_fof_by_country) }
}

// ============================================================
// Section D — NeighborCache (in-memory CSR topology cache)
// Verbatim from test/ldbc_snb_queries.cpp
// ============================================================

struct NeighborCache {
    static constexpr int N_VTYPES = 11;

    struct TypeAdj {
        std::vector<uint32_t>  off;
        std::vector<node_id_t> nbr;

        const node_id_t* begin(node_id_t id) const noexcept {
            uint64_t c = VCOUNTER_OF(id);
            return (c + 1 < off.size()) ? nbr.data() + off[c]
                                        : nbr.data() + nbr.size();
        }
        const node_id_t* end(node_id_t id) const noexcept {
            uint64_t c = VCOUNTER_OF(id);
            return (c + 1 < off.size()) ? nbr.data() + off[c + 1]
                                        : nbr.data() + nbr.size();
        }
    };

    TypeAdj out_adj[N_VTYPES];
    TypeAdj in_adj [N_VTYPES];

    const node_id_t* out_begin(node_id_t id) const noexcept {
        uint8_t t = (uint8_t)VTYPE_OF(id);
        return (t < N_VTYPES) ? out_adj[t].begin(id) : nullptr;
    }
    const node_id_t* out_end(node_id_t id) const noexcept {
        uint8_t t = (uint8_t)VTYPE_OF(id);
        return (t < N_VTYPES) ? out_adj[t].end(id) : nullptr;
    }
    const node_id_t* in_begin(node_id_t id) const noexcept {
        uint8_t t = (uint8_t)VTYPE_OF(id);
        return (t < N_VTYPES) ? in_adj[t].begin(id) : nullptr;
    }
    const node_id_t* in_end(node_id_t id) const noexcept {
        uint8_t t = (uint8_t)VTYPE_OF(id);
        return (t < N_VTYPES) ? in_adj[t].end(id) : nullptr;
    }
};

static NeighborCache build_neighbor_cache(GraphBase &graph, double *out_ms = nullptr)
{
    auto t0 = Clock_t::now();
    fprintf(stderr, "[CACHE] building in-memory neighbor cache ... ");

    NeighborCache cache;

    std::vector<std::pair<node_id_t, node_id_t>> edges;
    edges.reserve(32'000'000);
    {
        EdgeCursor *ec = graph.get_edge_iter();
        edge e;
        ec->next(&e);
        while (e.src_id != OutOfBand_ID_MAX) {
            edges.emplace_back(e.src_id, e.dst_id);
            ec->next(&e);
        }
        delete ec;
    }

    auto build_adj = [&](NeighborCache::TypeAdj adj[NeighborCache::N_VTYPES],
                         const std::vector<std::pair<node_id_t, node_id_t>> &sorted_pairs)
    {
        uint64_t max_c[NeighborCache::N_VTYPES] = {};
        for (auto &[nd, nbr] : sorted_pairs) {
            uint8_t vt = (uint8_t)VTYPE_OF(nd);
            if (vt < NeighborCache::N_VTYPES) {
                uint64_t c = VCOUNTER_OF(nd);
                if (c > max_c[vt]) max_c[vt] = c;
            }
        }
        for (int t = 0; t < NeighborCache::N_VTYPES; t++)
            adj[t].off.assign(max_c[t] + 2, 0u);
        for (auto &[nd, nbr] : sorted_pairs) {
            uint8_t vt = (uint8_t)VTYPE_OF(nd);
            if (vt < NeighborCache::N_VTYPES)
                adj[vt].off[VCOUNTER_OF(nd) + 1]++;
        }
        for (int t = 0; t < NeighborCache::N_VTYPES; t++) {
            for (size_t i = 1; i < adj[t].off.size(); i++)
                adj[t].off[i] += adj[t].off[i - 1];
            adj[t].nbr.resize(adj[t].off.back());
        }
        std::vector<uint32_t> cur[NeighborCache::N_VTYPES];
        for (int t = 0; t < NeighborCache::N_VTYPES; t++) {
            if (adj[t].off.size() > 1)
                cur[t].assign(adj[t].off.begin(), adj[t].off.begin() + adj[t].off.size() - 1);
        }
        for (auto &[nd, nbr_id] : sorted_pairs) {
            uint8_t vt = (uint8_t)VTYPE_OF(nd);
            if (vt < NeighborCache::N_VTYPES) {
                uint32_t &pos = cur[vt][VCOUNTER_OF(nd)];
                adj[vt].nbr[pos++] = nbr_id;
            }
        }
    };

    std::sort(edges.begin(), edges.end());
    build_adj(cache.out_adj, edges);

    for (auto &[src, dst] : edges) std::swap(src, dst);
    std::sort(edges.begin(), edges.end());
    build_adj(cache.in_adj, edges);

    double ms = Ms_t(Clock_t::now() - t0).count();
    if (out_ms) *out_ms = ms;
    size_t total_nbrs = 0;
    for (int t = 0; t < NeighborCache::N_VTYPES; t++)
        total_nbrs += cache.out_adj[t].nbr.size();
    fprintf(stderr, "%.3f ms  (%zu edges, %zu out-neighbor entries)\n",
            ms, edges.size(), total_nbrs);
    return cache;
}

// ============================================================
// Section E — OPT-1 cached variants (5 functions)
// Verbatim from test/ldbc_snb_queries.cpp
// ============================================================

static void tq_x3_ic2_friends_recent_posts_cached(GraphBase &g, const NeighborCache &cache,
                                                    node_id_t pid, int64_t cutoff_ms,
                                                    bool has_props, double *out_ms = nullptr)
{
    TQ_START(x3_ic2_friends_recent_posts_cached)
    std::vector<node_id_t> candidate_posts;
    for (const node_id_t *fp = cache.out_begin(pid); fp != cache.out_end(pid); ++fp) {
        if (!is_person(*fp)) continue;
        for (const node_id_t *mp = cache.in_begin(*fp); mp != cache.in_end(*fp); ++mp)
            if (is_post(*mp)) candidate_posts.push_back(*mp);
    }
    std::sort(candidate_posts.begin(), candidate_posts.end());
    std::vector<std::pair<node_id_t, int64_t>> result;
    if (has_props && !candidate_posts.empty()) {
        auto cg = g.get_node_prop_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
        for (node_id_t p2 : candidate_posts) {
            if (!cg->seek(p2)) continue;
            uint64_t cDate = cg->get_uint64(0);
            if ((int64_t)cDate < cutoff_ms) result.emplace_back(p2, (int64_t)cDate);
        }
    } else if (!has_props) {
        for (node_id_t p2 : candidate_posts) result.emplace_back(p2, 0LL);
    }
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b){ return a.second > b.second; });
    if (result.size() > 10) result.resize(10);
    if (out_ms) { TQ_END_CAP(x3_ic2_friends_recent_posts_cached, *out_ms) }
    else        { TQ_END(x3_ic2_friends_recent_posts_cached) }
}

static void tq_ic7_message_likes_cached(GraphBase &g, const NeighborCache &cache,
                                         node_id_t pid, bool has_props,
                                         double *out_ms = nullptr)
{
    TQ_START(ic7_message_likes_cached)
    std::vector<std::pair<node_id_t, node_id_t>> liker_msg;
    for (const node_id_t *mp = cache.in_begin(pid); mp != cache.in_end(pid); ++mp) {
        uint8_t vt = VTYPE_OF(*mp);
        if (vt != VT_POST && vt != VT_COMMENT) continue;
        node_id_t msg = *mp;
        for (const node_id_t *lp = cache.in_begin(msg); lp != cache.in_end(msg); ++lp)
            if (VTYPE_OF(*lp) == VT_PERSON) liker_msg.emplace_back(*lp, msg);
    }
    std::sort(liker_msg.begin(), liker_msg.end());
    std::vector<std::tuple<node_id_t, node_id_t, int64_t>> result;
    if (has_props && !liker_msg.empty()) {
        auto cg = g.get_edge_prop_cursor(LIKES_PROPS_TABLE, CG_TEMPORAL);
        for (auto &[liker, msg] : liker_msg) {
            if (!cg->seek(liker, msg)) continue;
            result.emplace_back(liker, msg, (int64_t)cg->get_uint64(0));
        }
    } else {
        for (auto &[liker, msg] : liker_msg) result.emplace_back(liker, msg, 0LL);
    }
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b){ return std::get<2>(a) > std::get<2>(b); });
    if (result.size() > 20) result.resize(20);
    if (out_ms) { TQ_END_CAP(ic7_message_likes_cached, *out_ms) }
    else        { TQ_END(ic7_message_likes_cached) }
}

static void tq_ic9_friends_messages_before_cached(GraphBase &g, const NeighborCache &cache,
                                                    node_id_t pid, int64_t cutoff_ms,
                                                    bool has_props, double *out_ms = nullptr)
{
    TQ_START(ic9_friends_messages_before_cached)
    std::vector<std::pair<node_id_t, node_id_t>> msg_creator;
    for (const node_id_t *fp = cache.out_begin(pid); fp != cache.out_end(pid); ++fp) {
        if (VTYPE_OF(*fp) != VT_PERSON) continue;
        node_id_t f = *fp;
        for (const node_id_t *mp = cache.in_begin(f); mp != cache.in_end(f); ++mp) {
            uint8_t vt = VTYPE_OF(*mp);
            if (vt == VT_POST || vt == VT_COMMENT) msg_creator.emplace_back(*mp, f);
        }
    }
    std::sort(msg_creator.begin(), msg_creator.end());
    std::vector<std::tuple<node_id_t, int64_t, node_id_t>> result;
    if (has_props && !msg_creator.empty()) {
        auto post_cg    = g.get_node_prop_cursor(POST_PROPS_TABLE,    CG_TEMPORAL);
        auto comment_cg = g.get_node_prop_cursor(COMMENT_PROPS_TABLE, CG_TEMPORAL);
        for (auto &[msg, creator] : msg_creator) {
            auto &cg = (VTYPE_OF(msg) == VT_POST) ? post_cg : comment_cg;
            if (!cg->seek(msg)) continue;
            uint64_t cDate = cg->get_uint64(0);
            if ((int64_t)cDate < cutoff_ms)
                result.emplace_back(msg, (int64_t)cDate, creator);
        }
    } else {
        for (auto &[msg, creator] : msg_creator) result.emplace_back(msg, 0LL, creator);
    }
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b){ return std::get<1>(a) > std::get<1>(b); });
    if (result.size() > 20) result.resize(20);
    if (out_ms) { TQ_END_CAP(ic9_friends_messages_before_cached, *out_ms) }
    else        { TQ_END(ic9_friends_messages_before_cached) }
}

static void tq_ic5_forums_by_friend_membership_cached(GraphBase &g, const NeighborCache &cache,
                                                        node_id_t pid, int64_t /*since_ms*/,
                                                        bool /*has_props*/, double *out_ms = nullptr)
{
    TQ_START(ic5_forums_by_friend_membership_cached)
    std::unordered_map<node_id_t, int32_t> forum_count;
    for (const node_id_t *fp = cache.out_begin(pid); fp != cache.out_end(pid); ++fp) {
        if (VTYPE_OF(*fp) != VT_PERSON) continue;
        node_id_t f = *fp;
        for (const node_id_t *np = cache.in_begin(f); np != cache.in_end(f); ++np)
            if (VTYPE_OF(*np) == VT_FORUM) forum_count[*np]++;
    }
    std::vector<std::pair<node_id_t, int32_t>> result(forum_count.begin(), forum_count.end());
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b){ return a.second > b.second; });
    if (result.size() > 20) result.resize(20);
    if (out_ms) { TQ_END_CAP(ic5_forums_by_friend_membership_cached, *out_ms) }
    else        { TQ_END(ic5_forums_by_friend_membership_cached) }
}

static void tq_ic3_fof_by_country_cached(const NeighborCache &cache,
                                          node_id_t pid,
                                          node_id_t country_x, node_id_t country_y,
                                          double *out_ms = nullptr)
{
    TQ_START(ic3_fof_by_country_cached)
    std::unordered_set<node_id_t> friends_set;
    for (const node_id_t *fp = cache.out_begin(pid); fp != cache.out_end(pid); ++fp)
        if (VTYPE_OF(*fp) == VT_PERSON) friends_set.insert(*fp);

    std::unordered_map<node_id_t, int32_t> fof_common;
    for (node_id_t f : friends_set)
        for (const node_id_t *ffp = cache.out_begin(f); ffp != cache.out_end(f); ++ffp) {
            if (VTYPE_OF(*ffp) != VT_PERSON) continue;
            if (*ffp == pid || friends_set.count(*ffp)) continue;
            fof_common[*ffp]++;
        }

    std::vector<std::pair<node_id_t, int32_t>> result;
    for (auto &[candidate, common] : fof_common) {
        node_id_t country = ID_NOT_FOUND;
        for (const node_id_t *cp = cache.out_begin(candidate);
             cp != cache.out_end(candidate) && country == ID_NOT_FOUND; ++cp) {
            if (VTYPE_OF(*cp) != VT_CITY) continue;
            for (const node_id_t *ccp = cache.out_begin(*cp);
                 ccp != cache.out_end(*cp); ++ccp) {
                if (VTYPE_OF(*ccp) == VT_COUNTRY) { country = *ccp; break; }
            }
        }
        if (country == ID_NOT_FOUND) continue;
        if (country_x != ID_NOT_FOUND && country != country_x) continue;
        if (country_y != ID_NOT_FOUND && country == country_y) continue;
        result.emplace_back(candidate, common);
    }
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b){ return a.second > b.second; });
    if (result.size() > 10) result.resize(10);
    if (out_ms) { TQ_END_CAP(ic3_fof_by_country_cached, *out_ms) }
    else        { TQ_END(ic3_fof_by_country_cached) }
}

// ============================================================
// Section F — Parallel BI variants (require <omp.h>)
// Verbatim from test/ldbc_snb_queries.cpp
// ============================================================

#include <omp.h>

static void bi1_posting_summary_parallel(WT_CONNECTION *conn,
                                          graph_opts &ro_opts,
                                          node_id_t total_posts,
                                          int n_threads,
                                          double *out_ms = nullptr)
{
    TQ_START(bi1_posting_summary_parallel)
    auto year_of = [](uint64_t e) -> int { return (int)(e / 31557600000ULL) + 1970; };
    auto cat_of  = [](int32_t l) -> int {
        return l < 40 ? 0 : l < 80 ? 1 : l < 255 ? 2 : 3; };

    using GroupMap = std::map<std::pair<int,int>, std::pair<int64_t,int64_t>>;
    std::vector<GroupMap> local_groups(n_threads);
    node_id_t chunk = (total_posts + n_threads - 1) / n_threads;

    std::vector<GraphBase*> handles(n_threads);
    for (int t = 0; t < n_threads; t++)
        handles[t] = (ro_opts.type == GraphType::Adj)
            ? (GraphBase*) new AdjList(ro_opts, conn)
            : (GraphBase*) new SplitEdgeKey(ro_opts, conn);

#pragma omp parallel for num_threads(n_threads) schedule(static,1)
    for (int t = 0; t < n_threads; t++) {
        node_id_t start_id = MAKE_TYPED_ID(VT_POST, (node_id_t)t * chunk);
        node_id_t end_id   = MAKE_TYPED_ID(VT_POST,
                                 std::min((node_id_t)(t + 1) * chunk, total_posts));
        auto cg = handles[t]->get_node_prop_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
        cg->set_range(start_id, end_id);
        while (cg->next()) {
            uint64_t cDate = cg->get_uint64(0);
            int32_t  length = cg->get_int32(1);
            auto &s = local_groups[t][{year_of(cDate), cat_of(length)}];
            s.first++; s.second += length;
        }
    }
    for (int t = 0; t < n_threads; t++) handles[t]->close(false);

    GroupMap groups;
    for (int t = 0; t < n_threads; t++)
        for (auto &[k, s] : local_groups[t]) {
            groups[k].first  += s.first;
            groups[k].second += s.second;
        }

    if (out_ms) { TQ_END_CAP(bi1_posting_summary_parallel, *out_ms) }
    else        { TQ_END(bi1_posting_summary_parallel) }
    fprintf(stderr, "  BI-1-parallel (%llu posts, %d threads, %zu groups)\n",
            (unsigned long long)total_posts, n_threads, groups.size());
}

static void bi12_message_distribution_fast_parallel(WT_CONNECTION *conn,
                                                     graph_opts &ro_opts,
                                                     node_id_t total_posts,
                                                     int n_threads,
                                                     int64_t max_date = INT64_MAX,
                                                     int32_t min_length = 0,
                                                     double *out_ms = nullptr)
{
    TQ_START(bi12_message_distribution_fast_parallel)
    bool no_filter = (max_date == INT64_MAX && min_length == 0);
    node_id_t chunk = (total_posts + n_threads - 1) / n_threads;

    std::unordered_set<node_id_t> qualifying;
    if (!no_filter) {
        std::vector<std::unordered_set<node_id_t>> local_q(n_threads);
        std::vector<GraphBase*> p1_handles(n_threads);
        for (int t = 0; t < n_threads; t++)
            p1_handles[t] = (ro_opts.type == GraphType::Adj)
                ? (GraphBase*) new AdjList(ro_opts, conn)
                : (GraphBase*) new SplitEdgeKey(ro_opts, conn);
#pragma omp parallel for num_threads(n_threads) schedule(static,1)
        for (int t = 0; t < n_threads; t++) {
            node_id_t start_id = MAKE_TYPED_ID(VT_POST, (node_id_t)t * chunk);
            node_id_t end_id   = MAKE_TYPED_ID(VT_POST,
                                     std::min((node_id_t)(t + 1) * chunk, total_posts));
            auto cg = p1_handles[t]->get_node_prop_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
            cg->set_range(start_id, end_id);
            while (cg->next()) {
                node_id_t vid = cg->key();
                uint64_t cDate = cg->get_uint64(0);
                int32_t length = cg->get_int32(1);
                if ((int64_t)cDate <= max_date && length >= min_length)
                    local_q[t].insert(vid);
            }
        }
        for (int t = 0; t < n_threads; t++) p1_handles[t]->close(false);
        qualifying.reserve(total_posts);
        for (int t = 0; t < n_threads; t++) qualifying.merge(local_q[t]);
    }

    std::vector<std::unordered_map<node_id_t, int64_t>> local_counts(n_threads);
    std::vector<GraphBase*> p2_handles(n_threads);
    for (int t = 0; t < n_threads; t++)
        p2_handles[t] = (ro_opts.type == GraphType::Adj)
            ? (GraphBase*) new AdjList(ro_opts, conn)
            : (GraphBase*) new SplitEdgeKey(ro_opts, conn);
#pragma omp parallel for num_threads(n_threads) schedule(static,1)
    for (int t = 0; t < n_threads; t++) {
        node_id_t start_src = MAKE_TYPED_ID(VT_POST, (node_id_t)t * chunk);
        node_id_t end_src   = MAKE_TYPED_ID(VT_POST,
                                  std::min((node_id_t)(t + 1) * chunk, total_posts));
        EdgeCursor *ec = p2_handles[t]->get_edge_iter();
        ec->set_key_range({{start_src, 1}, {OutOfBand_ID_MAX, OutOfBand_ID_MAX}});
        edge found;
        ec->next(&found);
        while (found.src_id != OutOfBand_ID_MAX) {
            if (VTYPE_OF(found.src_id) != VT_POST) break;
            if (found.src_id >= end_src) break;
            if (VTYPE_OF(found.dst_id) == VT_PERSON) {
                if (no_filter || qualifying.count(found.src_id))
                    local_counts[t][found.dst_id]++;
            }
            ec->next(&found);
        }
        delete ec;
    }
    for (int t = 0; t < n_threads; t++) p2_handles[t]->close(false);

    std::unordered_map<node_id_t, int64_t> creator_count;
    for (int t = 0; t < n_threads; t++)
        for (auto &[pid2, cnt] : local_counts[t])
            creator_count[pid2] += cnt;

    if (out_ms) { TQ_END_CAP(bi12_message_distribution_fast_parallel, *out_ms) }
    else        { TQ_END(bi12_message_distribution_fast_parallel) }
    fprintf(stderr, "  BI-12-fast-parallel (%llu posts, %d threads, %zu creators)\n",
            (unsigned long long)total_posts, n_threads, creator_count.size());
}

static void bi12_message_distribution_fast_v2(GraphBase &g,
                                               node_id_t total_posts,
                                               int64_t max_date = INT64_MAX,
                                               int32_t min_length = 0,
                                               double *out_ms = nullptr)
{
    TQ_START(bi12_message_distribution_fast_v2)
    bool no_filter = (max_date == INT64_MAX && min_length == 0);
    std::vector<uint8_t> qualifying;
    if (!no_filter) {
        qualifying.assign(total_posts, 0);
        auto prop_cur = g.get_node_prop_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
        prop_cur->set_range(0, OutOfBand_ID_MAX);
        while (prop_cur->next()) {
            node_id_t vid = prop_cur->key();
            uint64_t cDate = prop_cur->get_uint64(0);
            int32_t length = prop_cur->get_int32(1);
            if ((int64_t)cDate <= max_date && length >= min_length) {
                uint64_t c = VCOUNTER_OF(vid);
                if (c < (uint64_t)total_posts) qualifying[c] = 1;
            }
        }
    }
    std::unordered_map<node_id_t, int64_t> creator_count;
    EdgeCursor *ec = g.get_edge_iter();
    ec->set_key_range({{MAKE_TYPED_ID(VT_POST, 0), 1},
                       {OutOfBand_ID_MAX, OutOfBand_ID_MAX}});
    edge found;
    ec->next(&found);
    while (found.src_id != OutOfBand_ID_MAX) {
        if (VTYPE_OF(found.src_id) != VT_POST) break;
        if (VTYPE_OF(found.dst_id) == VT_PERSON) {
            bool ok = no_filter;
            if (!ok) {
                uint64_t c = VCOUNTER_OF(found.src_id);
                ok = (c < (uint64_t)total_posts) && qualifying[c];
            }
            if (ok) creator_count[found.dst_id]++;
        }
        ec->next(&found);
    }
    delete ec;
    if (out_ms) { TQ_END_CAP(bi12_message_distribution_fast_v2, *out_ms) }
    else        { TQ_END(bi12_message_distribution_fast_v2) }
    fprintf(stderr, "  BI-12-fast-v2 (%llu posts, %zu creators)\n",
            (unsigned long long)total_posts, creator_count.size());
}
