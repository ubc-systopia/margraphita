// snb_bulk_load.cpp — multithreaded, RAM-efficient LDBC SNB full-schema bulk loader.
//
// RAM advantages over ldbc_snb_loader.h:
//   1. No in-memory accumulation of pending props.  Each vertex/edge loader
//      streams its property blobs to a binary spool file on disk; the flush
//      phase reads and applies them sequentially with O(1) RAM per record.
//   2. Sorted-vector ID maps (SnbIdMap) instead of unordered_map: 16 B/entry
//      vs ~40 B/entry, no per-bucket overhead.  For SF-10 with ~15 M comments
//      the comment_map alone drops from ~600 MB to ~240 MB.
//   3. Vertex types and edge types are loaded in parallel, saturating I/O and
//      CPU across phases.
//
// Loading phases:
//   Phase 1a  parallel: places, orgs, tags, tagclasses, persons, posts, comments
//             (person emails + languages loaded inline for persons)
//   Phase 1b  sequential: forums (embeds moderator person_id → needs person_map)
//   Phase 2   parallel in batches of --threads: all edge CSVs
//   Phase 3   parallel: flush node props from spool files
//   Phase 4   parallel: flush edge props from spool files
//   Checkpoint and close.
//
// Usage:
//   snb_bulk_load <data_dir> <db_dir> [adj|splitekey]
//                [--threads N]        (default 8)
//                [--cache N]          (WT cache size in GB, default 4)
//                [--spool DIR]        (temp dir for spools, default <db_dir>/spool)
//                [--embedded]         (EMBEDDED prop mode instead of COLUMNAR)
//                [--no-props]         (topology only, no property blobs)
//                [--dry-run]          (parse and build ID maps only, no WT writes)
//                [--keep-spool]       (do not delete spool files after loading)

#include <sys/stat.h>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "common_defs.h"
#include "graph_engine.h"
#include "prop_schema.h"
#include "snb_id_map.h"
#include "snb_prop_spool.h"

// ============================================================
// CSV helpers
// ============================================================

static std::vector<std::string> csv_fields(const std::string& line, char delim = '|') {
    std::vector<std::string> v;
    std::istringstream ss(line);
    std::string tok;
    while (std::getline(ss, tok, delim))
        v.push_back(tok);
    return v;
}

static int64_t parse_ms(const std::string& s) {
    return s.empty() ? 0LL : std::stoll(s);
}

static int col_of(const std::vector<std::string>& hdr, const char* name) {
    for (int i = 0; i < (int)hdr.size(); i++)
        if (hdr[i] == name) return i;
    return -1;
}

static const std::string& safe_field(const std::vector<std::string>& flds, int col) {
    static const std::string empty;
    return (col >= 0 && col < (int)flds.size()) ? flds[col] : empty;
}

// ============================================================
// LoaderState — shared read-write between phases
// ============================================================

struct State {
    // Paths
    std::string dyn;        // <data_dir>/dynamic
    std::string sta;        // <data_dir>/static
    std::string spool_dir;

    // Options
    int  num_threads = 8;
    bool has_props   = true;
    bool dry_run     = false;
    bool keep_spool  = false;
    bool verbose     = true;

    // Graph engine (created once, shared across all threads)
    GraphEngine* engine = nullptr;
    graph_opts   gopts;

    // ID maps (written by Phase 1, read-only during Phase 2+)
    SnbIdMap person_map, post_map, comment_map, forum_map;
    SnbIdMap tag_map, tagclass_map, place_map, org_map;

    // Vertex counts (set by Phase 1 threads)
    size_t n_person = 0, n_post = 0, n_comment = 0, n_forum = 0;
    size_t n_tag = 0, n_tagclass = 0;
    size_t n_city = 0, n_country = 0, n_continent = 0;
    size_t n_company = 0, n_university = 0;
    std::atomic<size_t> n_edges{0};

    // Thread-safe exception capture
    std::mutex           ex_mutex;
    std::vector<std::exception_ptr> exceptions;
    void capture() { std::lock_guard<std::mutex> lk(ex_mutex); exceptions.push_back(std::current_exception()); }
    void rethrow()  { if (!exceptions.empty()) std::rethrow_exception(exceptions.front()); }
};

// Spool path helpers
static std::string nspool(const State& st, const char* name) { return st.spool_dir + "/" + name + ".nspool"; }
static std::string espool(const State& st, const char* name) { return st.spool_dir + "/" + name + ".espool"; }

// Progress log
static void log_done(const State& st, const char* label, size_t count = 0) {
    if (!st.verbose) return;
    if (count > 0)
        fprintf(stderr, "[BULK] %-50s %zu\n", label, count);
    else
        fprintf(stderr, "[BULK] %-50s done\n", label);
}
static void log_skip(const State& st, const char* label) {
    if (st.verbose) fprintf(stderr, "[BULK] %-50s skip\n", label);
}

// ============================================================
// Phase 1 — vertex loaders (one thread per vertex type)
// ============================================================

