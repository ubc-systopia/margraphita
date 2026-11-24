// dump_edges.cpp — export the edge list of a FlexoGraph DB as plain text.
//
// Usage:
//   dump_edges <db_dir> <db_name> <graph_type> [--directed]
//
//   e.g. dump_edges /drives/ssd2/flexograph_rd/kron/s12_e8 adj_r_s12_e8 adj
//
// Output (stdout):
//   # n=<num_nodes> m=<num_edges> directed=<0|1>
//   <u> <v>
//   <u> <v>
//   ...
//
// For undirected DBs (default), each unordered pair is printed once (u < v).
// The consumer symmetrizes. For directed DBs, every out-edge is printed.
//
// The header line is a comment (#) so consumers that skip comments see only
// edges; consumers that want n/m can parse it.
//
// --features=PATH (added 2026-11-24): also write the DB's node properties (e.g. FastRP
// embeddings stored with fg_fastrp FG_OUT=db) as a float32 .npy matrix of shape
// [max_node_id + 1, dim], row = node id (same indexing as the edge list); nodes without
// properties get zero rows. --features-only skips the edge output.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

#include "common_defs.h"
#include "graph.h"
#include "graph_engine.h"


int main(int argc, char *argv[])
{
    if (argc < 4 || argc > 7) {
        fprintf(stderr,
            "Usage: %s <db_dir> <db_name> <graph_type> [--directed] [--features=OUT.npy] [--features-only]\n"
            "  graph_type  adj | split_ekey\n"
            "  e.g. %s /drives/ssd2/flexograph_rd/kron/s12_e8 adj_r_s12_e8 adj\n",
            argv[0], argv[0]);
        return 1;
    }

    std::string db_dir     = argv[1];
    std::string db_name    = argv[2];
    std::string graph_type = argv[3];
    bool directed = false, features_only = false;
    std::string features_path;
    for (int i = 4; i < argc; ++i) {
        std::string a(argv[i]);
        if (a == "--directed") directed = true;
        else if (a.rfind("--features=", 0) == 0) features_path = a.substr(11);
        else if (a == "--features-only") features_only = true;
        else { fprintf(stderr, "ERROR: unknown option '%s'\n", argv[i]); return 1; }
    }
    if (features_only && features_path.empty()) { fprintf(stderr, "ERROR: --features-only needs --features=PATH\n"); return 1; }

    graph_opts opts;
    opts.create_new      = false;
    opts.optimize_create = true;
    opts.is_directed     = directed;
    opts.is_weighted     = false;
    opts.db_dir          = db_dir;
    opts.db_name         = db_name;
    if (!features_path.empty()) { opts.has_node_props = true; opts.prop_mode = EMBEDDED; }

    if      (graph_type == "adj")        opts.type = GraphType::Adj;
    else if (graph_type == "split_ekey") opts.type = GraphType::SplitEKey;
    else {
        fprintf(stderr, "ERROR: unknown graph_type '%s'\n", graph_type.c_str());
        return 1;
    }

    GraphEngine engine(1, opts);
    GraphBase *g = engine.create_graph_handle();
    if (g == nullptr) {
        fprintf(stderr, "ERROR: could not open graph handle\n");
        return 2;
    }

    if (!features_path.empty()) {
        std::vector<node_id_t> ids;
        NodeCursor *nc = g->get_node_iter();
        nc->set_key_range({0, OutOfBand_ID_MAX});
        node n; nc->next(&n);
        while (n.id != OutOfBand_ID_MAX) { ids.push_back(n.id); nc->next(&n); }
        nc->close(); delete nc;
        node_id_t max_id = 0; for (node_id_t id : ids) max_id = std::max(max_id, id);
        size_t dim = 0, missing = 0, bad = 0;
        std::vector<float> X;
        for (node_id_t id : ids) {
            prop_blob pb = g->get_node_properties(id);
            if (!pb.data || pb.size == 0) { ++missing; continue; }
            if (dim == 0) { dim = pb.size / sizeof(float); X.assign((size_t)(max_id + 1) * dim, 0.0f); }
            if (pb.size != dim * sizeof(float)) ++bad;
            else std::memcpy(&X[(size_t)id * dim], pb.data, pb.size);
            delete[] const_cast<uint8_t *>(pb.data);
        }
        if (dim == 0) { fprintf(stderr, "ERROR: no node properties in %s/%s\n", db_dir.c_str(), db_name.c_str()); return 3; }
        // .npy v1.0: magic, header dict padded with spaces to a 64-byte boundary, '\n'
        std::string hdr = "{'descr': '<f4', 'fortran_order': False, 'shape': (" +
                          std::to_string((unsigned long long)max_id + 1) + ", " + std::to_string(dim) + "), }";
        size_t total = 10 + hdr.size() + 1; hdr.append((64 - total % 64) % 64, ' '); hdr += '\n';
        FILE *f = fopen(features_path.c_str(), "wb");
        if (!f) { fprintf(stderr, "ERROR: cannot write %s\n", features_path.c_str()); return 4; }
        const char magic[] = "\x93NUMPY\x01\x00"; fwrite(magic, 1, 8, f);
        uint16_t hl = (uint16_t)hdr.size(); fwrite(&hl, 2, 1, f);
        fwrite(hdr.data(), 1, hdr.size(), f);
        fwrite(X.data(), sizeof(float), X.size(), f); fclose(f);
        fprintf(stderr, "features: %zu nodes, dim %zu, rows %llu, missing %zu, wrong size %zu -> %s\n",
                ids.size(), dim, (unsigned long long)max_id + 1, missing, bad, features_path.c_str());
        if (features_only) { g->close(); engine.close_graph(); return 0; }
    }

    // First pass: count nodes and edges.
    uint64_t n_nodes = 0, m_edges = 0;
    {
        NodeCursor *nc = g->get_node_iter();
        nc->set_key_range({0, OutOfBand_ID_MAX});
        node n;
        nc->next(&n);
        while (n.id != OutOfBand_ID_MAX) {
            n_nodes++;
            for (node_id_t v : g->get_out_nodes_id(n.id)) {
                if (!directed && n.id >= v) continue;
                m_edges++;
            }
            nc->next(&n);
        }
        nc->close();
        delete nc;
    }

    printf("# n=%llu m=%llu directed=%d\n",
           (unsigned long long)n_nodes,
           (unsigned long long)m_edges,
           directed ? 1 : 0);

    // Second pass: print edges.
    {
        NodeCursor *nc = g->get_node_iter();
        nc->set_key_range({0, OutOfBand_ID_MAX});
        node n;
        nc->next(&n);
        while (n.id != OutOfBand_ID_MAX) {
            for (node_id_t v : g->get_out_nodes_id(n.id)) {
                if (!directed && n.id >= v) continue;
                printf("%llu %llu\n",
                       (unsigned long long)n.id,
                       (unsigned long long)v);
            }
            nc->next(&n);
        }
        nc->close();
        delete nc;
    }

    g->close();
    engine.close_graph();
    return 0;
}