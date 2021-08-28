#include "common.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "sample_graph.h"
#include <cassert>
#include "test_standard_graph.h"

#define INFO() \
    fprintf(stderr, "Now running: %s\n", __FUNCTION__);

void create_init_nodes(StandardGraph graph, bool is_directed)
{
    INFO()
    for (node x : SampleGraph::test_nodes)
    {
        graph.add_node(x);
    }

    if (!is_directed)
    {
        SampleGraph::create_undirected_edges();
        assert(SampleGraph::test_edges.size() == 6); //checking if directed edges got created and stored in test_edges
    }
    int edge_cnt = 1;
    for (edge x : SampleGraph::test_edges)
    {
        graph.add_edge(x);
        edge_cnt++;
    }
}

void tearDown(StandardGraph graph)
{
    graph.close();
}

void test_node_add(StandardGraph graph, bool read_optimize)
{
    INFO();
    node new_node = {
        .id = 11,
        .in_degree = 0,
        .out_degree = 0};
    graph.add_node(new_node);
    node found = graph.get_node(new_node.id);
    assert(new_node.id == found.id);
    if (read_optimize)
    {
        assert(found.in_degree == 0);
        assert(found.out_degree == 0);
    }
}

void test_node_delete(StandardGraph graph, bool is_directed)
{
    INFO();
    node to_delete = SampleGraph::node2;
    graph.delete_node(to_delete.id);

    //check that the node has been deleted
    assert(graph.has_node(to_delete.id) == false);

    //check if the edges have been deleted
    if (is_directed)
    {
        assert(graph.has_edge(SampleGraph::edge1.src_id,
                              SampleGraph::edge1.dst_id) == false);
        assert(graph.has_edge(SampleGraph::edge3.src_id,
                              SampleGraph::edge3.dst_id) == false);
    }
    else
    {
        //edge from 1->2
        assert(graph.has_edge(SampleGraph::edge1.src_id,
                              SampleGraph::edge1.dst_id) == false);
        //edge from 2->1
        assert(graph.has_edge(SampleGraph::edge1.dst_id,
                              SampleGraph::edge1.src_id) == false);
        //edge from 2->3
        assert(graph.has_edge(SampleGraph::edge3.src_id,
                              SampleGraph::edge3.dst_id) == false);
        //edge from 3->2
        assert(graph.has_edge(SampleGraph::edge3.dst_id,
                              SampleGraph::edge3.src_id) == false);
    }
}

void test_get_edge_id(StandardGraph graph)
{
    INFO();
    int eid1 = graph.get_edge_id(1, 2); //Edge ID 1
    int eid2 = graph.get_edge_id(1, 3); //Edge ID 2
    int eid3 = graph.get_edge_id(2, 3); //Edge ID 3

    assert(eid1 == 1);
    assert(eid2 == 2);
    assert(eid3 == 3);
}

void test_get_num_nodes_and_edges(StandardGraph graph, bool is_directed)
{
    INFO();
    if (is_directed)
    {
        assert(graph.get_num_nodes() == 4);
        assert(graph.get_num_edges() == 3);
    }
    else
    {
        assert(graph.get_num_nodes() == 4);
        assert(graph.get_num_edges() == 6);
    }
}

void test_get_node(StandardGraph graph)
{
    INFO();
    node found = graph.get_node(1);
    assert(found.id == 1);
}

void test_get_in_degree(StandardGraph graph, bool is_directed)
{
    INFO();
    if (is_directed)
    {

        assert(graph.get_in_degree(1) == 0);
        assert(graph.get_in_degree(2) == 1);
        assert(graph.get_in_degree(3) == 2);
    }
    else
    {
        assert(graph.get_in_degree(1) == 2);
        assert(graph.get_in_degree(2) == 2);
        assert(graph.get_in_degree(3) == 2);
    }
}

void test_get_out_degree(StandardGraph graph, bool is_directed)
{
    INFO();
    if (is_directed)
    {
        assert(graph.get_out_degree(1) == 2);
        assert(graph.get_out_degree(2) == 1);
        assert(graph.get_out_degree(3) == 0);
    }
    else
    {
        assert(graph.get_out_degree(1) == 2);
        assert(graph.get_out_degree(2) == 2);
        assert(graph.get_out_degree(3) == 2);
    }
}

void test_get_nodes(StandardGraph graph)
{

    INFO();
    for (node x : graph.get_nodes())
    {
        CommonUtil::dump_node(x);
    }
}

void test_get_in_nodes_and_edges(StandardGraph graph, bool is_directed)
{
    INFO();
    vector<edge> got_e = graph.get_in_edges(3);
    vector<node> got_n = graph.get_in_nodes(3);
    if (is_directed)
    {
        assert(got_e.size() == 2);
        assert(got_n.size() == 2);
    }
    else
    {
        assert(got_e.size() == 2);
        assert(got_n.size() == 2);
    }
}