static void load_persons(State& st) {
    const std::string path = st.dyn + "/person_0_0.csv";
    std::ifstream f(path);
    if (!f.is_open()) { log_skip(st, "persons"); return; }

    std::string line;
    if (!std::getline(f, line)) return;
    auto hdr = csv_fields(line);
    int c_id  = col_of(hdr,"id"),  c_gen = col_of(hdr,"gender");
    int c_bday= col_of(hdr,"birthday"), c_cdate = col_of(hdr,"creationDate");
    int c_fn  = col_of(hdr,"firstName"), c_ln = col_of(hdr,"lastName");
    int c_br  = col_of(hdr,"browserUsed"), c_ip = col_of(hdr,"locationIP");
    if (c_id < 0) throw std::runtime_error("person CSV missing 'id'");

    GraphBase* h   = st.engine->create_graph_handle();
    NodePropSpool* sp = (st.has_props) ? new NodePropSpool(nspool(st,"person")) : nullptr;
    node_id_t ctr  = 0;

    while (std::getline(f, line)) {
        if (line.empty()) continue;
        auto flds = csv_fields(line);
        int64_t  lid = std::stoll(flds[c_id]);
        node_id_t tid = MAKE_TYPED_ID(VT_PERSON, ctr++);
        st.person_map.insert(lid, tid);
        if (!st.dry_run) {
            node n; n.id = tid; n.in_degree = 0; n.out_degree = 0;
            h->add_node(n, false);
        }
        if (sp) {
            uint8_t buf[SNBPersonSchema::TOTAL_SIZE] = {};
            SNBPersonSchema::set_creation_date(buf, parse_ms(safe_field(flds,c_cdate)));
            SNBPersonSchema::set_birthday(buf, parse_ms(safe_field(flds,c_bday)));
            const std::string& g = safe_field(flds,c_gen);
            SNBPersonSchema::set_gender(buf, g=="female" ? 1 : 0);
            SNBPersonSchema::set_first_name(buf,   safe_field(flds,c_fn).c_str());
            SNBPersonSchema::set_last_name(buf,    safe_field(flds,c_ln).c_str());
            SNBPersonSchema::set_browser_used(buf, safe_field(flds,c_br).c_str());
            SNBPersonSchema::set_location_ip(buf,  safe_field(flds,c_ip).c_str());
            sp->write(tid, buf, SNBPersonSchema::TOTAL_SIZE);
        }
    }

    st.n_person = ctr;
    st.person_map.sort();
    if (sp) { sp->close(); delete sp; }
    if (!st.dry_run) h->close(false);
    log_done(st, "persons", ctr);
}

// Helper for Post and Comment (same SNBPostSchema layout)
static void load_message_vertices(State& st,
    const std::string& path, const char* label,
    int vtype, SnbIdMap& id_map, size_t& counter_out) {
    std::ifstream f(path);
    if (!f.is_open()) { log_skip(st, label); return; }

    std::string line;
    if (!std::getline(f, line)) return;
    auto hdr = csv_fields(line);
    int c_id = col_of(hdr,"id"), c_cd = col_of(hdr,"creationDate");
    int c_len = col_of(hdr,"length"), c_img = col_of(hdr,"imageFile");
    int c_cnt = col_of(hdr,"content");
    if (c_id < 0) throw std::runtime_error(std::string(label) + " CSV missing 'id'");

    GraphBase* h  = st.engine->create_graph_handle();
    NodePropSpool* sp = (st.has_props) ? new NodePropSpool(nspool(st, label)) : nullptr;
    node_id_t ctr = 0;

    while (std::getline(f, line)) {
        if (line.empty()) continue;
        auto flds = csv_fields(line);
        int64_t  lid = std::stoll(flds[c_id]);
        node_id_t tid = MAKE_TYPED_ID(vtype, ctr++);
        id_map.insert(lid, tid);
        if (!st.dry_run) {
            node n; n.id = tid; n.in_degree = 0; n.out_degree = 0;
            h->add_node(n, false);
        }
        if (sp) {
            // imageFile present → tag=1; otherwise tag=0 and use content field.
            std::string cstr;
            int8_t tag_val = 0;
            const std::string& img = safe_field(flds, c_img);
            if (!img.empty()) {
                cstr = img;  tag_val = 1;
            } else {
                cstr = safe_field(flds, c_cnt);
                if (cstr.size() > SNBPostSchema::CONTENT_MAX_LEN)
                    cstr.resize(SNBPostSchema::CONTENT_MAX_LEN);
            }
            uint32_t bsz = (uint32_t)(SNBPostSchema::TOTAL_SIZE + cstr.size() + 1);
            std::vector<uint8_t> buf(bsz, 0);
            SNBPostSchema::set_creation_date(buf.data(), parse_ms(safe_field(flds,c_cd)));
            const std::string& ls = safe_field(flds,c_len);
            if (!ls.empty()) SNBPostSchema::set_length(buf.data(), std::stoi(ls));
            SNBPostSchema::set_tag(buf.data(), tag_val);
            SNBPostSchema::set_content(buf.data(), cstr.c_str());
            sp->write(tid, buf.data(), bsz);
        }
    }

    counter_out = ctr;
    id_map.sort();
    if (sp) { sp->close(); delete sp; }
    if (!st.dry_run) h->close(false);
    log_done(st, label, ctr);
}

static void load_posts(State& st) {
    load_message_vertices(st, st.dyn+"/post_0_0.csv", "post",
                          VT_POST, st.post_map, st.n_post);
}
static void load_comments(State& st) {
    load_message_vertices(st, st.dyn+"/comment_0_0.csv", "comment",
                          VT_COMMENT, st.comment_map, st.n_comment);
}

// Helper for fixed-schema 160-byte vertex types (Tag, TagClass)
static void load_tag_vertex(State& st,
    const std::string& path, const char* label,
    int vtype, SnbIdMap& id_map, size_t& counter_out) {
    std::ifstream f(path);
    if (!f.is_open()) { log_skip(st, label); return; }

    std::string line;
    if (!std::getline(f, line)) return;
    auto hdr = csv_fields(line);
    int c_id = col_of(hdr,"id"), c_name = col_of(hdr,"name"), c_url = col_of(hdr,"url");
    if (c_id < 0) throw std::runtime_error(std::string(label) + " CSV missing 'id'");

    GraphBase* h  = st.engine->create_graph_handle();
    NodePropSpool* sp = (st.has_props) ? new NodePropSpool(nspool(st, label)) : nullptr;
    node_id_t ctr = 0;

    while (std::getline(f, line)) {
        if (line.empty()) continue;
        auto flds = csv_fields(line);
        int64_t   lid = std::stoll(flds[c_id]);
        node_id_t tid = MAKE_TYPED_ID(vtype, ctr++);
        id_map.insert(lid, tid);
        if (!st.dry_run) {
            node n; n.id = tid; n.in_degree = 0; n.out_degree = 0;
            h->add_node(n, false);
        }
        if (sp) {
            uint8_t buf[SNBTagSchema::TOTAL_SIZE] = {};
            SNBTagSchema::set_name(buf, safe_field(flds,c_name).c_str());
            SNBTagSchema::set_url (buf, safe_field(flds,c_url).c_str());
            sp->write(tid, buf, SNBTagSchema::TOTAL_SIZE);
        }
    }

    counter_out = ctr;
    id_map.sort();
    if (sp) { sp->close(); delete sp; }
    if (!st.dry_run) h->close(false);
    log_done(st, label, ctr);
}

