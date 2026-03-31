// Full-schema COLUMNAR property storage tests for SplitEdgeKey.
//
// Tests:
//   1. Comment props    — creationDate/length/content round-trip
//   2. Forum props      — creationDate/moderator_id/title round-trip
//   3. Tag props        — name/url round-trip
//   4. TagClass props   — name/url round-trip (uses tagclass table)
//   5. Place props      — City/Country/Continent name/url/type round-trip
//   6. Organisation props — Company/University name/url/org_type round-trip
//   7. hasMember edge props — Forum→Person creationDate
//   8. studyAt edge props   — Person→University classYear
//   9. workAt edge props    — Person→Company workFrom
//  10. likes_comment edge props — Person→Comment creationDate

#include <cassert>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>

#include "common_defs.h"
#include "edgekey_split.h"
#include "graph_engine.h"
#include "prop_schema.h"

#define PASS() fprintf(stderr, "%s: PASSED\n", __FUNCTION__)
#define INFO() fprintf(stderr, "--- %s\n", __FUNCTION__)

static void add_node(GraphBase &g, node_id_t id) {
    node n{}; n.id = id; g.add_node(n, false);
}
static void add_edge(GraphBase &g, node_id_t src, node_id_t dst) {
    edge e{}; e.src_id = src; e.dst_id = dst; g.add_edge(e, false);
}

static const node_id_t P0   = MAKE_TYPED_ID(VT_PERSON,      0);
static const node_id_t P1   = MAKE_TYPED_ID(VT_PERSON,      1);
static const node_id_t C0   = MAKE_TYPED_ID(VT_COMMENT,     0);
static const node_id_t F0   = MAKE_TYPED_ID(VT_FORUM,       0);
static const node_id_t TG0  = MAKE_TYPED_ID(VT_TAG,         0);
static const node_id_t TC0  = MAKE_TYPED_ID(VT_TAGCLASS,    0);
static const node_id_t CY0  = MAKE_TYPED_ID(VT_CITY,        0);
static const node_id_t CO0  = MAKE_TYPED_ID(VT_COUNTRY,     0);
static const node_id_t CON0 = MAKE_TYPED_ID(VT_CONTINENT,   0);
static const node_id_t CP0  = MAKE_TYPED_ID(VT_COMPANY,     0);
static const node_id_t UN0  = MAKE_TYPED_ID(VT_UNIVERSITY,  0);

// ---- Test 1: Comment props -----------------------------------------------

void test_comment_props(GraphBase &g)
{
    INFO();
    add_node(g, C0);

    std::string text = "This is a test comment.";
    size_t total = SNBPostSchema::TOTAL_SIZE + text.size() + 1;
    std::vector<uint8_t> buf(total, 0);
    SNBPostSchema::set_creation_date(buf.data(), 111000LL);
    SNBPostSchema::set_length(buf.data(), (int32_t)text.size());
    SNBPostSchema::set_tag(buf.data(), 0);
    SNBPostSchema::set_content(buf.data(), text.c_str());
    g.set_node_properties(C0, buf.data(), buf.size());

    prop_blob pb = g.get_node_properties(C0);
    assert(pb.data != nullptr);
    assert(pb.size >= SNBPostSchema::TOTAL_SIZE);
    assert(SNBPostSchema::get_creation_date(pb.data) == 111000LL);
    assert(SNBPostSchema::get_length(pb.data) == (int32_t)text.size());
    assert(SNBPostSchema::get_tag(pb.data) == 0);
    assert(std::string(SNBPostSchema::get_content(pb.data, pb.size)) == text);
    delete[] pb.data;

    PASS();
}

// ---- Test 2: Forum props -------------------------------------------------

void test_forum_props(GraphBase &g)
{
    INFO();
    add_node(g, P0);
    add_node(g, F0);

    std::string title = "Graph Database Enthusiasts";
    size_t total = SNBForumSchema::TOTAL_SIZE + title.size() + 1;
    std::vector<uint8_t> buf(total, 0);
    SNBForumSchema::set_creation_date(buf.data(), 222000LL);
    SNBForumSchema::set_moderator_id(buf.data(), (int64_t)P0);
    SNBForumSchema::set_title(buf.data(), title.c_str());
    g.set_node_properties(F0, buf.data(), buf.size());

    prop_blob pb = g.get_node_properties(F0);
    assert(pb.data != nullptr);
    assert(pb.size >= SNBForumSchema::TOTAL_SIZE);
    assert(SNBForumSchema::get_creation_date(pb.data) == 222000LL);
    assert(SNBForumSchema::get_moderator_id(pb.data) == (int64_t)P0);
    assert(std::string(SNBForumSchema::get_title(pb.data, pb.size)) == title);
    delete[] pb.data;

    PASS();
}

// ---- Test 3: Tag props ---------------------------------------------------

