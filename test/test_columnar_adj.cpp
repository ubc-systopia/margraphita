// COLUMNAR property storage tests for AdjList.
//
// Tests:
//   1. Person props — all seven fields round-trip through person_props table
//   2. Post props   — creationDate/length/tag/content (text and imageFile)
//   3. Knows edge props — creationDate via knows_props table
//   4. Likes edge props — creationDate via likes_props table
//   5. person_email secondary table — multiple emails per person
//   6. person_speaks secondary table — multiple languages per person
//   7. colgroup range scan — knows_props:temporal forward scan for one source

#include <cassert>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>

#include "adj_list.h"
#include "common_defs.h"
#include "graph_engine.h"
#include "prop_schema.h"

#define PASS() fprintf(stderr, "%s: PASSED\n", __FUNCTION__)
#define INFO() fprintf(stderr, "--- %s\n", __FUNCTION__)

// Typed node IDs used throughout
static const node_id_t P0  = MAKE_TYPED_ID(VT_PERSON, 0);
static const node_id_t P1  = MAKE_TYPED_ID(VT_PERSON, 1);
static const node_id_t P2  = MAKE_TYPED_ID(VT_PERSON, 2);
static const node_id_t PT0 = MAKE_TYPED_ID(VT_POST, 0);

// Helpers to avoid designated-initializer syntax through virtual calls
static void add_node(GraphBase &g, node_id_t id)
{
    node n{}; n.id = id; g.add_node(n, false);
}
static void add_edge(GraphBase &g, node_id_t src, node_id_t dst)
{
    edge e{}; e.src_id = src; e.dst_id = dst; g.add_edge(e, false);
}

// ---- Test 1: Person props -----------------------------------------------

void test_columnar_person_props(GraphBase &g)
{
    INFO();
    add_node(g, P0);

    uint8_t buf[SNBPersonSchema::TOTAL_SIZE] = {};
    SNBPersonSchema::set_creation_date(buf, 1111111LL);
    SNBPersonSchema::set_birthday(buf, 2222222LL);
    SNBPersonSchema::set_gender(buf, 1);
    SNBPersonSchema::set_first_name(buf, "Alice");
    SNBPersonSchema::set_last_name(buf, "Smith");
    SNBPersonSchema::set_browser_used(buf, "Firefox");
    SNBPersonSchema::set_location_ip(buf, "10.0.0.1");
    SNBPersonSchema::set_country_id(buf, 42);
    g.set_node_properties(P0, buf, SNBPersonSchema::TOTAL_SIZE);

    prop_blob pb = g.get_node_properties(P0);
    assert(pb.data != nullptr);
    assert(pb.size == SNBPersonSchema::TOTAL_SIZE);
    assert(SNBPersonSchema::get_creation_date(pb.data) == 1111111LL);
    assert(SNBPersonSchema::get_birthday(pb.data) == 2222222LL);
    assert(SNBPersonSchema::get_gender(pb.data) == 1);
    assert(std::string(SNBPersonSchema::get_first_name(pb.data)) == "Alice");
    assert(std::string(SNBPersonSchema::get_last_name(pb.data)) == "Smith");
    assert(std::string(SNBPersonSchema::get_browser_used(pb.data)) == "Firefox");
    assert(std::string(SNBPersonSchema::get_location_ip(pb.data)) == "10.0.0.1");
    assert(SNBPersonSchema::get_country_id(pb.data) == 42);
    delete[] pb.data;

    // Overwrite and verify update path
    SNBPersonSchema::set_first_name(buf, "Alicia");
    g.set_node_properties(P0, buf, SNBPersonSchema::TOTAL_SIZE);
    pb = g.get_node_properties(P0);
    assert(std::string(SNBPersonSchema::get_first_name(pb.data)) == "Alicia");
    delete[] pb.data;

    PASS();
}

// ---- Test 2: Post props -------------------------------------------------