static void load_tags(State& st) {
    load_tag_vertex(st, st.sta+"/tag_0_0.csv", "tag",
                    VT_TAG, st.tag_map, st.n_tag);
}
static void load_tagclasses(State& st) {
    load_tag_vertex(st, st.sta+"/tagclass_0_0.csv", "tagclass",
                    VT_TAGCLASS, st.tagclass_map, st.n_tagclass);
}

static void load_places(State& st) {
    const std::string path = st.sta + "/place_0_0.csv";
    std::ifstream f(path);
    if (!f.is_open()) { log_skip(st, "places"); return; }

    std::string line;
    if (!std::getline(f, line)) return;
    auto hdr = csv_fields(line);
    int c_id   = col_of(hdr,"id"), c_name = col_of(hdr,"name");
    int c_url  = col_of(hdr,"url"), c_type = col_of(hdr,"type");
    if (c_id < 0) throw std::runtime_error("place CSV missing 'id'");

    GraphBase* h  = st.engine->create_graph_handle();
    NodePropSpool* sp = (st.has_props) ? new NodePropSpool(nspool(st,"place")) : nullptr;
    node_id_t ctr_city = 0, ctr_country = 0, ctr_cont = 0;

    while (std::getline(f, line)) {
        if (line.empty()) continue;
        auto flds = csv_fields(line);
        int64_t lid = std::stoll(flds[c_id]);
        const std::string& type_str = safe_field(flds, c_type);

        int vtype; int8_t pt; node_id_t* ctr_ptr;
        if      (type_str == "City")      { vtype=VT_CITY;      pt=SNBPlaceSchema::TYPE_CITY;      ctr_ptr=&ctr_city; }
        else if (type_str == "Country")   { vtype=VT_COUNTRY;   pt=SNBPlaceSchema::TYPE_COUNTRY;   ctr_ptr=&ctr_country; }
        else                              { vtype=VT_CONTINENT; pt=SNBPlaceSchema::TYPE_CONTINENT; ctr_ptr=&ctr_cont; }

        node_id_t tid = MAKE_TYPED_ID(vtype, (*ctr_ptr)++);
        st.place_map.insert(lid, tid);
        if (!st.dry_run) {
            node n; n.id = tid; n.in_degree = 0; n.out_degree = 0;
            h->add_node(n, false);
        }
        if (sp) {
            uint8_t buf[SNBPlaceSchema::TOTAL_SIZE] = {};
            SNBPlaceSchema::set_name(buf, safe_field(flds,c_name).c_str());
            SNBPlaceSchema::set_url (buf, safe_field(flds,c_url).c_str());
            SNBPlaceSchema::set_place_type(buf, pt);
            sp->write(tid, buf, SNBPlaceSchema::TOTAL_SIZE);
        }
    }

    st.n_city = ctr_city; st.n_country = ctr_country; st.n_continent = ctr_cont;
    st.place_map.sort();
    if (sp) { sp->close(); delete sp; }
    if (!st.dry_run) h->close(false);
    log_done(st, "places", ctr_city + ctr_country + ctr_cont);
}

static void load_organisations(State& st) {
    const std::string path = st.sta + "/organisation_0_0.csv";
    std::ifstream f(path);
    if (!f.is_open()) { log_skip(st, "organisations"); return; }

    std::string line;
    if (!std::getline(f, line)) return;
    auto hdr = csv_fields(line);
    int c_id   = col_of(hdr,"id"),   c_type = col_of(hdr,"type");
    int c_name = col_of(hdr,"name"), c_url  = col_of(hdr,"url");
    if (c_id < 0) throw std::runtime_error("organisation CSV missing 'id'");

    GraphBase* h  = st.engine->create_graph_handle();
    NodePropSpool* sp = (st.has_props) ? new NodePropSpool(nspool(st,"org")) : nullptr;
    node_id_t ctr_co = 0, ctr_uni = 0;

    while (std::getline(f, line)) {
        if (line.empty()) continue;
        auto flds = csv_fields(line);
        int64_t lid = std::stoll(flds[c_id]);
        const std::string& type_str = safe_field(flds, c_type);

        int vtype; int8_t ot; node_id_t* ctr_ptr;
        if (type_str == "company") { vtype=VT_COMPANY;    ot=SNBOrganisationSchema::TYPE_COMPANY;    ctr_ptr=&ctr_co; }
        else                       { vtype=VT_UNIVERSITY; ot=SNBOrganisationSchema::TYPE_UNIVERSITY; ctr_ptr=&ctr_uni; }

        node_id_t tid = MAKE_TYPED_ID(vtype, (*ctr_ptr)++);
        st.org_map.insert(lid, tid);
        if (!st.dry_run) {
            node n; n.id = tid; n.in_degree = 0; n.out_degree = 0;
            h->add_node(n, false);
        }
        if (sp) {
            uint8_t buf[SNBOrganisationSchema::TOTAL_SIZE] = {};
            SNBOrganisationSchema::set_name(buf, safe_field(flds,c_name).c_str());
            SNBOrganisationSchema::set_url (buf, safe_field(flds,c_url).c_str());
            SNBOrganisationSchema::set_org_type(buf, ot);
            sp->write(tid, buf, SNBOrganisationSchema::TOTAL_SIZE);
        }
    }

    st.n_company = ctr_co; st.n_university = ctr_uni;
    st.org_map.sort();
    if (sp) { sp->close(); delete sp; }
    if (!st.dry_run) h->close(false);
    log_done(st, "organisations", ctr_co + ctr_uni);
}