void test_tag_props(GraphBase &g)
{
    INFO();
    add_node(g, TG0);

    uint8_t buf[SNBTagSchema::TOTAL_SIZE] = {};
    SNBTagSchema::set_name(buf, "GraphDatabases");
    SNBTagSchema::set_url(buf, "http://dbpedia.org/resource/GraphDatabases");
    g.set_node_properties(TG0, buf, SNBTagSchema::TOTAL_SIZE);

    prop_blob pb = g.get_node_properties(TG0);
    assert(pb.data != nullptr);
    assert(pb.size == SNBTagSchema::TOTAL_SIZE);
    assert(std::string(SNBTagSchema::get_name(pb.data)) == "GraphDatabases");
    assert(std::string(SNBTagSchema::get_url(pb.data)) ==
           "http://dbpedia.org/resource/GraphDatabases");
    delete[] pb.data;

    PASS();
}

// ---- Test 4: TagClass props ----------------------------------------------

void test_tagclass_props(GraphBase &g)
{
    INFO();
    add_node(g, TC0);

    uint8_t buf[SNBTagSchema::TOTAL_SIZE] = {};
    SNBTagSchema::set_name(buf, "Software");
    SNBTagSchema::set_url(buf, "http://dbpedia.org/ontology/Software");
    g.set_node_properties(TC0, buf, SNBTagSchema::TOTAL_SIZE);

    prop_blob pb = g.get_node_properties(TC0);
    assert(pb.data != nullptr);
    assert(pb.size == SNBTagSchema::TOTAL_SIZE);
    assert(std::string(SNBTagSchema::get_name(pb.data)) == "Software");
    delete[] pb.data;

    PASS();
}

// ---- Test 5: Place props (City / Country / Continent) --------------------

void test_place_props(GraphBase &g)
{
    INFO();
    add_node(g, CY0);
    add_node(g, CO0);
    add_node(g, CON0);

    // City
    {
        uint8_t buf[SNBPlaceSchema::TOTAL_SIZE] = {};
        SNBPlaceSchema::set_name(buf, "Berlin");
        SNBPlaceSchema::set_url(buf, "http://dbpedia.org/resource/Berlin");
        SNBPlaceSchema::set_place_type(buf, SNBPlaceSchema::TYPE_CITY);
        g.set_node_properties(CY0, buf, SNBPlaceSchema::TOTAL_SIZE);

        prop_blob pb = g.get_node_properties(CY0);
        assert(pb.data != nullptr);
        assert(SNBPlaceSchema::get_place_type(pb.data) == SNBPlaceSchema::TYPE_CITY);
        assert(std::string(SNBPlaceSchema::get_name(pb.data)) == "Berlin");
        delete[] pb.data;
    }
    // Country
    {
        uint8_t buf[SNBPlaceSchema::TOTAL_SIZE] = {};
        SNBPlaceSchema::set_name(buf, "Germany");
        SNBPlaceSchema::set_url(buf, "http://dbpedia.org/resource/Germany");
        SNBPlaceSchema::set_place_type(buf, SNBPlaceSchema::TYPE_COUNTRY);
        g.set_node_properties(CO0, buf, SNBPlaceSchema::TOTAL_SIZE);

        prop_blob pb = g.get_node_properties(CO0);
        assert(pb.data != nullptr);
        assert(SNBPlaceSchema::get_place_type(pb.data) == SNBPlaceSchema::TYPE_COUNTRY);
        assert(std::string(SNBPlaceSchema::get_name(pb.data)) == "Germany");
        delete[] pb.data;
    }
    // Continent
    {
        uint8_t buf[SNBPlaceSchema::TOTAL_SIZE] = {};
        SNBPlaceSchema::set_name(buf, "Europe");
        SNBPlaceSchema::set_url(buf, "http://dbpedia.org/resource/Europe");
        SNBPlaceSchema::set_place_type(buf, SNBPlaceSchema::TYPE_CONTINENT);
        g.set_node_properties(CON0, buf, SNBPlaceSchema::TOTAL_SIZE);

        prop_blob pb = g.get_node_properties(CON0);
        assert(pb.data != nullptr);
        assert(SNBPlaceSchema::get_place_type(pb.data) == SNBPlaceSchema::TYPE_CONTINENT);
        assert(std::string(SNBPlaceSchema::get_name(pb.data)) == "Europe");
        delete[] pb.data;
    }

    PASS();
}

// ---- Test 6: Organisation props (Company / University) -------------------

