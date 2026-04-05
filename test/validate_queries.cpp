// validate_queries.cpp — outputs LDBC SNB query results as JSON lines (stdout).
//
// Used by validate_results.py to compare Flexograph results against NeuG.
// Suppresses all timing/debug output; only structured JSON goes to stdout.
//
// Usage:
//   validate_queries <db_dir> [adj|splitekey] [--embedded]
//
// Vertex IDs in output are encoded as compact strings so the Python script
// can map them back to LDBC IDs using CSV loading order:
//   "p<N>"  VT_PERSON   counter N
//   "q<N>"  VT_POST     counter N
//   "c<N>"  VT_COMMENT  counter N
//   "f<N>"  VT_FORUM    counter N
//
// Property mode:
//   default (COLUMNAR): uses colgroup cursors for edge props; get_node_properties
//                       for node props.
//   --embedded:         uses get_node_properties + get_edge_properties throughout.

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "common_defs.h"
#include "graph_engine.h"
#include "prop_schema.h"

// ─── Vertex type helpers ──────────────────────────────────────────────────────

static bool is_person(node_id_t id)  { return VTYPE_OF(id) == VT_PERSON; }
static bool is_post(node_id_t id)    { return VTYPE_OF(id) == VT_POST; }
static bool is_comment(node_id_t id) { return VTYPE_OF(id) == VT_COMMENT; }
static bool is_forum(node_id_t id)   { return VTYPE_OF(id) == VT_FORUM; }
static bool is_message(node_id_t id) { return is_post(id) || is_comment(id); }

// ─── ID encoding ─────────────────────────────────────────────────────────────