// Forums: must run after person_map is complete (embeds moderator typed ID).
static void load_forums(State& st) {
    // Step 1: build moderator map from hasModerator CSV
    // (forum_ldbc_id → person_ldbc_id)
    std::unordered_map<int64_t, int64_t> mod_map;
    {
        const std::string mpath = st.dyn + "/forum_hasModerator_person_0_0.csv";
        std::ifstream mf(mpath);
        if (mf.is_open()) {
            std::string line;
            std::getline(mf, line);               // skip header
            auto hdr = csv_fields(line);
            int c_forum = col_of(hdr,"Forum.id"), c_person = col_of(hdr,"Person.id");
            if (c_forum < 0) c_forum = 0;         // fallback: col 0
            if (c_person < 0) c_person = 1;
            while (std::getline(mf, line)) {
                if (line.empty()) continue;
                auto flds = csv_fields(line);
                mod_map[std::stoll(flds[c_forum])] = std::stoll(flds[c_person]);
            }
        }
    }

    // Step 2: load forum.csv
    const std::string path = st.dyn + "/forum_0_0.csv";
    std::ifstream f(path);
    if (!f.is_open()) { log_skip(st, "forums"); return; }

    std::string line;
    if (!std::getline(f, line)) return;
    auto hdr = csv_fields(line);
    int c_id    = col_of(hdr,"id"),    c_title = col_of(hdr,"title");
    int c_cdate = col_of(hdr,"creationDate");
    if (c_id < 0) throw std::runtime_error("forum CSV missing 'id'");

    GraphBase* h  = st.engine->create_graph_handle();
    NodePropSpool* sp = (st.has_props) ? new NodePropSpool(nspool(st,"forum")) : nullptr;
    node_id_t ctr = 0;

    while (std::getline(f, line)) {
        if (line.empty()) continue;
        auto flds = csv_fields(line);
        int64_t   lid = std::stoll(flds[c_id]);
        node_id_t tid = MAKE_TYPED_ID(VT_FORUM, ctr++);
        st.forum_map.insert(lid, tid);
        if (!st.dry_run) {
            node n; n.id = tid; n.in_degree = 0; n.out_degree = 0;
            h->add_node(n, false);
        }
        if (sp) {
            const std::string& title = safe_field(flds, c_title);
            uint32_t bsz = (uint32_t)(SNBForumSchema::TOTAL_SIZE + title.size() + 1);
            std::vector<uint8_t> buf(bsz, 0);
            SNBForumSchema::set_creation_date(buf.data(), parse_ms(safe_field(flds,c_cdate)));
            // Embed moderator typed ID if known.
            auto mit = mod_map.find(lid);
            if (mit != mod_map.end()) {
                node_id_t mod_tid = st.person_map.lookup(mit->second);
                if (mod_tid != SnbIdMap::INVALID)
                    SNBForumSchema::set_moderator_id(buf.data(), (int64_t)mod_tid);
            }
            SNBForumSchema::set_title(buf.data(), title.c_str());
            sp->write(tid, buf.data(), bsz);
        }
    }

    st.n_forum = ctr;
    st.forum_map.sort();
    if (sp) { sp->close(); delete sp; }
    if (!st.dry_run) h->close(false);
    log_done(st, "forums", ctr);
}

// ============================================================
// Phase 2 — edge loaders
// ============================================================

// Generic structural edge loader (no property blob).
static size_t load_struct_edges(State& st,
    const std::string& path, const char* label,
    const char* src_col_name, const char* dst_col_name,
    const SnbIdMap& src_map, const SnbIdMap& dst_map)
{
    std::ifstream f(path);
    if (!f.is_open()) { log_skip(st, label); return 0; }

    std::string line;
    if (!std::getline(f, line)) return 0;
    auto hdr = csv_fields(line);
    int c_src = col_of(hdr, src_col_name);
    int c_dst = col_of(hdr, dst_col_name);
    if (c_src < 0) c_src = 0;   // fallback
    if (c_dst < 0) c_dst = 1;

    GraphBase* h = st.engine->create_graph_handle();
    size_t cnt = 0;

    while (std::getline(f, line)) {
        if (line.empty()) continue;
        auto flds = csv_fields(line);
        node_id_t src = src_map.lookup(std::stoll(flds[c_src]));
        node_id_t dst = dst_map.lookup(std::stoll(flds[c_dst]));
        if (src == SnbIdMap::INVALID || dst == SnbIdMap::INVALID) continue;
        if (!st.dry_run) {
            edge e; e.src_id = src; e.dst_id = dst; e.edge_weight = 0;
            h->add_edge(e, false);
        }
        ++cnt;
    }

    st.n_edges += cnt;
    if (!st.dry_run) h->close(false);
    log_done(st, label, cnt);
    return cnt;
}

