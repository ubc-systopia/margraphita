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
#define HASH_CODE 0x1a7aade5878a2539

WT_CONNECTION *conn;
WT_CURSOR *cursor;
WT_SESSION *session;
const char *home;
omp_lock_t edge_num_lock;
uint64_t edge_num;
uint64_t range[100];

void __setup_non_hash()
{
    // Create new directory for WT DB
    std::string dirname = "./db/common_test";
    CommonUtil::create_dir(dirname);
    omp_init_lock(&edge_num_lock);

    wiredtiger_open(dirname.c_str(), NULL, "create", &conn);
    conn->open_session(conn, NULL, NULL, &session);

    CommonUtil::set_table(session, "edge", {"src", "dst", "val"}, "II", "I");
}

void insert_non_hash()
{
    WT_CURSOR *edge_cur;
    GraphBase::_get_table_cursor("edge", &edge_cur, session, false, false);

    for (uint32_t i = 0; i < 1e7; i++)
    {
        edge_cur->set_key(edge_cur, i, (int)1e7 - i);
        edge_cur->set_value(edge_cur, 1);
        int32_t a = 0, b = 0;
        edge_cur->insert(edge_cur);
        if (i % (int)1e7 == 0)
        {
            cout << "inserted another 10%\n";
        }
    }
    edge_num = 1e8;
    session->close(session, NULL);
}

void read_non_hash()
{
    conn->open_session(conn, NULL, NULL, &session);
    WT_CURSOR *edge_cur;
    GraphBase::_get_table_cursor("edge", &edge_cur, session, false, false);
    edge_cur->next(edge_cur);
    for (uint32_t i = 0; i < 1e7; i++)
    {
        uint32_t a = 0, b = 0;
        edge_cur->get_key(edge_cur, &a, &b);
        edge_cur->next(edge_cur);
    }
    session->close(session, NULL);
}

void __setup_hash()
{
    // Create new directory for WT DB
    std::string dirname = "./db/common_test";
    CommonUtil::create_dir(dirname);
    omp_init_lock(&edge_num_lock);

    wiredtiger_open(dirname.c_str(), NULL, "create", &conn);
    conn->open_session(conn, NULL, NULL, &session);

    CommonUtil::set_table(session, "edge", {"srcdst", "val"}, "u", "I");
}

void insert_hash()
{
    WT_CURSOR *edge_cur;
    GraphBase::_get_table_cursor("edge", &edge_cur, session, false, false);
    for (int i = 0; i < 20 * 1e6; i++)
    {
        uint32_t src = i;
        uint32_t dst = (uint32_t)(20 * 1e6) - i;
        uint64_t byte = ((src << 31) + dst);
        WT_ITEM k = {.data = &byte, .size = sizeof(byte)};
        edge_cur->set_key(edge_cur, &k);
        edge_cur->set_value(edge_cur, 1);
        edge_cur->insert(edge_cur);
        if (i % (int)5e6 == 0)
        {
            cout << "inserted another 10%\n";
        }
    }
    edge_num = 1e8;
    session->close(session, NULL);
}

void paritition_hash()
{
    conn->open_session(conn, NULL, NULL, &session);
    WT_CURSOR *edge_cur;
    GraphBase::_get_table_cursor("edge", &edge_cur, session, false, false);
    edge_cur->next(edge_cur);
    int t = 0;
    for (int i = 0; i < 20 * 1e6; i++)
    {
        uint64_t key;
        if (i % (int)1e6 == 0)
        {
            edge_cur->get_key(edge_cur, &key);
            range[t] = key;
            t++;
        }
        if (i % (int)5e6 == 0)
        {
            cout << "partitioned another 10%\n";
        }
    }
    edge_num = 1e8;
    session->close(session, NULL);
}

void read_hash()
{
#pragma omp parallel for num_threads(THREAD_NUM)
    for (int t = 0; t < THREAD_NUM; t++)
    {
        WT_SESSION *session;
        conn->open_session(conn, NULL, NULL, &session);
        WT_CURSOR *edge_cur;
        GraphBase::_get_table_cursor("edge", &edge_cur, session, false, false);
        edge_cur->set_key(edge_cur, range[t]);
        for (int i = 0; i < 1e6; i++)
        {
            uint64_t a;
            WT_ITEM k;
            edge_cur->get_key(edge_cur, &k);
            // edge_cur->get_key(edge_cur, &a);
            edge_cur->next(edge_cur);
        }
        session->close(session, NULL);
    }
}

void __close()
{
    conn->close(conn, NULL);
    // CommonUtil::remove_dir("./db/common_test");
}

int main()
{
    Times t;
    __setup_non_hash();

    t.start();
    insert_non_hash();
    t.stop();
    cout << "Insert non_hash: " << t.t_micros() << endl;
    t.start();
    read_non_hash();
    t.stop();
    cout << "Read non_hash: " << t.t_micros() << endl;
    __close();

    __setup_hash();
    t.start();
    insert_hash();
    t.stop();
    cout << "Insert hash: " << t.t_micros() << endl;

    paritition_hash();

    t.start();
    read_hash();
    t.stop();
    cout << "Read hash: " << t.t_micros() << endl;
    __close();
    return 0;
}