void test_columnar_post_props(GraphBase &g)
{
    INFO();
    add_node(g, PT0);

    // Text content post
    std::string content = "Hello COLUMNAR world!";
    size_t total = SNBPostSchema::TOTAL_SIZE + content.size() + 1;
    std::vector<uint8_t> buf(total, 0);
    SNBPostSchema::set_creation_date(buf.data(), 5555555LL);
    SNBPostSchema::set_length(buf.data(), (int32_t)content.size());
    SNBPostSchema::set_tag(buf.data(), 0);  // text content
    SNBPostSchema::set_content(buf.data(), content.c_str());
    g.set_node_properties(PT0, buf.data(), buf.size());

    prop_blob pb = g.get_node_properties(PT0);
    assert(pb.data != nullptr);
    assert(pb.size >= SNBPostSchema::TOTAL_SIZE);
    assert(SNBPostSchema::get_creation_date(pb.data) == 5555555LL);
    assert(SNBPostSchema::get_length(pb.data) == (int32_t)content.size());
    assert(SNBPostSchema::get_tag(pb.data) == 0);
    assert(std::string(SNBPostSchema::get_content(pb.data, pb.size)) == content);
    delete[] pb.data;

    // imageFile post (overwrite)
    std::string img = "photo_42.jpg";
    size_t total2 = SNBPostSchema::TOTAL_SIZE + img.size() + 1;
    std::vector<uint8_t> buf2(total2, 0);
    SNBPostSchema::set_creation_date(buf2.data(), 6666666LL);
    SNBPostSchema::set_length(buf2.data(), 0);
    SNBPostSchema::set_tag(buf2.data(), 1);  // imageFile
    SNBPostSchema::set_content(buf2.data(), img.c_str());
    g.set_node_properties(PT0, buf2.data(), buf2.size());

    pb = g.get_node_properties(PT0);
    assert(pb.data != nullptr);
    assert(SNBPostSchema::get_tag(pb.data) == 1);
    assert(std::string(SNBPostSchema::get_content(pb.data, pb.size)) == img);
    delete[] pb.data;

    PASS();
}

// ---- Test 3: Knows edge props -------------------------------------------

void test_columnar_knows_props(GraphBase &g)
{
    INFO();
    add_node(g, P1);
    add_node(g, P2);
    add_edge(g, P1, P2);

    uint8_t buf[SNBKnowsSchema::TOTAL_SIZE] = {};
    SNBKnowsSchema::set_creation_date(buf, 9876543LL);
    g.set_edge_properties(P1, P2, buf, SNBKnowsSchema::TOTAL_SIZE);

    prop_blob pb = g.get_edge_properties(P1, P2);
    assert(pb.data != nullptr);
    assert(pb.size == SNBKnowsSchema::TOTAL_SIZE);
    assert(SNBKnowsSchema::get_creation_date(pb.data) == 9876543LL);
    delete[] pb.data;

    PASS();
}

// ---- Test 4: Likes edge props -------------------------------------------

void test_columnar_likes_props(GraphBase &g)
{
    INFO();
    // P0 and PT0 already exist from earlier tests
    add_edge(g, P0, PT0);

    uint8_t buf[SNBLikesSchema::TOTAL_SIZE] = {};
    SNBLikesSchema::set_creation_date(buf, 1122334LL);
    g.set_edge_properties(P0, PT0, buf, SNBLikesSchema::TOTAL_SIZE);

    prop_blob pb = g.get_edge_properties(P0, PT0);
    assert(pb.data != nullptr);
    assert(pb.size == SNBLikesSchema::TOTAL_SIZE);
    assert(SNBLikesSchema::get_creation_date(pb.data) == 1122334LL);
    delete[] pb.data;

    PASS();
}

// ---- Test 5: person_email secondary table -------------------------------