// Edge loader for edges with a single int64 creationDate property.
static size_t load_dated_edges(State& st,
    const std::string& path, const char* label, const char* spool_name,
    const char* src_col_name, const char* dst_col_name,
    const SnbIdMap& src_map, const SnbIdMap& dst_map)
{
    std::ifstream f(path);
    if (!f.is_open()) { log_skip(st, label); return 0; }

    std::string line;
    if (!std::getline(f, line)) return 0;
    auto hdr = csv_fields(line);
    int c_src  = col_of(hdr, src_col_name);
    int c_dst  = col_of(hdr, dst_col_name);
    int c_date = col_of(hdr, "creationDate");
    if (c_src < 0) c_src = 0;
    if (c_dst < 0) c_dst = 1;

    GraphBase*    h  = st.engine->create_graph_handle();
    EdgePropSpool* sp = (st.has_props) ? new EdgePropSpool(espool(st, spool_name)) : nullptr;
    size_t cnt = 0;

    while (std::getline(f, line)) {
        if (line.empty()) continue;
        auto flds = csv_fields(line);
        node_id_t src = src_map.lookup(std::stoll(flds[c_src]));
        node_id_t dst = dst_map.lookup(std::stoll(flds[c_dst]));
        if (src == SnbIdMap::INVALID || dst == SnbIdMap::INVALID) continue;
        if (!st.dry_run) {
            edge e; e.src_id = src; e.dst_id = dst; e.edge_weight = 0;
            h->add_edge(e, false);
        }
        if (sp) {
            // All dated edge schemas (knows, likes, hasMember) share the same
            // 8-byte layout: | int64 creationDate |.
            uint8_t buf[8] = {};
            int64_t ms = parse_ms(safe_field(flds, c_date));
            std::memcpy(buf, &ms, sizeof(ms));
            sp->write(src, dst, buf, 8);
        }
        ++cnt;
    }

    st.n_edges += cnt;
    if (sp) { sp->close(); delete sp; }
    if (!st.dry_run) h->close(false);
    log_done(st, label, cnt);
    return cnt;
}

// Edge loader for studyAt (classYear int32) and workAt (workFrom int32).
static size_t load_year_edges(State& st,
    const std::string& path, const char* label, const char* spool_name,
    const char* src_col_name, const char* dst_col_name, const char* year_col_name,
    const SnbIdMap& src_map, const SnbIdMap& dst_map)
{
    std::ifstream f(path);
    if (!f.is_open()) { log_skip(st, label); return 0; }

    std::string line;
    if (!std::getline(f, line)) return 0;
    auto hdr = csv_fields(line);
    int c_src  = col_of(hdr, src_col_name);
    int c_dst  = col_of(hdr, dst_col_name);
    int c_year = col_of(hdr, year_col_name);
    if (c_src < 0) c_src = 0;
    if (c_dst < 0) c_dst = 1;

    GraphBase*    h  = st.engine->create_graph_handle();
    EdgePropSpool* sp = (st.has_props) ? new EdgePropSpool(espool(st, spool_name)) : nullptr;
    size_t cnt = 0;

    while (std::getline(f, line)) {
        if (line.empty()) continue;
        auto flds = csv_fields(line);
        node_id_t src = src_map.lookup(std::stoll(flds[c_src]));
        node_id_t dst = dst_map.lookup(std::stoll(flds[c_dst]));
        if (src == SnbIdMap::INVALID || dst == SnbIdMap::INVALID) continue;
        if (!st.dry_run) {
            edge e; e.src_id = src; e.dst_id = dst; e.edge_weight = 0;
            h->add_edge(e, false);
        }
        if (sp) {
            uint8_t buf[4] = {};
            int32_t yr = 0;
            const std::string& ys = safe_field(flds, c_year);
            if (!ys.empty()) yr = std::stoi(ys);
            std::memcpy(buf, &yr, sizeof(yr));
            sp->write(src, dst, buf, 4);
        }
        ++cnt;
    }

    st.n_edges += cnt;
    if (sp) { sp->close(); delete sp; }
    if (!st.dry_run) h->close(false);
    log_done(st, label, cnt);
    return cnt;
}

// ============================================================
// Phase 3 — flush node prop spools
// ============================================================

static void flush_node_spool(State& st, const std::string& spool_path) {
    NodePropSpoolReader rdr(spool_path);
    GraphBase* h = st.engine->create_graph_handle();
    node_id_t id; std::vector<uint8_t> data;
    while (rdr.read(id, data)) {
        if (!st.dry_run)
            h->set_node_properties(id, data.data(), data.size());
    }
    if (!st.dry_run) h->close(false);
}

// ============================================================
// Phase 4 — flush edge prop spools
// ============================================================

static void flush_edge_spool(State& st, const std::string& spool_path) {
    EdgePropSpoolReader rdr(spool_path);
    GraphBase* h = st.engine->create_graph_handle();
    node_id_t src, dst; std::vector<uint8_t> data;
    while (rdr.read(src, dst, data)) {
        if (!st.dry_run)
            h->set_edge_properties(src, dst, data.data(), data.size());
    }
    if (!st.dry_run) h->close(false);
}

// ============================================================
// Phase 5 — secondary multi-valued tables (person_email, person_speaks)
// ============================================================

static void load_person_emails(State& st) {
    const std::string path = st.dyn + "/person_email_emailaddress_0_0.csv";
    std::ifstream f(path);
    if (!f.is_open()) { log_skip(st, "person_emails"); return; }

    std::string line;
    if (!std::getline(f, line)) return;
    auto hdr = csv_fields(line);
    int c_pid = col_of(hdr,"Person.id"), c_email = col_of(hdr,"email");
    if (c_pid   < 0) c_pid   = 0;
    if (c_email < 0) c_email = 1;

    GraphBase* h = st.engine->create_graph_handle();
    std::unordered_map<node_id_t, uint64_t> idx_map;
    size_t cnt = 0;

    while (std::getline(f, line)) {
        if (line.empty()) continue;
        auto flds = csv_fields(line);
        node_id_t pid = st.person_map.lookup(std::stoll(flds[c_pid]));
        if (pid == SnbIdMap::INVALID) continue;
        if (!st.dry_run)
            h->add_person_email(pid, idx_map[pid]++, safe_field(flds,c_email).c_str());
        ++cnt;
    }

    if (!st.dry_run) h->close(false);
    log_done(st, "person_emails", cnt);
}

