#ifndef LDBC_SNB_LOADER_H
#define LDBC_SNB_LOADER_H

// LDBC SNB full-schema CSV loader for Flexograph.
//
// Vertex types loaded:
//   Person, Post, Comment, Forum, Tag, TagClass,
//   City, Country, Continent, Company, University
//
// Edge types loaded (structural + property-bearing):
//   knows, hasCreator(post+comment), likes(post+comment),
//   replyOf(post+comment), containerOf, hasMember, hasModerator(*),
//   hasTag(post+comment+forum), hasInterest, isLocatedIn(person/post/comment/org),
//   isPartOf, hasType, isSubclassOf, studyAt, workAt
//
// (*) hasModerator is NOT stored as a graph edge; the moderator_id field is
//     embedded in SNBForumSchema. Call load_has_moderator_map() and pass the
//     result to load_forums() to populate it.
//
// Typed vertex ID scheme:
//   Person  → MAKE_TYPED_ID(VT_PERSON,      counter)
//   Post    → MAKE_TYPED_ID(VT_POST,        counter)
//   Comment → MAKE_TYPED_ID(VT_COMMENT,     counter)
//   Forum   → MAKE_TYPED_ID(VT_FORUM,       counter)
//   Tag     → MAKE_TYPED_ID(VT_TAG,         counter)
//   TagClass→ MAKE_TYPED_ID(VT_TAGCLASS,    counter)
//   City    → MAKE_TYPED_ID(VT_CITY,        counter)
//   Country → MAKE_TYPED_ID(VT_COUNTRY,     counter)
//   Continent→MAKE_TYPED_ID(VT_CONTINENT,   counter)
//   Company → MAKE_TYPED_ID(VT_COMPANY,     counter)
//   Univ.   → MAKE_TYPED_ID(VT_UNIVERSITY,  counter)
//
// Loading order requirement (critical for SplitEdgeKey):
//   1. load all vertices  → add_node + enqueue prop write
//   2. load all edges     → add_edge (+ enqueue edge prop write if needed)
//   3. flush_node_props() → set_node_properties for all nodes
//   4. flush_edge_props() → set_edge_properties for all edges with props

#include <cassert>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdio>
#include <cstring>

#include "common_defs.h"
#include "graph.h"
#include "prop_schema.h"

// ---- Pending property writes -------------------------------------------------

struct PendingNodeProp {
    node_id_t id;
    std::vector<uint8_t> data;
};

struct PendingEdgeProp {
    node_id_t src;
    node_id_t dst;
    std::vector<uint8_t> data;
};

// ---- CSV helpers -------------------------------------------------------------

static std::vector<std::string> csv_split(const std::string &line, char delim = '|')
{
    std::vector<std::string> fields;
    std::istringstream ss(line);
    std::string token;
    while (std::getline(ss, token, delim))
        fields.push_back(token);
    return fields;
}

static int64_t parse_epoch_ms(const std::string &s)
{
    if (s.empty()) return 0;
    return std::stoll(s);
}

// ---- LDBCLoader -------------------------------------------------------------

struct LDBCLoader {
    GraphBase *graph;
    graph_opts opts;

    // Vertex counters and ID maps
    node_id_t person_count    = 0;
    node_id_t post_count      = 0;
    node_id_t comment_count   = 0;
    node_id_t forum_count     = 0;
    node_id_t tag_count       = 0;
    node_id_t tagclass_count  = 0;
    node_id_t city_count      = 0;
    node_id_t country_count   = 0;
    node_id_t continent_count = 0;
    node_id_t company_count   = 0;
    node_id_t university_count= 0;

    std::unordered_map<int64_t, node_id_t> person_id_map;
    std::unordered_map<int64_t, node_id_t> post_id_map;
    std::unordered_map<int64_t, node_id_t> comment_id_map;
    std::unordered_map<int64_t, node_id_t> forum_id_map;
    std::unordered_map<int64_t, node_id_t> tag_id_map;
    std::unordered_map<int64_t, node_id_t> tagclass_id_map;
    std::unordered_map<int64_t, node_id_t> place_id_map;        // City+Country+Continent
    std::unordered_map<int64_t, node_id_t> organisation_id_map; // Company+University

    std::vector<PendingNodeProp> pending_node_props;
    std::vector<PendingEdgeProp> pending_edge_props;

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

    // Add a structural-only edge between two already-mapped typed IDs.
    void add_structural_edge(node_id_t src, node_id_t dst) {
        log_edge(src, dst);
        if (!dry_run) {
            edge e;
            e.src_id = src;
            e.dst_id = dst;
            graph->add_edge(e, false);
        }
    }

public:

    // =========================================================================
    // Phase 1: load vertices (add_node + enqueue prop write)
    // =========================================================================