void test_get_out_nodes_and_edges(StandardGraph graph, bool is_directed)
{
    INFO();
    vector<edge> got_e = graph.get_out_edges(3);
    vector<node> got_n = graph.get_out_nodes(3);
    if (!is_directed)
    {
        assert(got_e.size() == 2);
        assert(got_n.size() == 2);
    }
    else
    {
        assert(got_e.size() == 0);
        assert(got_n.size() == 0);
    }

    for (edge x : got_e)
    {
        CommonUtil::dump_edge(x);
    }

    for (node x : got_n)
    {
        CommonUtil::dump_node(x);
    }
}

void test_cursor(StandardGraph graph)
{
    INFO();
    for (edge x : graph.test_cursor_iter(2))
    {
        CommonUtil::dump_edge(x);
    }
}

void test_get_edges(StandardGraph graph, bool is_directed)
{

    INFO();
    CommonUtil::dump_edge(graph.get_edge(1, 2));

    vector<edge> edges = graph.get_edges();

    if (!is_directed)
    {
        assert(edges.size() == 6);
    }
    else
    {
        assert(edges.size() == 3);
    }
}

// std::string create_intvec_format_new(std::vector<int> to_pack,
//                                      size_t *total_size)
// {
//     std::string fmt = "I";
//     *total_size = sizeof(uint32_t);

//     for (int x : to_pack)
//     {
//         fmt = fmt + 'i';
//         *total_size = *total_size + sizeof(int32_t);
//     }
//     return fmt;
// }

// char *pack_int_vector_wt_new(std::vector<int> to_pack, WT_SESSION *session,
//                              size_t *size, std::string *fmt)
// {

//     size_t _size = 0;
//     std::string format = create_intvec_format_new(to_pack, &_size);

//     char *buffer = (char *)malloc((_size) * sizeof(int));
//     WT_PACK_STREAM *psp;

//     (void)wiredtiger_pack_start(session, format.c_str(), buffer, *size, &psp);
//     for (int x : to_pack)
//     {
//         (void)wiredtiger_pack_int(psp, x);
//     }
//     *fmt = format;
//     *size = _size;
//     return buffer;
// }

// std::vector<int> unpack_int_vector_wt_new(const char *to_unpack, WT_SESSION *session, string format, size_t size)
// {
//     std::vector<int> unpacked_vector;
//     WT_PACK_STREAM *psp;

//     (void)wiredtiger_unpack_start(session, format.c_str(), to_unpack, size, &psp);
//     for (uint i = 0; i < format.length(); i++)
//     {
//         int64_t res;
//         (void)wiredtiger_unpack_int(psp, &res);
//         unpacked_vector.push_back(res);
//     }
//     return unpacked_vector;
// }

// void test_int_pack()
// {
//     string testname = "Integer Vector Packing";
//     int ret = 0;

//     vector<int> test = {1, 2, 3, 4, 55, 66, 77, 888, 999};
//     size_t size;
//     string format;
//     char *buffer = pack_int_vector_wt_new(test, session, &size, &format);
//     vector<int> result = unpack_int_vector_wt_new(buffer, session, format, size);

//     assert(result == test);
// }

void print_delim()
{
    cout << endl
         << "--------------------" << endl;
}

int main()
{

    graph_opts opts;
    opts.create_new = true;
    opts.optimize_create = false;
    opts.is_directed = true;
    //opts.is_directed = false;
    //opts.read_optimize = false;
    opts.read_optimize = true;
    opts.is_weighted = false;
    opts.db_name = "test_std";

    //Test std_graph setup
    StandardGraph graph = StandardGraph(opts);
    create_init_nodes(graph, opts.is_directed);
    test_get_nodes(graph);
    //tearDown(graph);
    //exit(0);

    //create_init_nodes(graph, opts.is_directed);
    print_delim();

    //Test num_get_nodes and num_get_edges

    test_get_num_nodes_and_edges(graph, opts.is_directed);
    print_delim();

    //Test get_node()
    test_get_node(graph);
    print_delim();

    //Test get_in_degree()
    test_get_in_degree(graph, opts.is_directed);
    print_delim();

    //Test get_out_degree()
    test_get_out_degree(graph, opts.is_directed);
    print_delim();

    //test get_nodes()
    test_get_nodes(graph);
    print_delim();

    //Test adding a node
    test_node_add(graph, opts.read_optimize);
    print_delim();

    //test get_edge_id()
    test_get_edge_id(graph);
    print_delim();

    // test get_edge and get_edges
    test_get_edges(graph, opts.is_directed);
    print_delim();

    //test_cursor(graph);

    //test get_in_nodes_and_edges
    test_get_in_nodes_and_edges(graph, opts.is_directed);
    print_delim();

    //test get_out_nodes_and_edges
    test_get_out_nodes_and_edges(graph, opts.is_directed);
    print_delim();

    //Test deleting a node
    test_node_delete(graph, opts.is_directed);

    //Test std_graph teardown
    tearDown(graph);

    //test common util packing and unpacking features.
    // test_stringvec_packing();
    // test_intvec_packing();

    return 0;
}