static void load_person_speaks(State& st) {
    const std::string path = st.dyn + "/person_speaks_language_0_0.csv";
    std::ifstream f(path);
    if (!f.is_open()) { log_skip(st, "person_languages"); return; }

    std::string line;
    if (!std::getline(f, line)) return;
    auto hdr = csv_fields(line);
    int c_pid = col_of(hdr,"Person.id"), c_lang = col_of(hdr,"language");
    if (c_pid  < 0) c_pid  = 0;
    if (c_lang < 0) c_lang = 1;

    GraphBase* h = st.engine->create_graph_handle();
    std::unordered_map<node_id_t, uint64_t> idx_map;
    size_t cnt = 0;

    while (std::getline(f, line)) {
        if (line.empty()) continue;
        auto flds = csv_fields(line);
        node_id_t pid = st.person_map.lookup(std::stoll(flds[c_pid]));
        if (pid == SnbIdMap::INVALID) continue;
        if (!st.dry_run)
            h->add_person_language(pid, idx_map[pid]++, safe_field(flds,c_lang).c_str());
        ++cnt;
    }

    if (!st.dry_run) h->close(false);
    log_done(st, "person_languages", cnt);
}

// ============================================================
// Thread pool helper: run tasks in batches of num_threads
// ============================================================

static void run_parallel(std::vector<std::function<void()>>& tasks, int num_threads) {
    for (size_t i = 0; i < tasks.size(); i += (size_t)num_threads) {
        size_t end = std::min(i + (size_t)num_threads, tasks.size());
        std::vector<std::thread> threads;
        threads.reserve(end - i);
        for (size_t j = i; j < end; j++)
            threads.emplace_back(tasks[j]);
        for (auto& t : threads) t.join();
    }
}

// ============================================================
// Run all phases
// ============================================================

