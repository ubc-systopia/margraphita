#ifndef LDBC_SNB_LOADER_H
#define LDBC_SNB_LOADER_H

// LDBC SNB SF-scale CSV loader for Flexograph.
//
// Loads the subset: Person + Post vertices, knows + hasCreator + likes edges.
//
// Typed vertex ID scheme (bit-reservation):
//   Person IDs → MAKE_TYPED_ID(VT_PERSON, counter)  [0x0000…, 0x0100…)
//   Post   IDs → MAKE_TYPED_ID(VT_POST,   counter)  [0x0100…, 0x0200…)
//
// Loading order requirement (critical for SplitEdgeKey):
//   1. load_persons / load_posts  → add_node + enqueue prop write
//   2. load_knows / load_has_creator / load_likes → add_edge
//   3. flush_node_props()         → set_node_properties for all nodes
//   4. flush_edge_props()         → set_edge_properties for all edges with props
//
// Rationale: add_node_txn (SplitEdgeKey) writes a sentinel value to the node
// row on every call. If set_node_properties were called before all add_edge
// calls, a subsequent add_node for a degree-update might overwrite the prop
// bytes stored in the sentinel. Flushing after all structural inserts avoids
// this hazard.

#include <cassert>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdio>

#include "common_defs.h"
#include "graph.h"
#include "prop_schema.h"

// ---- Pending property writes -------------------------------------------------

struct PendingNodeProp {
    node_id_t id;
    std::vector<uint8_t> data;  // serialized blob
};

struct PendingEdgeProp {
    node_id_t src;
    node_id_t dst;
    std::vector<uint8_t> data;  // serialized blob (empty → no edge props)
};

// ---- CSV helpers -------------------------------------------------------------

// Split a line on '|' (LDBC default delimiter).
static std::vector<std::string> csv_split(const std::string &line, char delim = '|')
{
    std::vector<std::string> fields;
    std::istringstream ss(line);
    std::string token;
    while (std::getline(ss, token, delim))
        fields.push_back(token);
    return fields;
}

// Parse an LDBC LongDateFormatter timestamp: raw epoch milliseconds as a decimal string.
static int64_t parse_epoch_ms(const std::string &s)
{
    if (s.empty()) return 0;
    return std::stoll(s);
}

// ---- LDBCLoader -------------------------------------------------------------

struct LDBCLoader {
    GraphBase *graph;
    graph_opts opts;  // copy for has_node_props / has_edge_props checks

    node_id_t person_count = 0;   // counter only (for logging / assertions)
    node_id_t post_count   = 0;   // counter only (for logging / assertions)

    // LDBC original ID → compact Flexograph node_id_t
    std::unordered_map<int64_t, node_id_t> person_id_map;
    std::unordered_map<int64_t, node_id_t> post_id_map;

    std::vector<PendingNodeProp> pending_node_props;
    std::vector<PendingEdgeProp> pending_edge_props;

    // Optional insertion log: records every node/edge about to be inserted.
    // Format: "NODE <id>" or "EDGE <src> <dst>", one per line.
    // Enable via enable_insertion_log(path).
    // Set dry_run = true to parse CSVs and build the log without touching the DB.
    FILE *insertion_log_fp = nullptr;
    bool dry_run = false;

    explicit LDBCLoader(GraphBase *g, const graph_opts &o) : graph(g), opts(o) {}

    void enable_insertion_log(const std::string &path) {
        insertion_log_fp = std::fopen(path.c_str(), "w");
        if (!insertion_log_fp)
            throw std::runtime_error("Cannot open insertion log: " + path);
    }

    ~LDBCLoader() {
        if (insertion_log_fp) std::fclose(insertion_log_fp);
    }

private:
    void log_node(node_id_t id) {
        if (insertion_log_fp)
            std::fprintf(insertion_log_fp, "NODE %llu\n", (unsigned long long)id);
    }
    void log_edge(node_id_t src, node_id_t dst) {
        if (insertion_log_fp)
            std::fprintf(insertion_log_fp, "EDGE %llu %llu\n",
                         (unsigned long long)src, (unsigned long long)dst);
    }
public:

    // ------------------------------------------------------------------
    // Phase 1: load vertices (add_node + enqueue prop write)
    // ------------------------------------------------------------------

    // person_0_0.csv expected columns (header line, '|' delimited):
    //   id|firstName|lastName|gender|birthday|creationDate|locationIP|browserUsed|...
    // We only parse: id, gender (m/f → 0/1), birthday, creationDate
    void load_persons(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open())
            throw std::runtime_error("Cannot open: " + path);