static std::string enc(node_id_t id)
{
    char buf[32];
    uint64_t c = VCOUNTER_OF(id);
    switch (VTYPE_OF(id)) {
        case VT_PERSON:  snprintf(buf, sizeof(buf), "p%llu", (unsigned long long)c); break;
        case VT_POST:    snprintf(buf, sizeof(buf), "q%llu", (unsigned long long)c); break;
        case VT_COMMENT: snprintf(buf, sizeof(buf), "c%llu", (unsigned long long)c); break;
        case VT_FORUM:   snprintf(buf, sizeof(buf), "f%llu", (unsigned long long)c); break;
        default:         snprintf(buf, sizeof(buf), "x%llu_t%u",
                                  (unsigned long long)c,
                                  (unsigned)VTYPE_OF(id)); break;
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

static DecodedPerson decode_person(const prop_blob &pb)
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
    int64_t creationDate = 0;
    int32_t length = 0;
    int8_t  tag = 0;
    std::string content;
    bool valid = false;
};

static DecodedPost decode_post(const prop_blob &pb)
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

// ─── Query implementations ────────────────────────────────────────────────────

// R1: Person profile
static void run_r1(GraphBase &g, node_id_t pid)
{
    prop_blob pb = g.get_node_properties(pid);
    DecodedPerson p = decode_person(pb);
    auto emails = g.get_person_emails(pid);
    auto langs  = g.get_person_languages(pid);

    printf("{\"query\":\"r1\",\"pid\":%llu", (unsigned long long)VCOUNTER_OF(pid));
    if (!p.valid) { printf(",\"error\":\"no_props\"}\n"); return; }
    printf(",\"firstName\":%s",   jstr(p.firstName).c_str());
    printf(",\"lastName\":%s",    jstr(p.lastName).c_str());
    printf(",\"gender\":%d",      (int)p.gender);
    printf(",\"birthday\":%lld",  (long long)p.birthday);
    printf(",\"creationDate\":%lld", (long long)p.creationDate);
    printf(",\"locationIP\":%s",  jstr(p.locIP).c_str());
    printf(",\"browserUsed\":%s", jstr(p.browser).c_str());
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
    } else {
        for (node_id_t nb : g.get_out_nodes_id(pid)) {
            if (!is_person(nb)) continue;
            prop_blob pb = g.get_edge_properties(pid, nb);
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
            // Undirected: traverse out-edges and in-edges (knows stored once per pair)
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
    DecodedPost p = decode_post(pb);
    printf("{\"query\":\"x1\",\"post\":%llu", (unsigned long long)VCOUNTER_OF(post_id));
    if (!p.valid) { printf(",\"error\":\"no_props\"}\n"); return; }
    printf(",\"creationDate\":%lld", (long long)p.creationDate);
    printf(",\"length\":%d",         p.length);
    printf(",\"tag\":%d",            (int)p.tag);
    printf(",\"content\":%s",        jstr(p.content).c_str());
    printf("}\n");
}

// X2: Post -> author
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

    std::vector<std::pair<int64_t, node_id_t>> cands; // (date, post_id)

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
                DecodedPost p2 = decode_post(pb);
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

// A1: Out-degree and in-degree (all edge types)
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
    // Scan all posts; use node property blob for post dates/lengths.
    // get_node_properties works in both EMBEDDED and COLUMNAR modes.
    std::map<std::pair<int,int>, std::pair<int64_t,int64_t>> groups; // (year,cat) -> (cnt, sum_len)

    // Iterate over all post nodes: VT_POST counters 0..N-1
    node_id_t total_posts = g.get_num_nodes();  // upper bound; walk via cursor
    // Use an edge cursor approach: scan a NodeCursor for VT_POST nodes
    // Actually, iterate post props via the temporal colgroup if COLUMNAR,
    // or via NodeCursor if EMBEDDED.
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
        // Scan nodes: try VT_POST counters sequentially until we miss
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
        // find creator: out-edge from post to person
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

    auto count_vttype = [&](VertexType vt,
                            const std::string &table,
                            int64_t &w1, int64_t &w2)
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
                // For comments we'd need SNBCommentSchema; use generic offset 0 = creationDate
                // Both schemas have creationDate as the first Q field (8 bytes at offset 0).
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
static void run_ic7(GraphBase &g, node_id_t pid, bool has_props)
{
    // Step 1: collect all messages authored by pid
    std::vector<node_id_t> msgs;
    for (node_id_t nb : g.get_in_nodes_id(pid))
        if (is_message(nb)) msgs.push_back(nb);

    // Step 2: for each message, collect likers and their like date
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
static void run_ic8(GraphBase &g, node_id_t pid, bool has_props)
{
    struct Row { node_id_t reply, creator; int64_t cDate; };
    std::vector<Row> rows;

    for (node_id_t msg : g.get_in_nodes_id(pid)) {
        if (!is_message(msg)) continue;
        for (node_id_t reply : g.get_in_nodes_id(msg)) {
            if (!is_comment(reply)) continue;
            int64_t cDate = -1;
            prop_blob pb = g.get_node_properties(reply);
            // SNBCommentSchema: first 8 bytes = creationDate
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
static void run_ic9(GraphBase &g, node_id_t pid, int64_t cutoff_ms, bool has_props)
{
    std::vector<node_id_t> friends;
    for (node_id_t nb : g.get_out_nodes_id(pid))
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
static void run_ic5(GraphBase &g, node_id_t pid, int64_t since_ms, bool has_props)
{
    std::vector<node_id_t> friends;
    for (node_id_t nb : g.get_out_nodes_id(pid))
        if (is_person(nb)) friends.push_back(nb);

    std::unordered_set<node_id_t> friend_set(friends.begin(), friends.end());
    std::unordered_map<node_id_t,int64_t> forum_cnt; // forum -> qualifying member count

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
static void run_ic3(GraphBase &g, node_id_t pid,
                    node_id_t country_x_id, node_id_t country_y_id)
{
    // Build direct friend set
    std::unordered_set<node_id_t> direct;
    for (node_id_t nb : g.get_out_nodes_id(pid))
        if (is_person(nb)) direct.insert(nb);
    for (node_id_t nb : g.get_in_nodes_id(pid))
        if (is_person(nb)) direct.insert(nb);

    // 2-hop expansion: FoF candidates + common friend count
    std::unordered_map<node_id_t,int64_t> common_cnt;
    for (node_id_t f : direct) {
        std::vector<node_id_t> fof_nbrs = g.get_out_nodes_id(f);
        for (node_id_t v : g.get_in_nodes_id(f))
            if (is_person(v)) fof_nbrs.push_back(v);
        for (node_id_t fof : fof_nbrs) {
            if (!is_person(fof)) continue;
            if (fof == pid || direct.count(fof)) continue;
            common_cnt[fof]++;
        }
    }

    // Resolve each FoF's country: personIsLocatedIn -> city, isPartOf -> country
    auto resolve_country = [&](node_id_t person) -> node_id_t {
        for (node_id_t city : g.get_out_nodes_id(person)) {
            if (VTYPE_OF(city) != VT_CITY && VTYPE_OF(city) != VT_COUNTRY &&
                VTYPE_OF(city) != VT_CONTINENT) continue;
            // city could be VT_CITY; country is one hop via isPartOf
            for (node_id_t country : g.get_out_nodes_id(city))
                if (VTYPE_OF(country) == VT_COUNTRY) return country;
            // if city is already a country
            if (VTYPE_OF(city) == VT_COUNTRY) return city;
        }
        return OutOfBand_ID_MAX;
    };

    struct Row { node_id_t fof; int64_t common; };
    std::vector<Row> rows;
    for (auto &kv : common_cnt) {
        node_id_t fof = kv.first;
        node_id_t country = resolve_country(fof);
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

// ─── main ─────────────────────────────────────────────────────────────────────

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <db_dir> [adj|splitekey] [--embedded]\n", argv[0]);
        return 1;
    }

    std::string db_dir = argv[1];
    GraphType   gtype  = GraphType::SplitEKey;
    bool        emb    = false;

    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if      (a == "adj")        gtype = GraphType::Adj;
        else if (a == "splitekey")  gtype = GraphType::SplitEKey;
        else if (a == "--embedded") emb   = true;
        else { fprintf(stderr, "Unknown arg: %s\n", argv[i]); return 1; }
    }

    graph_opts opts;
    opts.create_new      = false;
    opts.is_directed     = true;
    opts.read_optimize   = true;
    opts.has_node_props  = true;
    opts.has_edge_props  = true;
    opts.prop_mode       = emb ? EMBEDDED : COLUMNAR;
    opts.type            = gtype;
    opts.db_name         = "ldbc_snb_queries";
    opts.db_dir          = db_dir;
    opts.conn_config     = "cache_size=4GB";
    opts.stat_log        = "./";

    fprintf(stderr, "[validate_queries] db=%s type=%s mode=%s\n",
            db_dir.c_str(),
            gtype == GraphType::Adj ? "adj" : "splitekey",
            emb ? "embedded" : "columnar");

    GraphEngine engine(1, opts);
    GraphBase  *g = engine.create_graph_handle();

    node_id_t pid0  = MAKE_TYPED_ID(VT_PERSON, 0);
    node_id_t pid1  = MAKE_TYPED_ID(VT_PERSON, 1);
    node_id_t post0 = MAKE_TYPED_ID(VT_POST,   0);

    // Sample country IDs: first two countries by counter
    node_id_t cx = MAKE_TYPED_ID(VT_COUNTRY, 0);
    node_id_t cy = MAKE_TYPED_ID(VT_COUNTRY, 1);

    bool has_props = !emb;  // COLUMNAR uses colgroup cursors; EMBEDDED uses blobs
    const int64_t INT64_MAX_VAL = 9223372036854775807LL;

    fprintf(stderr, "[validate_queries] running queries...\n");

    run_r1(*g, pid0);
    run_r2(*g, pid0, has_props);
    run_r3(*g, pid0, pid1);
    run_x1(*g, post0);
    run_x2(*g, post0);
    run_x3(*g, pid0, INT64_MAX_VAL, has_props);
    run_x5(*g, post0, 0LL, INT64_MAX_VAL);
    run_a1(*g, pid0);
    run_a2(*g, pid0, 0LL, INT64_MAX_VAL, has_props);
    run_a3(*g, pid1, 0LL, INT64_MAX_VAL, has_props);
    run_bi1(*g, has_props);
    run_bi12(*g);
    run_bi2(*g, has_props);
    run_ic7(*g, pid0, has_props);
    run_ic8(*g, pid0, has_props);
    run_ic9(*g, pid0, INT64_MAX_VAL, has_props);
    run_ic5(*g, pid0, 0LL, has_props);
    run_ic3(*g, pid0, cx, cy);

    fprintf(stderr, "[validate_queries] done.\n");
    return 0;
}