static void run_phases(State& st) {
    auto t0 = std::chrono::steady_clock::now();
    auto elapsed = [&]() {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    };

    // ---- Phase 1a + 1b: all non-forum vertices in parallel ----
    fprintf(stderr, "\n[BULK] === Phase 1: load vertices ===\n");
    {
        std::vector<std::function<void()>> tasks = {
            [&]{ try { load_places      (st); } catch(...){ st.capture(); } },
            [&]{ try { load_organisations(st); } catch(...){ st.capture(); } },
            [&]{ try { load_tags        (st); } catch(...){ st.capture(); } },
            [&]{ try { load_tagclasses  (st); } catch(...){ st.capture(); } },
            [&]{ try { load_persons     (st); } catch(...){ st.capture(); } },
            [&]{ try { load_posts       (st); } catch(...){ st.capture(); } },
            [&]{ try { load_comments    (st); } catch(...){ st.capture(); } },
        };
        run_parallel(tasks, st.num_threads);
        st.rethrow();
    }

    // ---- Phase 1c: forums (sequential; needs person_map) ----
    try { load_forums(st); } catch(const std::exception& e) {
        fprintf(stderr, "[BULK] forums: skip (%s)\n", e.what());
    }

    fprintf(stderr, "[BULK] Phase 1 done (%.1f s) — %zu persons, %zu posts, "
            "%zu comments, %zu forums, %zu tags, %zu tagclasses, "
            "%zu places, %zu orgs\n",
            elapsed(),
            st.n_person, st.n_post, st.n_comment, st.n_forum,
            st.n_tag, st.n_tagclass,
            st.n_city + st.n_country + st.n_continent,
            st.n_company + st.n_university);

    // ---- Phase 2: all edge CSVs in parallel ----
    fprintf(stderr, "\n[BULK] === Phase 2: load edges ===\n");
    {
        // Static structural edges (no props)
        std::vector<std::function<void()>> tasks = {
            [&]{ try { load_struct_edges(st, st.sta+"/place_isPartOf_place_0_0.csv",
                    "place_isPartOf_place", "Place.id","Place.id",
                    st.place_map, st.place_map); } catch(...){ st.capture(); } },
            [&]{ try { load_struct_edges(st, st.sta+"/organisation_isLocatedIn_place_0_0.csv",
                    "org_isLocatedIn_place", "Organisation.id","Place.id",
                    st.org_map, st.place_map); } catch(...){ st.capture(); } },
            [&]{ try { load_struct_edges(st, st.sta+"/tag_hasType_tagclass_0_0.csv",
                    "tag_hasType_tagclass", "Tag.id","TagClass.id",
                    st.tag_map, st.tagclass_map); } catch(...){ st.capture(); } },
            [&]{ try { load_struct_edges(st, st.sta+"/tagclass_isSubclassOf_tagclass_0_0.csv",
                    "tagclass_isSubclassOf", "TagClass.id","TagClass.id",
                    st.tagclass_map, st.tagclass_map); } catch(...){ st.capture(); } },
            // Dynamic structural edges
            [&]{ try { load_struct_edges(st, st.dyn+"/post_hasCreator_person_0_0.csv",
                    "post_hasCreator_person", "Post.id","Person.id",
                    st.post_map, st.person_map); } catch(...){ st.capture(); } },
            [&]{ try { load_struct_edges(st, st.dyn+"/comment_hasCreator_person_0_0.csv",
                    "comment_hasCreator_person", "Comment.id","Person.id",
                    st.comment_map, st.person_map); } catch(...){ st.capture(); } },
            [&]{ try { load_struct_edges(st, st.dyn+"/comment_replyOf_post_0_0.csv",
                    "comment_replyOf_post", "Comment.id","Post.id",
                    st.comment_map, st.post_map); } catch(...){ st.capture(); } },
            [&]{ try { load_struct_edges(st, st.dyn+"/comment_replyOf_comment_0_0.csv",
                    "comment_replyOf_comment", "Comment.id","Comment.id",
                    st.comment_map, st.comment_map); } catch(...){ st.capture(); } },
            [&]{ try { load_struct_edges(st, st.dyn+"/forum_containerOf_post_0_0.csv",
                    "forum_containerOf_post", "Forum.id","Post.id",
                    st.forum_map, st.post_map); } catch(...){ st.capture(); } },
            [&]{ try { load_struct_edges(st, st.dyn+"/post_hasTag_tag_0_0.csv",
                    "post_hasTag_tag", "Post.id","Tag.id",
                    st.post_map, st.tag_map); } catch(...){ st.capture(); } },
            [&]{ try { load_struct_edges(st, st.dyn+"/comment_hasTag_tag_0_0.csv",
                    "comment_hasTag_tag", "Comment.id","Tag.id",
                    st.comment_map, st.tag_map); } catch(...){ st.capture(); } },
            [&]{ try { load_struct_edges(st, st.dyn+"/forum_hasTag_tag_0_0.csv",
                    "forum_hasTag_tag", "Forum.id","Tag.id",
                    st.forum_map, st.tag_map); } catch(...){ st.capture(); } },
            [&]{ try { load_struct_edges(st, st.dyn+"/person_hasInterest_tag_0_0.csv",
                    "person_hasInterest_tag", "Person.id","Tag.id",
                    st.person_map, st.tag_map); } catch(...){ st.capture(); } },
            [&]{ try { load_struct_edges(st, st.dyn+"/person_isLocatedIn_place_0_0.csv",
                    "person_isLocatedIn_place", "Person.id","Place.id",
                    st.person_map, st.place_map); } catch(...){ st.capture(); } },
            [&]{ try { load_struct_edges(st, st.dyn+"/post_isLocatedIn_place_0_0.csv",
                    "post_isLocatedIn_place", "Post.id","Place.id",
                    st.post_map, st.place_map); } catch(...){ st.capture(); } },
            [&]{ try { load_struct_edges(st, st.dyn+"/comment_isLocatedIn_place_0_0.csv",
                    "comment_isLocatedIn_place", "Comment.id","Place.id",
                    st.comment_map, st.place_map); } catch(...){ st.capture(); } },
            // Property-bearing edges
            [&]{ try { load_dated_edges(st, st.dyn+"/person_knows_person_0_0.csv",
                    "person_knows_person", "knows",
                    "Person.id","Person.id",
                    st.person_map, st.person_map); } catch(...){ st.capture(); } },
            [&]{ try { load_dated_edges(st, st.dyn+"/forum_hasMember_person_0_0.csv",
                    "forum_hasMember_person", "hasmember",
                    "Forum.id","Person.id",
                    st.forum_map, st.person_map); } catch(...){ st.capture(); } },
            [&]{ try { load_dated_edges(st, st.dyn+"/person_likes_post_0_0.csv",
                    "person_likes_post", "likes_post",
                    "Person.id","Post.id",
                    st.person_map, st.post_map); } catch(...){ st.capture(); } },
            [&]{ try { load_dated_edges(st, st.dyn+"/person_likes_comment_0_0.csv",
                    "person_likes_comment", "likes_comment",
                    "Person.id","Comment.id",
                    st.person_map, st.comment_map); } catch(...){ st.capture(); } },
            [&]{ try { load_year_edges(st, st.dyn+"/person_studyAt_organisation_0_0.csv",
                    "person_studyAt", "studyat",
                    "Person.id","Organisation.id","classYear",
                    st.person_map, st.org_map); } catch(...){ st.capture(); } },
            [&]{ try { load_year_edges(st, st.dyn+"/person_workAt_organisation_0_0.csv",
                    "person_workAt", "workat",
                    "Person.id","Organisation.id","workFrom",
                    st.person_map, st.org_map); } catch(...){ st.capture(); } },
        };
        run_parallel(tasks, st.num_threads);
        st.rethrow();
    }
    fprintf(stderr, "[BULK] Phase 2 done (%.1f s) — %zu edges total\n",
            elapsed(), st.n_edges.load());

    // ---- Phase 3: flush node props ----
    // Must run AFTER Phase 2 (SplitEdgeKey sentinel layout requires
    // set_node_properties to be called after all add_edge calls).
    if (st.has_props) {
        fprintf(stderr, "\n[BULK] === Phase 3: flush node props ===\n");
        std::vector<std::function<void()>> tasks = {
            [&]{ flush_node_spool(st, nspool(st,"person"));   log_done(st,"flush person props"); },
            [&]{ flush_node_spool(st, nspool(st,"post"));     log_done(st,"flush post props"); },
            [&]{ flush_node_spool(st, nspool(st,"comment"));  log_done(st,"flush comment props"); },
            [&]{ flush_node_spool(st, nspool(st,"forum"));    log_done(st,"flush forum props"); },
            [&]{ flush_node_spool(st, nspool(st,"tag"));      log_done(st,"flush tag props"); },
            [&]{ flush_node_spool(st, nspool(st,"tagclass")); log_done(st,"flush tagclass props"); },
            [&]{ flush_node_spool(st, nspool(st,"place"));    log_done(st,"flush place props"); },
            [&]{ flush_node_spool(st, nspool(st,"org"));      log_done(st,"flush org props"); },
        };
        run_parallel(tasks, st.num_threads);
        fprintf(stderr, "[BULK] Phase 3 done (%.1f s)\n", elapsed());
    }

    // ---- Phase 4: flush edge props ----
    if (st.has_props) {
        fprintf(stderr, "\n[BULK] === Phase 4: flush edge props ===\n");
        std::vector<std::function<void()>> tasks = {
            [&]{ flush_edge_spool(st, espool(st,"knows"));        log_done(st,"flush knows props"); },
            [&]{ flush_edge_spool(st, espool(st,"hasmember"));    log_done(st,"flush hasMember props"); },
            [&]{ flush_edge_spool(st, espool(st,"likes_post"));   log_done(st,"flush likes(post) props"); },
            [&]{ flush_edge_spool(st, espool(st,"likes_comment"));log_done(st,"flush likes(comment) props"); },
            [&]{ flush_edge_spool(st, espool(st,"studyat"));      log_done(st,"flush studyAt props"); },
            [&]{ flush_edge_spool(st, espool(st,"workat"));       log_done(st,"flush workAt props"); },
        };
        run_parallel(tasks, st.num_threads);
        fprintf(stderr, "[BULK] Phase 4 done (%.1f s)\n", elapsed());
    }

    // ---- Phase 5: secondary tables ----
    fprintf(stderr, "\n[BULK] === Phase 5: secondary tables ===\n");
    {
        std::vector<std::function<void()>> tasks = {
            [&]{ try { load_person_emails(st); } catch(...){ st.capture(); } },
            [&]{ try { load_person_speaks(st); } catch(...){ st.capture(); } },
        };
        run_parallel(tasks, st.num_threads);
        st.rethrow();
    }
    fprintf(stderr, "[BULK] Phase 5 done (%.1f s)\n", elapsed());

    // ---- Cleanup spool files ----
    if (!st.keep_spool && st.has_props) {
        for (const char* name : {"person","post","comment","forum","tag","tagclass","place","org"})
            std::remove(nspool(st, name).c_str());
        for (const char* name : {"knows","hasmember","likes_post","likes_comment","studyat","workat"})
            std::remove(espool(st, name).c_str());
        if (st.verbose) fprintf(stderr, "[BULK] spool files removed\n");
    }

    fprintf(stderr, "\n[BULK] Total load time: %.1f s\n", elapsed());
}