        std::string line;
        // Skip header
        if (!std::getline(f, line))
            return;

        // Detect column positions from header
        auto hdr = csv_split(line);
        int col_id = -1, col_gender = -1, col_birthday = -1, col_creation = -1;
        for (int i = 0; i < (int)hdr.size(); i++) {
            if (hdr[i] == "id")           col_id       = i;
            else if (hdr[i] == "gender")  col_gender   = i;
            else if (hdr[i] == "birthday")col_birthday = i;
            else if (hdr[i] == "creationDate") col_creation = i;
        }
        if (col_id < 0)
            throw std::runtime_error("person CSV missing 'id' column");

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);

            int64_t ldbc_id = std::stoll(fields[col_id]);
            node_id_t fid = MAKE_TYPED_ID(VT_PERSON, person_count++);
            person_id_map[ldbc_id] = fid;

            node n;
            n.id = fid;
            log_node(fid);
            if (!dry_run) {
                graph->add_node(n, false);
            }

            if (!dry_run && opts.has_node_props) {
                uint8_t buf[SNBPersonSchema::TOTAL_SIZE] = {};
                if (col_creation >= 0 && col_creation < (int)fields.size())
                    SNBPersonSchema::set_creation_date(buf, parse_epoch_ms(fields[col_creation]));
                if (col_birthday >= 0 && col_birthday < (int)fields.size())
                    SNBPersonSchema::set_birthday(buf, parse_epoch_ms(fields[col_birthday]));
                if (col_gender >= 0 && col_gender < (int)fields.size()) {
                    // gender: "male" → 0, "female" → 1
                    int8_t g = (fields[col_gender] == "female") ? 1 : 0;
                    SNBPersonSchema::set_gender(buf, g);
                }
                PendingNodeProp p;
                p.id = fid;
                p.data.assign(buf, buf + SNBPersonSchema::TOTAL_SIZE);
                pending_node_props.push_back(std::move(p));
            }
        }
    }

    // post_0_0.csv expected columns (header '|' delimited):
    //   id|imageFile|creationDate|locationIP|browserUsed|language|content|length|...
    // We parse: id, creationDate, length
    void load_posts(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open())
            throw std::runtime_error("Cannot open: " + path);

        std::string line;
        if (!std::getline(f, line))
            return;

        auto hdr = csv_split(line);
        int col_id = -1, col_creation = -1, col_length = -1;
        for (int i = 0; i < (int)hdr.size(); i++) {
            if (hdr[i] == "id")                  col_id       = i;
            else if (hdr[i] == "creationDate")    col_creation = i;
            else if (hdr[i] == "length")          col_length   = i;
        }
        if (col_id < 0)
            throw std::runtime_error("post CSV missing 'id' column");

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);

            int64_t ldbc_id = std::stoll(fields[col_id]);
            node_id_t fid = MAKE_TYPED_ID(VT_POST, post_count++);
            post_id_map[ldbc_id] = fid;

            node n;
            n.id = fid;
            log_node(fid);
            if (!dry_run) {
                graph->add_node(n, false);
            }

            if (!dry_run && opts.has_node_props) {
                uint8_t buf[SNBPostSchema::TOTAL_SIZE] = {};
                if (col_creation >= 0 && col_creation < (int)fields.size())
                    SNBPostSchema::set_creation_date(buf, parse_epoch_ms(fields[col_creation]));
                if (col_length >= 0 && col_length < (int)fields.size())
                    SNBPostSchema::set_length(buf, std::stoi(fields[col_length]));
                PendingNodeProp p;
                p.id = fid;
                p.data.assign(buf, buf + SNBPostSchema::TOTAL_SIZE);
                pending_node_props.push_back(std::move(p));
            }
        }
    }

    // ------------------------------------------------------------------
    // Phase 2: flush node props (after ALL add_node calls)
    // ------------------------------------------------------------------

    void flush_node_props()
    {
        if (!opts.has_node_props) return;
        for (auto &p : pending_node_props)
            graph->set_node_properties(p.id, p.data.data(), p.data.size());
        pending_node_props.clear();
    }

    // ------------------------------------------------------------------
    // Phase 3: load edges (add_edge + enqueue edge prop write if needed)
    // ------------------------------------------------------------------

    // person_knows_person_0_0.csv columns: Person.id|Person.id|creationDate
    void load_knows(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open())
            throw std::runtime_error("Cannot open: " + path);

        std::string line;
        if (!std::getline(f, line))
            return;

        auto hdr = csv_split(line);
        int col_src = -1, col_dst = -1, col_creation = -1;
        for (int i = 0; i < (int)hdr.size(); i++) {
            if (i == 0)                        col_src      = i;
            else if (i == 1)                   col_dst      = i;
            else if (hdr[i] == "creationDate") col_creation = i;
        }

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            if ((int)fields.size() < 2) continue;

            int64_t src_ldbc = std::stoll(fields[col_src]);
            int64_t dst_ldbc = std::stoll(fields[col_dst]);

            auto sit = person_id_map.find(src_ldbc);
            auto dit = person_id_map.find(dst_ldbc);
            if (sit == person_id_map.end() || dit == person_id_map.end())
                continue;  // skip unknown persons

            node_id_t src = sit->second;
            node_id_t dst = dit->second;

            edge e;
            e.src_id = src;
            e.dst_id = dst;
            log_edge(src, dst);
            if (!dry_run) {
                graph->add_edge(e, false);

                if (opts.has_edge_props && col_creation >= 0 && col_creation < (int)fields.size()) {
                    uint8_t buf[SNBKnowsSchema::TOTAL_SIZE] = {};
                    SNBKnowsSchema::set_creation_date(buf, parse_epoch_ms(fields[col_creation]));
                    PendingEdgeProp p;
                    p.src = src; p.dst = dst;
                    p.data.assign(buf, buf + SNBKnowsSchema::TOTAL_SIZE);
                    pending_edge_props.push_back(std::move(p));
                }
            }
        }
    }

    // post_hasCreator_person_0_0.csv columns: Post.id|Person.id
    // hasCreator edges have no properties.
    void load_has_creator(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open())
            throw std::runtime_error("Cannot open: " + path);

        std::string line;
        if (!std::getline(f, line))
            return;

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            if ((int)fields.size() < 2) continue;

            int64_t post_ldbc   = std::stoll(fields[0]);
            int64_t person_ldbc = std::stoll(fields[1]);

            auto pit = post_id_map.find(post_ldbc);
            auto ait = person_id_map.find(person_ldbc);
            if (pit == post_id_map.end() || ait == person_id_map.end())
                continue;

            edge e;
            e.src_id = pit->second;
            e.dst_id = ait->second;
            log_edge(pit->second, ait->second);
            if (!dry_run) {
                graph->add_edge(e, false);
            }
            // No edge properties for hasCreator — no pending write.
        }
    }

    // person_likes_post_0_0.csv columns: Person.id|Post.id|creationDate
    void load_likes(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open())
            throw std::runtime_error("Cannot open: " + path);

        std::string line;
        if (!std::getline(f, line))
            return;

        auto hdr = csv_split(line);
        int col_creation = -1;
        for (int i = 0; i < (int)hdr.size(); i++) {
            if (hdr[i] == "creationDate") col_creation = i;
        }

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            if ((int)fields.size() < 2) continue;

            int64_t person_ldbc = std::stoll(fields[0]);
            int64_t post_ldbc   = std::stoll(fields[1]);

            auto pit = person_id_map.find(person_ldbc);
            auto poit = post_id_map.find(post_ldbc);
            if (pit == person_id_map.end() || poit == post_id_map.end())
                continue;

            node_id_t src = pit->second;
            node_id_t dst = poit->second;

            edge e;
            e.src_id = src;
            e.dst_id = dst;
            log_edge(src, dst);
            if (!dry_run) {
                graph->add_edge(e, false);

                if (opts.has_edge_props && col_creation >= 0 && col_creation < (int)fields.size()) {
                    uint8_t buf[SNBLikesSchema::TOTAL_SIZE] = {};
                    SNBLikesSchema::set_creation_date(buf, parse_epoch_ms(fields[col_creation]));
                    PendingEdgeProp p;
                    p.src = src; p.dst = dst;
                    p.data.assign(buf, buf + SNBLikesSchema::TOTAL_SIZE);
                    pending_edge_props.push_back(std::move(p));
                }
            }
        }
    }

    // ------------------------------------------------------------------
    // Phase 4: flush edge props (after ALL add_edge calls)
    // ------------------------------------------------------------------

    void flush_edge_props()
    {
        if (!opts.has_edge_props) return;
        for (auto &p : pending_edge_props)
            graph->set_edge_properties(p.src, p.dst, p.data.data(), p.data.size());
        pending_edge_props.clear();
    }

    // Helper: is this node ID a Person?
    static bool is_person(node_id_t id) { return VTYPE_OF(id) == VT_PERSON; }
    // Helper: is this node ID a Post?
    static bool is_post(node_id_t id) { return VTYPE_OF(id) == VT_POST; }
};

#endif  // LDBC_SNB_LOADER_H