void test_organisation_props(GraphBase &g)
{
    INFO();
    add_node(g, CP0);
    add_node(g, UN0);

    // Company
    {
        uint8_t buf[SNBOrganisationSchema::TOTAL_SIZE] = {};
        SNBOrganisationSchema::set_name(buf, "Neo4j");
        SNBOrganisationSchema::set_url(buf, "http://dbpedia.org/resource/Neo4j");
        SNBOrganisationSchema::set_org_type(buf, SNBOrganisationSchema::TYPE_COMPANY);
        g.set_node_properties(CP0, buf, SNBOrganisationSchema::TOTAL_SIZE);

        prop_blob pb = g.get_node_properties(CP0);
        assert(pb.data != nullptr);
        assert(SNBOrganisationSchema::get_org_type(pb.data) == SNBOrganisationSchema::TYPE_COMPANY);
        assert(std::string(SNBOrganisationSchema::get_name(pb.data)) == "Neo4j");
        delete[] pb.data;
    }
    // University
    {
        uint8_t buf[SNBOrganisationSchema::TOTAL_SIZE] = {};
        SNBOrganisationSchema::set_name(buf, "MIT");
        SNBOrganisationSchema::set_url(buf, "http://dbpedia.org/resource/MIT");
        SNBOrganisationSchema::set_org_type(buf, SNBOrganisationSchema::TYPE_UNIVERSITY);
        g.set_node_properties(UN0, buf, SNBOrganisationSchema::TOTAL_SIZE);

        prop_blob pb = g.get_node_properties(UN0);
        assert(pb.data != nullptr);
        assert(SNBOrganisationSchema::get_org_type(pb.data) == SNBOrganisationSchema::TYPE_UNIVERSITY);
        assert(std::string(SNBOrganisationSchema::get_name(pb.data)) == "MIT");
        delete[] pb.data;
    }

    PASS();
}

// ---- Test 7: hasMember edge props (Forum → Person) -----------------------

void test_has_member_props(GraphBase &g)
{
    INFO();
    add_edge(g, F0, P0);

    uint8_t buf[SNBHasMemberSchema::TOTAL_SIZE] = {};
    SNBHasMemberSchema::set_creation_date(buf, 333000LL);
    g.set_edge_properties(F0, P0, buf, SNBHasMemberSchema::TOTAL_SIZE);

    prop_blob pb = g.get_edge_properties(F0, P0);
    assert(pb.data != nullptr);
    assert(pb.size == SNBHasMemberSchema::TOTAL_SIZE);
    assert(SNBHasMemberSchema::get_creation_date(pb.data) == 333000LL);
    delete[] pb.data;

    PASS();
}

// ---- Test 8: studyAt edge props (Person → University) --------------------

void test_study_at_props(GraphBase &g)
{
    INFO();
    add_edge(g, P0, UN0);

    uint8_t buf[SNBStudyAtSchema::TOTAL_SIZE] = {};
    SNBStudyAtSchema::set_class_year(buf, 2005);
    g.set_edge_properties(P0, UN0, buf, SNBStudyAtSchema::TOTAL_SIZE);

    prop_blob pb = g.get_edge_properties(P0, UN0);
    assert(pb.data != nullptr);
    assert(pb.size == SNBStudyAtSchema::TOTAL_SIZE);
    assert(SNBStudyAtSchema::get_class_year(pb.data) == 2005);
    delete[] pb.data;

    PASS();
}

// ---- Test 9: workAt edge props (Person → Company) ------------------------

void test_work_at_props(GraphBase &g)
{
    INFO();
    add_edge(g, P0, CP0);

    uint8_t buf[SNBWorkAtSchema::TOTAL_SIZE] = {};
    SNBWorkAtSchema::set_work_from(buf, 2012);
    g.set_edge_properties(P0, CP0, buf, SNBWorkAtSchema::TOTAL_SIZE);

    prop_blob pb = g.get_edge_properties(P0, CP0);
    assert(pb.data != nullptr);
    assert(pb.size == SNBWorkAtSchema::TOTAL_SIZE);
    assert(SNBWorkAtSchema::get_work_from(pb.data) == 2012);
    delete[] pb.data;

    PASS();
}

// ---- Test 10: likes_comment edge props (Person → Comment) ----------------

void test_likes_comment_props(GraphBase &g)
{
    INFO();
    add_edge(g, P1, C0);

    uint8_t buf[SNBLikesSchema::TOTAL_SIZE] = {};
    SNBLikesSchema::set_creation_date(buf, 444000LL);
    g.set_edge_properties(P1, C0, buf, SNBLikesSchema::TOTAL_SIZE);

    prop_blob pb = g.get_edge_properties(P1, C0);
    assert(pb.data != nullptr);
    assert(pb.size == SNBLikesSchema::TOTAL_SIZE);
    assert(SNBLikesSchema::get_creation_date(pb.data) == 444000LL);
    delete[] pb.data;

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
    opts.type            = GraphType::SplitEKey;
    opts.db_name         = "test_col_full_ekey";
    opts.db_dir          = "./db_test/db_col_full_ekey";
    opts.conn_config     = "cache_size=512MB";
    opts.stat_log        = "./";

    GraphEngine engine(1, opts);
    WT_CONNECTION *conn = engine.get_connection();
    SplitEdgeKey graph(opts, conn);

    test_comment_props(graph);
    test_forum_props(graph);
    test_tag_props(graph);
    test_tagclass_props(graph);
    test_place_props(graph);
    test_organisation_props(graph);
    test_has_member_props(graph);
    test_study_at_props(graph);
    test_work_at_props(graph);
    test_likes_comment_props(graph);

    graph.close(false);
    engine.close_graph();

    fprintf(stderr, "\nAll full-schema COLUMNAR SplitEdgeKey tests passed.\n");
    return 0;
}