void test_columnar_person_email(GraphBase &g)
{
    INFO();
    // P0 already exists; write three emails
    g.add_person_email(P0, 0, "alice@example.com");
    g.add_person_email(P0, 1, "alice@work.com");
    g.add_person_email(P0, 2, "alice@uni.edu");

    auto emails = g.get_person_emails(P0);
    assert(emails.size() == 3);
    assert(emails[0] == "alice@example.com");
    assert(emails[1] == "alice@work.com");
    assert(emails[2] == "alice@uni.edu");

    // P1 gets one email; must not appear in P0's results
    g.add_person_email(P1, 0, "bob@example.com");
    emails = g.get_person_emails(P0);
    assert(emails.size() == 3);
    emails = g.get_person_emails(P1);
    assert(emails.size() == 1);
    assert(emails[0] == "bob@example.com");

    // Unknown person returns empty
    node_id_t unknown = MAKE_TYPED_ID(VT_PERSON, 99);
    assert(g.get_person_emails(unknown).empty());

    PASS();
}

// ---- Test 6: person_speaks secondary table ------------------------------

void test_columnar_person_speaks(GraphBase &g)
{
    INFO();
    g.add_person_language(P0, 0, "en");
    g.add_person_language(P0, 1, "fr");

    auto langs = g.get_person_languages(P0);
    assert(langs.size() == 2);
    assert(langs[0] == "en");
    assert(langs[1] == "fr");

    g.add_person_language(P1, 0, "de");
    langs = g.get_person_languages(P0);
    assert(langs.size() == 2);
    langs = g.get_person_languages(P1);
    assert(langs.size() == 1);
    assert(langs[0] == "de");

    PASS();
}

// ---- Test 7: colgroup range scan ----------------------------------------

void test_columnar_colgroup_scan(GraphBase &g)
{
    INFO();
    add_node(g, P0);
    add_node(g, P1);
    add_node(g, P2);
    add_edge(g, P0, P1);
    add_edge(g, P0, P2);

    uint8_t buf[SNBKnowsSchema::TOTAL_SIZE] = {};
    SNBKnowsSchema::set_creation_date(buf, 1000LL);
    g.set_edge_properties(P0, P1, buf, SNBKnowsSchema::TOTAL_SIZE);

    SNBKnowsSchema::set_creation_date(buf, 2000LL);
    g.set_edge_properties(P0, P2, buf, SNBKnowsSchema::TOTAL_SIZE);

    auto cur = g.get_edge_prop_cursor(KNOWS_PROPS_TABLE, CG_TEMPORAL);
    assert(cur != nullptr);

    std::vector<std::pair<node_id_t, int64_t>> found;
    cur->set_src(P0);
    while (cur->next())
        found.emplace_back(cur->dst(), (int64_t)cur->get_uint64(0));

    assert(found.size() >= 2);
    bool found_p1 = false, found_p2 = false;
    for (auto &[dst, cdate] : found) {
        if (dst == P1 && cdate == 1000LL) found_p1 = true;
        if (dst == P2 && cdate == 2000LL) found_p2 = true;
    }
    assert(found_p1);
    assert(found_p2);

    PASS();
}

// ---- main ---------------------------------------------------------------

int main()
{
    graph_opts opts;
    opts.create_new      = true;
    opts.optimize_create = false;
    opts.is_directed     = true;
    opts.read_optimize   = true;
    opts.is_weighted     = false;
    opts.has_node_props  = true;
    opts.has_edge_props  = true;
    opts.prop_mode       = COLUMNAR;
    opts.type            = GraphType::Adj;
    opts.db_name         = "test_columnar_adj";
    opts.db_dir          = "./db_test/db_col_adj";
    opts.conn_config     = "cache_size=512MB";
    opts.stat_log        = "./";

    GraphEngine engine(1, opts);
    GraphBase *graph_ptr = engine.create_graph_handle();
    GraphBase &graph = *graph_ptr;

    test_columnar_person_props(graph);
    test_columnar_post_props(graph);
    test_columnar_knows_props(graph);
    test_columnar_likes_props(graph);
    test_columnar_person_email(graph);
    test_columnar_person_speaks(graph);
    test_columnar_colgroup_scan(graph);

    graph_ptr->close(false);
    engine.close_graph();

    fprintf(stderr, "\nAll COLUMNAR AdjList tests passed.\n");
    return 0;
}
