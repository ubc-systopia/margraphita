#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>
#include <random>

#include "common.h"
#include "graph.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "times.h"
#define THREAD_NUM 16

WT_CONNECTION *conn;
WT_CURSOR *cursor;
WT_SESSION *session;
const char *home;
omp_lock_t edge_num_lock;
uint64_t edge_num;

void _setup()
{
    // Create new directory for WT DB
    std::string dirname = "./db/pack_test";
    CommonUtil::create_dir(dirname);
    omp_init_lock(&edge_num_lock);

    wiredtiger_open(dirname.c_str(), NULL, "create", &conn);
    conn->open_session(conn, nullptr, nullptr, &session);

    CommonUtil::set_table(session, "edge", {"id", "src", "dst"}, "I", "II");
}

void insert()
{
    WT_CURSOR *edge_cur;
    GraphBase::_get_table_cursor("edge", &edge_cur, session, false, false);
    for (int i = 0; i < 100000000; i++)
    {

//        CommonUtil::set_key(edge_cur, i);
        edge_cur->set_key(edge_cur, i);
        edge_cur->set_value(edge_cur, 0, 0);
        edge_cur->insert(edge_cur);

        if (i % (int)10000 == 0)
        {
            cout << "inserted another 10000%\n";
        }
    }
    edge_num = 1e8;
    session->close(session, nullptr);
}

/**
void insert_test_locks()
{
#pragma omp parallel for num_threads(THREAD_NUM)
    for (int t = 0; t < THREAD_NUM; t++)
    {
        WT_SESSION *session;
        conn->open_session(conn, nullptr, nullptr, &session);
        WT_CURSOR *edge_cur;
        GraphBase::_get_table_cursor("edge", &edge_cur, session, false, false);
        for (int i = 0; i < 1e5; i++)
        {
            omp_set_lock(&edge_num_lock);
            uint64_t edgeno = edge_num;
            edge_num += 1;
            omp_unset_lock(&edge_num_lock);

            session->begin_transaction(session, "isolation=snapshot");
            CommonUtil::set_key(edge_cur, edgeno);
            edge_cur->set_value(edge_cur, 0, 0);
            edge_cur->insert(edge_cur);

            session->commit_transaction(session, nullptr);
        }
        session->close(session, nullptr);
    }
}

void insert_test_lockfree()
{
#pragma omp parallel for num_threads(THREAD_NUM)
    for (int t = 0; t < THREAD_NUM; t++)
    {
        WT_SESSION *session;
        conn->open_session(conn, nullptr, nullptr, &session);
        WT_CURSOR *edge_cur;
        GraphBase::_get_table_cursor("edge", &edge_cur, session, false, false);

        for (int i = 0; i < 1e5; i++)
        {
            session->begin_transaction(session, "isolation=snapshot");
            CommonUtil::set_key(edge_cur, t * 1e5 + i * 2 + 1);
            edge_cur->set_value(edge_cur, 0, 0);
            edge_cur->insert(edge_cur);
            session->commit_transaction(session, nullptr);
        }
        session->close(session, nullptr);
    }
}
**/
void _close()
{
    conn->close(conn, nullptr);
    // CommonUtil::remove_dir("./db/common_test");
}

int main()
{
    _setup();
    Times t;
    t.start();
    insert();
    t.stop();
    cout << "Graph loaded in " << t.t_micros() << endl;

    // t.start();
    // insert_test_lockfree();
    // t.stop();
    // cout << "Lockfree: " << t.t_micros() << endl;

    // t.start();
    // insert_test_locks();
    // t.stop();
    // cout << "Lock: " << t.t_micros() << endl;

    _close();
    return 0;
}