// ============================================================
// main
// ============================================================

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr,
            "Usage: %s <data_dir> <db_dir> [adj|splitekey]\n"
            "          [--threads N]    (default 8)\n"
            "          [--cache N]      (WT cache GB, default 4)\n"
            "          [--spool DIR]    (temp dir, default <db_dir>/spool)\n"
            "          [--embedded]     (EMBEDDED prop mode)\n"
            "          [--no-props]     (topology only)\n"
            "          [--dry-run]      (no WT writes)\n"
            "          [--keep-spool]   (keep spool files after load)\n",
            argv[0]);
        return 1;
    }

    std::string data_dir = argv[1];
    std::string db_dir   = argv[2];

    GraphType gtype      = GraphType::SplitEKey;
    int       num_threads = 8;
    int       cache_gb    = 4;
    std::string spool_dir;
    PropStorageMode prop_mode = COLUMNAR;
    bool has_props  = true;
    bool dry_run    = false;
    bool keep_spool = false;

    for (int i = 3; i < argc; i++) {
        std::string a = argv[i];
        if      (a == "adj")          gtype       = GraphType::Adj;
        else if (a == "splitekey")    gtype       = GraphType::SplitEKey;
        else if (a == "--threads" && i+1 < argc) num_threads = std::stoi(argv[++i]);
        else if (a == "--cache"   && i+1 < argc) cache_gb    = std::stoi(argv[++i]);
        else if (a == "--spool"   && i+1 < argc) spool_dir   = argv[++i];
        else if (a == "--embedded")   prop_mode   = EMBEDDED;
        else if (a == "--no-props")   has_props   = false;
        else if (a == "--dry-run")    dry_run     = true;
        else if (a == "--keep-spool") keep_spool  = true;
        else { fprintf(stderr, "Unknown argument: %s\n", a.c_str()); return 1; }
    }

    if (spool_dir.empty()) spool_dir = db_dir + "/spool";
    mkdir(db_dir.c_str(),    0755);
    mkdir(spool_dir.c_str(), 0755);

    // Build graph_opts
    graph_opts gopts;
    gopts.create_new      = true;
    gopts.optimize_create = false;  // parallel inserts cannot guarantee sorted key order
    gopts.is_directed     = true;
    gopts.read_optimize   = true;
    gopts.is_weighted     = false;
    gopts.type            = gtype;
    gopts.db_dir          = db_dir;
    gopts.db_name         = "snb";
    gopts.has_node_props  = has_props;
    gopts.has_edge_props  = has_props;
    gopts.prop_mode       = has_props ? prop_mode : EMBEDDED;
    gopts.num_threads     = num_threads;
    gopts.conn_config     = "cache_size=" + std::to_string(cache_gb) + "GB,session_max=128";

    fprintf(stderr, "[BULK] data_dir:    %s\n", data_dir.c_str());
    fprintf(stderr, "[BULK] db_dir:      %s\n", db_dir.c_str());
    fprintf(stderr, "[BULK] spool_dir:   %s\n", spool_dir.c_str());
    fprintf(stderr, "[BULK] graph_type:  %s\n", gtype == GraphType::Adj ? "adj" : "splitekey");
    fprintf(stderr, "[BULK] prop_mode:   %s\n", !has_props ? "none" : (prop_mode == COLUMNAR ? "columnar" : "embedded"));
    fprintf(stderr, "[BULK] threads:     %d\n", num_threads);
    fprintf(stderr, "[BULK] cache:       %d GB\n", cache_gb);
    fprintf(stderr, "[BULK] dry_run:     %s\n", dry_run ? "yes" : "no");

    // Create GraphEngine (initialises WT connection and creates all tables)
    GraphEngine* engine = nullptr;
    if (!dry_run) {
        engine = new GraphEngine(num_threads, gopts);
    }

    // Build LoaderState
    State st;
    st.dyn         = data_dir + "/dynamic";
    st.sta         = data_dir + "/static";
    st.spool_dir   = spool_dir;
    st.num_threads = num_threads;
    st.has_props   = has_props;
    st.dry_run     = dry_run;
    st.keep_spool  = keep_spool;
    st.engine      = engine;
    st.gopts       = gopts;

    // Run all phases
    try {
        run_phases(st);
    } catch (const std::exception& e) {
        fprintf(stderr, "[BULK] FATAL: %s\n", e.what());
        if (engine) engine->close_graph();
        delete engine;
        return 1;
    }

    // Checkpoint and close
    if (!dry_run && engine) {
        GraphBase* final_h = engine->create_graph_handle();
        final_h->close(true);   // true = synchronize / checkpoint
        engine->close_graph();
    }
    delete engine;

    fprintf(stderr, "\n[BULK] Load complete.\n");
    return 0;
}
