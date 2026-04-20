// add_typed_edge_table.cpp — standalone tool to add a per-type edge table
// to an existing Flexograph database.
//
// Creates a WiredTiger table `post_hascreator` with key=(person_tid, post_tid).
// This key order enables:
//   - x3 (IC-2):  range scan on (friend, *) to find all posts by a friend
//   - bi12:       full sequential scan; posts arrive grouped by creator
//
// Populates the table by scanning the existing mixed-type edge structure
// and extracting all post→person (hasCreator) edges.  Runs on an already-built
// database — no CSV or bulk loader changes needed.
//
// Usage:
//   add_typed_edge_table <db_dir> [adj|splitekey] [--embedded]
//                        [--db-name=NAME]

#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

#include "adj_list.h"
#include "common_defs.h"
#include "edgekey_split.h"
#include "graph_engine.h"
#include "prop_schema.h"

static const std::string POST_HASCREATOR_TABLE = "post_hascreator";

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <db_dir> [adj|splitekey] [--embedded] [--db-name=NAME]\n", argv[0]);
        return 1;
    }

    std::string db_dir  = argv[1];
    GraphType   gtype   = GraphType::Adj;
    bool        emb     = false;
    std::string db_name = "snb";

    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if      (a == "adj")                       gtype   = GraphType::Adj;
        else if (a == "splitekey")                  gtype   = GraphType::SplitEKey;
        else if (a == "--embedded")                 emb     = true;
        else if (a.rfind("--db-name=", 0) == 0)    db_name = a.substr(10);
        else { fprintf(stderr, "Unknown arg: %s\n", argv[i]); return 1; }
    }

    // ---- Open DB (read-write) ----
    graph_opts opts;
    opts.create_new      = false;
    opts.is_directed     = true;
    opts.read_optimize   = true;
    opts.has_node_props  = true;
    opts.has_edge_props  = true;
    opts.prop_mode       = emb ? EMBEDDED : COLUMNAR;
    opts.type            = gtype;
    opts.db_name         = db_name;
    opts.db_dir          = db_dir;
    opts.conn_config     = "cache_size=4GB";
    opts.stat_log        = "./";

    const char *gtype_str = (gtype == GraphType::Adj) ? "adj" : "splitekey";
    const char *mode_str  = emb ? "emb" : "col";
    fprintf(stderr, "[add_typed_edge_table] db=%s type=%s mode=%s\n",
            db_dir.c_str(), gtype_str, mode_str);

    GraphEngine engine(1, opts);
    GraphBase  *gp = engine.create_graph_handle();
    GraphBase  &g  = *gp;

    // ---- Create the post_hascreator table via raw WT session ----
    // Key = (person_tid, post_tid): enables per-person range scans (x3)
    // and grouped-by-creator full scans (bi12).
    WT_CONNECTION *conn = engine.get_connection();
    WT_SESSION *sess;
    int ret = conn->open_session(conn, nullptr, nullptr, &sess);
    if (ret != 0) {
        fprintf(stderr, "Cannot open session: %s\n", wiredtiger_strerror(ret));
        return 1;
    }

    // Drop if exists (idempotent re-runs)
    sess->drop(sess, ("table:" + POST_HASCREATOR_TABLE).c_str(), "force");

    ret = sess->create(sess, ("table:" + POST_HASCREATOR_TABLE).c_str(),
        "key_format=QQ,value_format=x,"
        "columns=(person_id,post_id,dummy),"
        "leaf_page_max=64KB,"
        "internal_page_max=16KB");
    if (ret != 0) {
        fprintf(stderr, "Cannot create table: %s\n", wiredtiger_strerror(ret));
        return 1;
    }
    fprintf(stderr, "[add_typed_edge_table] Created table: %s (key=person,post)\n",
            POST_HASCREATOR_TABLE.c_str());

    // ---- Open a cursor on the new table ----
    WT_CURSOR *ins_cur;
    ret = sess->open_cursor(sess, ("table:" + POST_HASCREATOR_TABLE).c_str(),
                            nullptr, nullptr, &ins_cur);
    if (ret != 0) {
        fprintf(stderr, "Cannot open insert cursor: %s\n", wiredtiger_strerror(ret));
        return 1;
    }

    // ---- Scan all post→person edges from existing edge structure ----
    auto t0 = std::chrono::steady_clock::now();

    EdgeCursor *ec = g.get_edge_iter();
    // Range: all edges with src in the post type range
    ec->set_key_range({{MAKE_TYPED_ID(VT_POST, 0), 1},
                       {MAKE_TYPED_ID(VT_POST + 1, 0), 0}});
    edge found;
    ec->next(&found);
    size_t count = 0;
    size_t scanned = 0;

    while (found.src_id != OutOfBand_ID_MAX) {
        if (VTYPE_OF(found.src_id) != VT_POST) break;
        scanned++;
        if (VTYPE_OF(found.dst_id) == VT_PERSON) {
            // Insert as (person_tid, post_tid) — reversed from the edge direction
            ins_cur->set_key(ins_cur, (uint64_t)found.dst_id, (uint64_t)found.src_id);
            ins_cur->set_value(ins_cur, (uint8_t)0);
            ret = ins_cur->insert(ins_cur);
            if (ret != 0) {
                fprintf(stderr, "Insert failed at edge %zu: %s\n",
                        count, wiredtiger_strerror(ret));
                break;
            }
            count++;
        }
        ec->next(&found);
    }
    delete ec;

    auto t1 = std::chrono::steady_clock::now();
    double secs = std::chrono::duration<double>(t1 - t0).count();

    fprintf(stderr, "[add_typed_edge_table] Scanned %zu post edges, "
            "inserted %zu postHasCreator entries as (person,post) (%.1f s)\n",
            scanned, count, secs);

    // ---- Checkpoint and close ----
    ins_cur->close(ins_cur);
    ret = sess->checkpoint(sess, nullptr);
    if (ret != 0)
        fprintf(stderr, "Checkpoint failed: %s\n", wiredtiger_strerror(ret));
    else
        fprintf(stderr, "[add_typed_edge_table] Checkpoint done\n");

    sess->close(sess, nullptr);
    gp->close(false);

    fprintf(stderr, "[add_typed_edge_table] Done. Table '%s' has %zu rows.\n",
            POST_HASCREATOR_TABLE.c_str(), count);
    return 0;
}
