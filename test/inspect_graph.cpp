// inspect_graph.cpp — inspect adjacency structure of a golden image DB.
// Usage: inspect_graph <db_dir> [adj|splitekey] [--embedded] [--db-name=NAME]

#include <cstdio>
#include <cstring>
#include <string>

#include "common_defs.h"
#include "graph_engine.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <db_dir> [adj|splitekey] [--embedded] [--db-name=NAME]\n", argv[0]);
        return 1;
    }

    std::string db_dir  = argv[1];
    GraphType   gtype   = GraphType::Adj;
    bool        emb     = false;
    std::string db_name = "snb";

    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if      (a == "adj")                     gtype   = GraphType::Adj;
        else if (a == "splitekey")               gtype   = GraphType::SplitEKey;
        else if (a == "--embedded")              emb     = true;
        else if (a.rfind("--db-name=", 0) == 0) db_name = a.substr(10);
    }

    graph_opts opts;
    opts.create_new     = false;
    opts.is_directed    = true;
    opts.read_optimize  = true;
    opts.has_node_props = true;
    opts.has_edge_props = true;
    opts.prop_mode      = emb ? EMBEDDED : COLUMNAR;
    opts.type           = gtype;
    opts.db_name        = db_name;
    opts.db_dir         = db_dir;
    opts.conn_config    = "cache_size=2GB";
    opts.stat_log       = "./";

    GraphEngine engine(1, opts);
    GraphBase  *g = engine.create_graph_handle();

    // ── Person 0: out- and in-neighbor type breakdown ───────────────────────
    node_id_t pid0 = MAKE_TYPED_ID(VT_PERSON, 0);
    fprintf(stderr, "[inspect] pid0 = 0x%llx (type=%u counter=%llu)\n",
            (unsigned long long)pid0, (unsigned)VTYPE_OF(pid0),
            (unsigned long long)VCOUNTER_OF(pid0));

    auto count_by_type = [](const std::vector<node_id_t> &nbrs) {
        int p=0, post=0, com=0, forum=0, other=0;
        for (auto nb : nbrs) {
            switch (VTYPE_OF(nb)) {
                case VT_PERSON:  p++;     break;
                case VT_POST:    post++;  break;
                case VT_COMMENT: com++;   break;
                case VT_FORUM:   forum++; break;
                default:         other++; break;
            }
        }
        fprintf(stderr, "  total=%zu persons=%d posts=%d comments=%d forums=%d other=%d\n",
                nbrs.size(), p, post, com, forum, other);
    };

    fprintf(stderr, "[inspect] person0 out-neighbors:\n");
    count_by_type(g->get_out_nodes_id(pid0));
    fprintf(stderr, "[inspect] person0 out-person-neighbors:\n");
    for (auto nb : g->get_out_nodes_id(pid0))
        if (VTYPE_OF(nb) == VT_PERSON)
            fprintf(stderr, "  -> person counter=%llu (typed_id=0x%llx)\n",
                    (unsigned long long)VCOUNTER_OF(nb), (unsigned long long)nb);

    fprintf(stderr, "[inspect] person0 in-neighbors:\n");
    count_by_type(g->get_in_nodes_id(pid0));
    fprintf(stderr, "[inspect] person0 in-person-neighbors:\n");
    for (auto nb : g->get_in_nodes_id(pid0))
        if (VTYPE_OF(nb) == VT_PERSON)
            fprintf(stderr, "  <- person counter=%llu (typed_id=0x%llx)\n",
                    (unsigned long long)VCOUNTER_OF(nb), (unsigned long long)nb);

    // ── First comment: out-neighbors ────────────────────────────────────────
    node_id_t c0 = MAKE_TYPED_ID(VT_COMMENT, 0);
    auto c0_out = g->get_out_nodes_id(c0);
    fprintf(stderr, "[inspect] comment0 out-neighbors (%zu):\n", c0_out.size());
    for (auto nb : c0_out)
        fprintf(stderr, "  -> type=%u counter=%llu\n",
                (unsigned)VTYPE_OF(nb), (unsigned long long)VCOUNTER_OF(nb));

    // ── First message authored by person0, and its in-neighbors ─────────────
    fprintf(stderr, "[inspect] scanning in-neighbors of person0 for messages:\n");
    int msg_cnt = 0;
    for (node_id_t nb : g->get_in_nodes_id(pid0)) {
        if (VTYPE_OF(nb) != VT_POST && VTYPE_OF(nb) != VT_COMMENT) continue;
        if (++msg_cnt > 3) break;  // show first 3 messages only
        fprintf(stderr, "  msg type=%u counter=%llu\n",
                (unsigned)VTYPE_OF(nb), (unsigned long long)VCOUNTER_OF(nb));
        auto msg_in = g->get_in_nodes_id(nb);
        int reply_cnt = 0;
        for (auto r : msg_in) {
            if (++reply_cnt > 3) break;
            fprintf(stderr, "    reply type=%u counter=%llu\n",
                    (unsigned)VTYPE_OF(r), (unsigned long long)VCOUNTER_OF(r));
            auto r_out = g->get_out_nodes_id(r);
            for (auto ro : r_out)
                fprintf(stderr, "      reply->out type=%u counter=%llu\n",
                        (unsigned)VTYPE_OF(ro), (unsigned long long)VCOUNTER_OF(ro));
        }
    }

    return 0;
}
// Note: additional BFS debug code appended at bottom — never executed from main().
// Left here for reference.