    // person_0_0.csv: id|firstName|lastName|gender|birthday|creationDate|locationIP|browserUsed|...
    void load_persons(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;

        auto hdr = csv_split(line);
        int col_id=-1, col_gender=-1, col_birthday=-1, col_creation=-1;
        int col_firstname=-1, col_lastname=-1, col_browser=-1, col_loc_ip=-1;
        for (int i = 0; i < (int)hdr.size(); i++) {
            if      (hdr[i] == "id")          col_id        = i;
            else if (hdr[i] == "gender")      col_gender    = i;
            else if (hdr[i] == "birthday")    col_birthday  = i;
            else if (hdr[i] == "creationDate")col_creation  = i;
            else if (hdr[i] == "firstName")   col_firstname = i;
            else if (hdr[i] == "lastName")    col_lastname  = i;
            else if (hdr[i] == "browserUsed") col_browser   = i;
            else if (hdr[i] == "locationIP")  col_loc_ip    = i;
        }
        if (col_id < 0) throw std::runtime_error("person CSV missing 'id' column");

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            int64_t ldbc_id = std::stoll(fields[col_id]);
            node_id_t fid = MAKE_TYPED_ID(VT_PERSON, person_count++);
            person_id_map[ldbc_id] = fid;
            log_node(fid);
            if (!dry_run) graph->add_node({fid, 0, 0}, false);
            if (!dry_run && opts.has_node_props) {
                uint8_t buf[SNBPersonSchema::TOTAL_SIZE] = {};
                if (col_creation >= 0 && col_creation < (int)fields.size())
                    SNBPersonSchema::set_creation_date(buf, parse_epoch_ms(fields[col_creation]));
                if (col_birthday >= 0 && col_birthday < (int)fields.size())
                    SNBPersonSchema::set_birthday(buf, parse_epoch_ms(fields[col_birthday]));
                if (col_gender >= 0 && col_gender < (int)fields.size())
                    SNBPersonSchema::set_gender(buf, fields[col_gender] == "female" ? 1 : 0);
                if (col_firstname >= 0 && col_firstname < (int)fields.size())
                    SNBPersonSchema::set_first_name(buf, fields[col_firstname].c_str());
                if (col_lastname >= 0 && col_lastname < (int)fields.size())
                    SNBPersonSchema::set_last_name(buf, fields[col_lastname].c_str());
                if (col_browser >= 0 && col_browser < (int)fields.size())
                    SNBPersonSchema::set_browser_used(buf, fields[col_browser].c_str());
                if (col_loc_ip >= 0 && col_loc_ip < (int)fields.size())
                    SNBPersonSchema::set_location_ip(buf, fields[col_loc_ip].c_str());
                PendingNodeProp p;
                p.id = fid;
                p.data.assign(buf, buf + SNBPersonSchema::TOTAL_SIZE);
                pending_node_props.push_back(std::move(p));
            }
        }
    }

    // post_0_0.csv: id|imageFile|creationDate|locationIP|browserUsed|language|content|length|...
    void load_posts(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;

        auto hdr = csv_split(line);
        int col_id=-1, col_creation=-1, col_length=-1, col_imagefile=-1, col_content=-1;
        for (int i = 0; i < (int)hdr.size(); i++) {
            if      (hdr[i] == "id")          col_id        = i;
            else if (hdr[i] == "creationDate")col_creation  = i;
            else if (hdr[i] == "length")      col_length    = i;
            else if (hdr[i] == "imageFile")   col_imagefile = i;
            else if (hdr[i] == "content")     col_content   = i;
        }
        if (col_id < 0) throw std::runtime_error("post CSV missing 'id' column");

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            int64_t ldbc_id = std::stoll(fields[col_id]);
            node_id_t fid = MAKE_TYPED_ID(VT_POST, post_count++);
            post_id_map[ldbc_id] = fid;
            log_node(fid);
            if (!dry_run) graph->add_node({fid, 0, 0}, false);
            if (!dry_run && opts.has_node_props) {
                int8_t tag = 0;
                std::string cstr;
                bool has_image = col_imagefile >= 0 && col_imagefile < (int)fields.size()
                                 && !fields[col_imagefile].empty();
                if (has_image) {
                    tag  = 1;
                    cstr = fields[col_imagefile];
                } else if (col_content >= 0 && col_content < (int)fields.size()) {
                    cstr = fields[col_content];
                }
                if (cstr.size() > SNBPostSchema::CONTENT_MAX_LEN)
                    cstr.resize(SNBPostSchema::CONTENT_MAX_LEN);
                size_t total = SNBPostSchema::TOTAL_SIZE + cstr.size() + 1;
                std::vector<uint8_t> buf(total, 0);
                if (col_creation >= 0 && col_creation < (int)fields.size())
                    SNBPostSchema::set_creation_date(buf.data(), parse_epoch_ms(fields[col_creation]));
                if (col_length >= 0 && col_length < (int)fields.size())
                    SNBPostSchema::set_length(buf.data(), std::stoi(fields[col_length]));
                SNBPostSchema::set_tag(buf.data(), tag);
                SNBPostSchema::set_content(buf.data(), cstr.c_str());
                PendingNodeProp p;
                p.id = fid;
                p.data = std::move(buf);
                pending_node_props.push_back(std::move(p));
            }
        }
    }

    // comment_0_0.csv: id|creationDate|locationIP|browserUsed|content|length
    void load_comments(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;

        auto hdr = csv_split(line);
        int col_id=-1, col_creation=-1, col_length=-1, col_content=-1;
        for (int i = 0; i < (int)hdr.size(); i++) {
            if      (hdr[i] == "id")          col_id       = i;
            else if (hdr[i] == "creationDate")col_creation = i;
            else if (hdr[i] == "length")      col_length   = i;
            else if (hdr[i] == "content")     col_content  = i;
        }
        if (col_id < 0) throw std::runtime_error("comment CSV missing 'id' column");

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            int64_t ldbc_id = std::stoll(fields[col_id]);
            node_id_t fid = MAKE_TYPED_ID(VT_COMMENT, comment_count++);
            comment_id_map[ldbc_id] = fid;
            log_node(fid);
            if (!dry_run) graph->add_node({fid, 0, 0}, false);
            if (!dry_run && opts.has_node_props) {
                std::string cstr;
                if (col_content >= 0 && col_content < (int)fields.size())
                    cstr = fields[col_content];
                if (cstr.size() > SNBPostSchema::CONTENT_MAX_LEN)
                    cstr.resize(SNBPostSchema::CONTENT_MAX_LEN);
                size_t total = SNBPostSchema::TOTAL_SIZE + cstr.size() + 1;
                std::vector<uint8_t> buf(total, 0);
                if (col_creation >= 0 && col_creation < (int)fields.size())
                    SNBPostSchema::set_creation_date(buf.data(), parse_epoch_ms(fields[col_creation]));
                if (col_length >= 0 && col_length < (int)fields.size())
                    SNBPostSchema::set_length(buf.data(), std::stoi(fields[col_length]));
                SNBPostSchema::set_tag(buf.data(), 0);  // comments never have image tag
                SNBPostSchema::set_content(buf.data(), cstr.c_str());
                PendingNodeProp p;
                p.id = fid;
                p.data = std::move(buf);
                pending_node_props.push_back(std::move(p));
            }
        }
    }

    // Build a map from forum_ldbc_id → person_ldbc_id from forum_hasModerator_person_0_0.csv.
    // Call this BEFORE load_forums so the moderator can be embedded in the forum blob.
    // CSV columns: Forum.id|Person.id
    static void load_has_moderator_map(const std::string &path,
                                       std::unordered_map<int64_t, int64_t> &mod_map)
    {
        std::ifstream f(path);
        if (!f.is_open()) return;  // not fatal if missing
        std::string line;
        if (!std::getline(f, line)) return;  // skip header
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            if ((int)fields.size() < 2) continue;
            int64_t forum_ldbc  = std::stoll(fields[0]);
            int64_t person_ldbc = std::stoll(fields[1]);
            mod_map[forum_ldbc] = person_ldbc;
        }
    }

    // forum_0_0.csv: id|title|creationDate
    // mod_map: forum_ldbc_id → person_ldbc_id (from load_has_moderator_map)
    void load_forums(const std::string &path,
                     const std::unordered_map<int64_t, int64_t> *mod_map = nullptr)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;

        auto hdr = csv_split(line);
        int col_id=-1, col_title=-1, col_creation=-1;
        for (int i = 0; i < (int)hdr.size(); i++) {
            if      (hdr[i] == "id")          col_id       = i;
            else if (hdr[i] == "title")       col_title    = i;
            else if (hdr[i] == "creationDate")col_creation = i;
        }
        if (col_id < 0) throw std::runtime_error("forum CSV missing 'id' column");

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            int64_t ldbc_id = std::stoll(fields[col_id]);
            node_id_t fid = MAKE_TYPED_ID(VT_FORUM, forum_count++);
            forum_id_map[ldbc_id] = fid;
            log_node(fid);
            if (!dry_run) graph->add_node({fid, 0, 0}, false);
            if (!dry_run && opts.has_node_props) {
                std::string title;
                if (col_title >= 0 && col_title < (int)fields.size())
                    title = fields[col_title];
                if (title.size() > SNBForumSchema::TITLE_MAX_LEN)
                    title.resize(SNBForumSchema::TITLE_MAX_LEN);
                int64_t mod_typed_id = 0;
                if (mod_map) {
                    auto mit = mod_map->find(ldbc_id);
                    if (mit != mod_map->end()) {
                        auto pit = person_id_map.find(mit->second);
                        if (pit != person_id_map.end())
                            mod_typed_id = (int64_t)pit->second;
                    }
                }
                size_t total = SNBForumSchema::TOTAL_SIZE + title.size() + 1;
                std::vector<uint8_t> buf(total, 0);
                if (col_creation >= 0 && col_creation < (int)fields.size())
                    SNBForumSchema::set_creation_date(buf.data(), parse_epoch_ms(fields[col_creation]));
                SNBForumSchema::set_moderator_id(buf.data(), mod_typed_id);
                SNBForumSchema::set_title(buf.data(), title.c_str());
                PendingNodeProp p;
                p.id = fid;
                p.data = std::move(buf);
                pending_node_props.push_back(std::move(p));
            }
        }
    }

    // tag_0_0.csv: id|name|url
    void load_tags(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;

        auto hdr = csv_split(line);
        int col_id=-1, col_name=-1, col_url=-1;
        for (int i = 0; i < (int)hdr.size(); i++) {
            if      (hdr[i] == "id")   col_id   = i;
            else if (hdr[i] == "name") col_name = i;
            else if (hdr[i] == "url")  col_url  = i;
        }
        if (col_id < 0) throw std::runtime_error("tag CSV missing 'id' column");

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            int64_t ldbc_id = std::stoll(fields[col_id]);
            node_id_t fid = MAKE_TYPED_ID(VT_TAG, tag_count++);
            tag_id_map[ldbc_id] = fid;
            log_node(fid);
            if (!dry_run) graph->add_node({fid, 0, 0}, false);
            if (!dry_run && opts.has_node_props) {
                uint8_t buf[SNBTagSchema::TOTAL_SIZE] = {};
                if (col_name >= 0 && col_name < (int)fields.size())
                    SNBTagSchema::set_name(buf, fields[col_name].c_str());
                if (col_url >= 0 && col_url < (int)fields.size())
                    SNBTagSchema::set_url(buf, fields[col_url].c_str());
                PendingNodeProp p;
                p.id = fid;
                p.data.assign(buf, buf + SNBTagSchema::TOTAL_SIZE);
                pending_node_props.push_back(std::move(p));
            }
        }
    }

    // tagclass_0_0.csv: id|name|url
    void load_tagclasses(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;

        auto hdr = csv_split(line);
        int col_id=-1, col_name=-1, col_url=-1;
        for (int i = 0; i < (int)hdr.size(); i++) {
            if      (hdr[i] == "id")   col_id   = i;
            else if (hdr[i] == "name") col_name = i;
            else if (hdr[i] == "url")  col_url  = i;
        }
        if (col_id < 0) throw std::runtime_error("tagclass CSV missing 'id' column");

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            int64_t ldbc_id = std::stoll(fields[col_id]);
            node_id_t fid = MAKE_TYPED_ID(VT_TAGCLASS, tagclass_count++);
            tagclass_id_map[ldbc_id] = fid;
            log_node(fid);
            if (!dry_run) graph->add_node({fid, 0, 0}, false);
            if (!dry_run && opts.has_node_props) {
                uint8_t buf[SNBTagSchema::TOTAL_SIZE] = {};
                if (col_name >= 0 && col_name < (int)fields.size())
                    SNBTagSchema::set_name(buf, fields[col_name].c_str());
                if (col_url >= 0 && col_url < (int)fields.size())
                    SNBTagSchema::set_url(buf, fields[col_url].c_str());
                PendingNodeProp p;
                p.id = fid;
                p.data.assign(buf, buf + SNBTagSchema::TOTAL_SIZE);
                pending_node_props.push_back(std::move(p));
            }
        }
    }

    // place_0_0.csv: id|name|url|type  (type: City / Country / Continent)
    void load_places(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;

        auto hdr = csv_split(line);
        int col_id=-1, col_name=-1, col_url=-1, col_type=-1;
        for (int i = 0; i < (int)hdr.size(); i++) {
            if      (hdr[i] == "id")   col_id   = i;
            else if (hdr[i] == "name") col_name = i;
            else if (hdr[i] == "url")  col_url  = i;
            else if (hdr[i] == "type") col_type = i;
        }
        if (col_id < 0) throw std::runtime_error("place CSV missing 'id' column");

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            int64_t ldbc_id = std::stoll(fields[col_id]);

            std::string type_str;
            if (col_type >= 0 && col_type < (int)fields.size())
                type_str = fields[col_type];

            uint8_t vt, place_type_val;
            node_id_t counter;
            if (type_str == "City") {
                vt = VT_CITY; place_type_val = SNBPlaceSchema::TYPE_CITY;
                counter = city_count++;
            } else if (type_str == "Continent") {
                vt = VT_CONTINENT; place_type_val = SNBPlaceSchema::TYPE_CONTINENT;
                counter = continent_count++;
            } else {
                // Default: Country
                vt = VT_COUNTRY; place_type_val = SNBPlaceSchema::TYPE_COUNTRY;
                counter = country_count++;
            }

            node_id_t fid = MAKE_TYPED_ID(vt, counter);
            place_id_map[ldbc_id] = fid;
            log_node(fid);
            if (!dry_run) graph->add_node({fid, 0, 0}, false);
            if (!dry_run && opts.has_node_props) {
                uint8_t buf[SNBPlaceSchema::TOTAL_SIZE] = {};
                if (col_name >= 0 && col_name < (int)fields.size())
                    SNBPlaceSchema::set_name(buf, fields[col_name].c_str());
                if (col_url >= 0 && col_url < (int)fields.size())
                    SNBPlaceSchema::set_url(buf, fields[col_url].c_str());
                SNBPlaceSchema::set_place_type(buf, (int8_t)place_type_val);
                PendingNodeProp p;
                p.id = fid;
                p.data.assign(buf, buf + SNBPlaceSchema::TOTAL_SIZE);
                pending_node_props.push_back(std::move(p));
            }
        }
    }

    // organisation_0_0.csv: id|type|name|url  (type: company / university)
    void load_organisations(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;

        auto hdr = csv_split(line);
        int col_id=-1, col_type=-1, col_name=-1, col_url=-1;
        for (int i = 0; i < (int)hdr.size(); i++) {
            if      (hdr[i] == "id")   col_id   = i;
            else if (hdr[i] == "type") col_type = i;
            else if (hdr[i] == "name") col_name = i;
            else if (hdr[i] == "url")  col_url  = i;
        }
        if (col_id < 0) throw std::runtime_error("organisation CSV missing 'id' column");

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            int64_t ldbc_id = std::stoll(fields[col_id]);

            std::string type_str;
            if (col_type >= 0 && col_type < (int)fields.size())
                type_str = fields[col_type];

            uint8_t vt, org_type_val;
            node_id_t counter;
            if (type_str == "university") {
                vt = VT_UNIVERSITY; org_type_val = SNBOrganisationSchema::TYPE_UNIVERSITY;
                counter = university_count++;
            } else {
                // Default: company
                vt = VT_COMPANY; org_type_val = SNBOrganisationSchema::TYPE_COMPANY;
                counter = company_count++;
            }

            node_id_t fid = MAKE_TYPED_ID(vt, counter);
            organisation_id_map[ldbc_id] = fid;
            log_node(fid);
            if (!dry_run) graph->add_node({fid, 0, 0}, false);
            if (!dry_run && opts.has_node_props) {
                uint8_t buf[SNBOrganisationSchema::TOTAL_SIZE] = {};
                if (col_name >= 0 && col_name < (int)fields.size())
                    SNBOrganisationSchema::set_name(buf, fields[col_name].c_str());
                if (col_url >= 0 && col_url < (int)fields.size())
                    SNBOrganisationSchema::set_url(buf, fields[col_url].c_str());
                SNBOrganisationSchema::set_org_type(buf, (int8_t)org_type_val);
                PendingNodeProp p;
                p.id = fid;
                p.data.assign(buf, buf + SNBOrganisationSchema::TOTAL_SIZE);
                pending_node_props.push_back(std::move(p));
            }
        }
    }

    // person_email_emailaddress_0_0.csv: Person.id|email
    void load_person_emails(const std::string &path)
    {
        if (opts.prop_mode != COLUMNAR) return;
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;
        std::unordered_map<node_id_t, uint64_t> idx_counter;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            if ((int)fields.size() < 2) continue;
            int64_t ldbc_id = std::stoll(fields[0]);
            auto it = person_id_map.find(ldbc_id);
            if (it == person_id_map.end()) continue;
            node_id_t fid = it->second;
            uint64_t idx = idx_counter[fid]++;
            if (!dry_run) graph->add_person_email(fid, idx, fields[1].c_str());
        }
    }

    // person_speaks_language_0_0.csv: Person.id|language
    void load_person_speaks(const std::string &path)
    {
        if (opts.prop_mode != COLUMNAR) return;
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;
        std::unordered_map<node_id_t, uint64_t> idx_counter;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            if ((int)fields.size() < 2) continue;
            int64_t ldbc_id = std::stoll(fields[0]);
            auto it = person_id_map.find(ldbc_id);
            if (it == person_id_map.end()) continue;
            node_id_t fid = it->second;
            uint64_t idx = idx_counter[fid]++;
            if (!dry_run) graph->add_person_language(fid, idx, fields[1].c_str());
        }
    }

    // =========================================================================
    // Phase 2: flush node props (after ALL add_node calls)
    // =========================================================================

    void flush_node_props()
    {
        if (!opts.has_node_props) return;
        for (auto &p : pending_node_props)
            graph->set_node_properties(p.id, p.data.data(), p.data.size());
        pending_node_props.clear();
    }

    // =========================================================================
    // Phase 3: load edges
    // =========================================================================

    // person_knows_person_0_0.csv: Person.id|Person.id|creationDate
    void load_knows(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;

        auto hdr = csv_split(line);
        int col_creation = -1;
        for (int i = 0; i < (int)hdr.size(); i++)
            if (hdr[i] == "creationDate") col_creation = i;

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            if ((int)fields.size() < 2) continue;
            auto sit = person_id_map.find(std::stoll(fields[0]));
            auto dit = person_id_map.find(std::stoll(fields[1]));
            if (sit == person_id_map.end() || dit == person_id_map.end()) continue;
            node_id_t src = sit->second, dst = dit->second;
            log_edge(src, dst);
            if (!dry_run) {
                { edge e; e.src_id=src; e.dst_id=dst; graph->add_edge(e, false); }
                if (opts.has_edge_props && col_creation >= 0 && col_creation < (int)fields.size()) {
                    uint8_t buf[SNBKnowsSchema::TOTAL_SIZE] = {};
                    SNBKnowsSchema::set_creation_date(buf, parse_epoch_ms(fields[col_creation]));
                    PendingEdgeProp p; p.src=src; p.dst=dst;
                    p.data.assign(buf, buf + SNBKnowsSchema::TOTAL_SIZE);
                    pending_edge_props.push_back(std::move(p));
                }
            }
        }
    }

    // post_hasCreator_person_0_0.csv: Post.id|Person.id  (no props)
    void load_has_creator(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            if ((int)fields.size() < 2) continue;
            auto pit = post_id_map.find(std::stoll(fields[0]));
            auto ait = person_id_map.find(std::stoll(fields[1]));
            if (pit == post_id_map.end() || ait == person_id_map.end()) continue;
            add_structural_edge(pit->second, ait->second);
        }
    }

    // comment_hasCreator_person_0_0.csv: Comment.id|Person.id  (no props)
    void load_comment_has_creator(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            if ((int)fields.size() < 2) continue;
            auto cit = comment_id_map.find(std::stoll(fields[0]));
            auto ait = person_id_map.find(std::stoll(fields[1]));
            if (cit == comment_id_map.end() || ait == person_id_map.end()) continue;
            add_structural_edge(cit->second, ait->second);
        }
    }

    // person_likes_post_0_0.csv: Person.id|Post.id|creationDate
    void load_likes(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;

        auto hdr = csv_split(line);
        int col_creation = -1;
        for (int i = 0; i < (int)hdr.size(); i++)
            if (hdr[i] == "creationDate") col_creation = i;

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            if ((int)fields.size() < 2) continue;
            auto pit = person_id_map.find(std::stoll(fields[0]));
            auto poit = post_id_map.find(std::stoll(fields[1]));
            if (pit == person_id_map.end() || poit == post_id_map.end()) continue;
            node_id_t src = pit->second, dst = poit->second;
            log_edge(src, dst);
            if (!dry_run) {
                { edge e; e.src_id=src; e.dst_id=dst; graph->add_edge(e, false); }
                if (opts.has_edge_props && col_creation >= 0 && col_creation < (int)fields.size()) {
                    uint8_t buf[SNBLikesSchema::TOTAL_SIZE] = {};
                    SNBLikesSchema::set_creation_date(buf, parse_epoch_ms(fields[col_creation]));
                    PendingEdgeProp p; p.src=src; p.dst=dst;
                    p.data.assign(buf, buf + SNBLikesSchema::TOTAL_SIZE);
                    pending_edge_props.push_back(std::move(p));
                }
            }
        }
    }

    // person_likes_comment_0_0.csv: Person.id|Comment.id|creationDate
    void load_likes_comment(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;

        auto hdr = csv_split(line);
        int col_creation = -1;
        for (int i = 0; i < (int)hdr.size(); i++)
            if (hdr[i] == "creationDate") col_creation = i;

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            if ((int)fields.size() < 2) continue;
            auto pit = person_id_map.find(std::stoll(fields[0]));
            auto cit = comment_id_map.find(std::stoll(fields[1]));
            if (pit == person_id_map.end() || cit == comment_id_map.end()) continue;
            node_id_t src = pit->second, dst = cit->second;
            log_edge(src, dst);
            if (!dry_run) {
                { edge e; e.src_id=src; e.dst_id=dst; graph->add_edge(e, false); }
                if (opts.has_edge_props && col_creation >= 0 && col_creation < (int)fields.size()) {
                    uint8_t buf[SNBLikesSchema::TOTAL_SIZE] = {};
                    SNBLikesSchema::set_creation_date(buf, parse_epoch_ms(fields[col_creation]));
                    PendingEdgeProp p; p.src=src; p.dst=dst;
                    p.data.assign(buf, buf + SNBLikesSchema::TOTAL_SIZE);
                    pending_edge_props.push_back(std::move(p));
                }
            }
        }
    }

    // comment_replyOf_post_0_0.csv: Comment.id|Post.id  (no props)
    void load_reply_of_post(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            if ((int)fields.size() < 2) continue;
            auto cit = comment_id_map.find(std::stoll(fields[0]));
            auto pit = post_id_map.find(std::stoll(fields[1]));
            if (cit == comment_id_map.end() || pit == post_id_map.end()) continue;
            add_structural_edge(cit->second, pit->second);
        }
    }

    // comment_replyOf_comment_0_0.csv: Comment.id|Comment.id  (no props)
    void load_reply_of_comment(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            if ((int)fields.size() < 2) continue;
            auto sit = comment_id_map.find(std::stoll(fields[0]));
            auto dit = comment_id_map.find(std::stoll(fields[1]));
            if (sit == comment_id_map.end() || dit == comment_id_map.end()) continue;
            add_structural_edge(sit->second, dit->second);
        }
    }

    // forum_containerOf_post_0_0.csv: Forum.id|Post.id  (no props)
    void load_container_of(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            if ((int)fields.size() < 2) continue;
            auto fit = forum_id_map.find(std::stoll(fields[0]));
            auto pit = post_id_map.find(std::stoll(fields[1]));
            if (fit == forum_id_map.end() || pit == post_id_map.end()) continue;
            add_structural_edge(fit->second, pit->second);
        }
    }

    // forum_hasMember_person_0_0.csv: Forum.id|Person.id|creationDate
    void load_has_member(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;

        auto hdr = csv_split(line);
        int col_creation = -1;
        for (int i = 0; i < (int)hdr.size(); i++)
            if (hdr[i] == "creationDate") col_creation = i;

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            if ((int)fields.size() < 2) continue;
            auto fit = forum_id_map.find(std::stoll(fields[0]));
            auto pit = person_id_map.find(std::stoll(fields[1]));
            if (fit == forum_id_map.end() || pit == person_id_map.end()) continue;
            node_id_t src = fit->second, dst = pit->second;
            log_edge(src, dst);
            if (!dry_run) {
                { edge e; e.src_id=src; e.dst_id=dst; graph->add_edge(e, false); }
                if (opts.has_edge_props && col_creation >= 0 && col_creation < (int)fields.size()) {
                    uint8_t buf[SNBHasMemberSchema::TOTAL_SIZE] = {};
                    SNBHasMemberSchema::set_creation_date(buf, parse_epoch_ms(fields[col_creation]));
                    PendingEdgeProp p; p.src=src; p.dst=dst;
                    p.data.assign(buf, buf + SNBHasMemberSchema::TOTAL_SIZE);
                    pending_edge_props.push_back(std::move(p));
                }
            }
        }
    }

    // person_studyAt_organisation_0_0.csv: Person.id|Organisation.id|classYear
    // Organisation must be loaded first (so organisation_id_map is populated with VT_UNIVERSITY IDs).
    void load_study_at(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;

        auto hdr = csv_split(line);
        int col_year = -1;
        for (int i = 0; i < (int)hdr.size(); i++)
            if (hdr[i] == "classYear") col_year = i;

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            if ((int)fields.size() < 2) continue;
            auto pit = person_id_map.find(std::stoll(fields[0]));
            auto oit = organisation_id_map.find(std::stoll(fields[1]));
            if (pit == person_id_map.end() || oit == organisation_id_map.end()) continue;
            node_id_t src = pit->second, dst = oit->second;
            log_edge(src, dst);
            if (!dry_run) {
                { edge e; e.src_id=src; e.dst_id=dst; graph->add_edge(e, false); }
                if (opts.has_edge_props && col_year >= 0 && col_year < (int)fields.size()) {
                    uint8_t buf[SNBStudyAtSchema::TOTAL_SIZE] = {};
                    SNBStudyAtSchema::set_class_year(buf, std::stoi(fields[col_year]));
                    PendingEdgeProp p; p.src=src; p.dst=dst;
                    p.data.assign(buf, buf + SNBStudyAtSchema::TOTAL_SIZE);
                    pending_edge_props.push_back(std::move(p));
                }
            }
        }
    }

    // person_workAt_company_0_0.csv: Person.id|Organisation.id|workFrom
    void load_work_at(const std::string &path)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;

        auto hdr = csv_split(line);
        int col_from = -1;
        for (int i = 0; i < (int)hdr.size(); i++)
            if (hdr[i] == "workFrom") col_from = i;

        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            if ((int)fields.size() < 2) continue;
            auto pit = person_id_map.find(std::stoll(fields[0]));
            auto oit = organisation_id_map.find(std::stoll(fields[1]));
            if (pit == person_id_map.end() || oit == organisation_id_map.end()) continue;
            node_id_t src = pit->second, dst = oit->second;
            log_edge(src, dst);
            if (!dry_run) {
                { edge e; e.src_id=src; e.dst_id=dst; graph->add_edge(e, false); }
                if (opts.has_edge_props && col_from >= 0 && col_from < (int)fields.size()) {
                    uint8_t buf[SNBWorkAtSchema::TOTAL_SIZE] = {};
                    SNBWorkAtSchema::set_work_from(buf, std::stoi(fields[col_from]));
                    PendingEdgeProp p; p.src=src; p.dst=dst;
                    p.data.assign(buf, buf + SNBWorkAtSchema::TOTAL_SIZE);
                    pending_edge_props.push_back(std::move(p));
                }
            }
        }
    }

    // ---- Structural-only edge loaders (no properties) -----------------------

    // Generic two-column CSV loader for structural-only edges.
    // src_map and dst_map are the id maps for the source and destination types.
    void load_structural_edges(const std::string &path,
                               const std::unordered_map<int64_t, node_id_t> &src_map,
                               const std::unordered_map<int64_t, node_id_t> &dst_map)
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open: " + path);
        std::string line;
        if (!std::getline(f, line)) return;  // skip header
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = csv_split(line);
            if ((int)fields.size() < 2) continue;
            auto sit = src_map.find(std::stoll(fields[0]));
            auto dit = dst_map.find(std::stoll(fields[1]));
            if (sit == src_map.end() || dit == dst_map.end()) continue;
            add_structural_edge(sit->second, dit->second);
        }
    }

    // Convenience wrappers using load_structural_edges:

    // post_hasTag_tag_0_0.csv: Post.id|Tag.id
    void load_post_has_tag(const std::string &path)
    { load_structural_edges(path, post_id_map, tag_id_map); }

    // comment_hasTag_tag_0_0.csv: Comment.id|Tag.id
    void load_comment_has_tag(const std::string &path)
    { load_structural_edges(path, comment_id_map, tag_id_map); }

    // forum_hasTag_tag_0_0.csv: Forum.id|Tag.id
    void load_forum_has_tag(const std::string &path)
    { load_structural_edges(path, forum_id_map, tag_id_map); }

    // person_hasInterest_tag_0_0.csv: Person.id|Tag.id
    void load_has_interest(const std::string &path)
    { load_structural_edges(path, person_id_map, tag_id_map); }

    // person_isLocatedIn_place_0_0.csv: Person.id|Place.id (City)
    void load_person_is_located_in(const std::string &path)
    { load_structural_edges(path, person_id_map, place_id_map); }

    // post_isLocatedIn_place_0_0.csv: Post.id|Place.id (Country)
    void load_post_is_located_in(const std::string &path)
    { load_structural_edges(path, post_id_map, place_id_map); }

    // comment_isLocatedIn_place_0_0.csv: Comment.id|Place.id (Country)
    void load_comment_is_located_in(const std::string &path)
    { load_structural_edges(path, comment_id_map, place_id_map); }

    // organisation_isLocatedIn_place_0_0.csv: Organisation.id|Place.id
    void load_org_is_located_in(const std::string &path)
    { load_structural_edges(path, organisation_id_map, place_id_map); }

    // place_isPartOf_place_0_0.csv: Place.id|Place.id
    void load_is_part_of(const std::string &path)
    { load_structural_edges(path, place_id_map, place_id_map); }

    // tag_hasType_tagclass_0_0.csv: Tag.id|TagClass.id
    void load_has_type(const std::string &path)
    { load_structural_edges(path, tag_id_map, tagclass_id_map); }

    // tagclass_isSubclassOf_tagclass_0_0.csv: TagClass.id|TagClass.id
    void load_is_subclass_of(const std::string &path)
    { load_structural_edges(path, tagclass_id_map, tagclass_id_map); }

    // =========================================================================
    // Phase 4: flush edge props (after ALL add_edge calls)
    // =========================================================================

    void flush_edge_props()
    {
        if (!opts.has_edge_props) return;
        for (auto &p : pending_edge_props)
            graph->set_edge_properties(p.src, p.dst, p.data.data(), p.data.size());
        pending_edge_props.clear();
    }

    // ---- ID-type helpers ----
    static bool is_person(node_id_t id)      { return VTYPE_OF(id) == VT_PERSON; }
    static bool is_post(node_id_t id)        { return VTYPE_OF(id) == VT_POST; }
    static bool is_comment(node_id_t id)     { return VTYPE_OF(id) == VT_COMMENT; }
    static bool is_forum(node_id_t id)       { return VTYPE_OF(id) == VT_FORUM; }
    static bool is_tag(node_id_t id)         { return VTYPE_OF(id) == VT_TAG; }
    static bool is_tagclass(node_id_t id)    { return VTYPE_OF(id) == VT_TAGCLASS; }
    static bool is_place(node_id_t id)       { uint8_t t = VTYPE_OF(id);
                                               return t==VT_CITY||t==VT_COUNTRY||t==VT_CONTINENT; }
    static bool is_organisation(node_id_t id){ uint8_t t = VTYPE_OF(id);
                                               return t==VT_COMPANY||t==VT_UNIVERSITY; }
};

#endif  // LDBC_SNB_LOADER_H
