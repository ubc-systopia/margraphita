//
// Created by puneet on 22/01/24.
//

#include <wiredtiger.h>
#include <cassert>
#include <thread>
#include <iostream>
static const char* home = "WT_HOME";
WT_CONNECTION *conn;


static void
access_example(int init)
{
    /*! [access example connection] */

    WT_CURSOR *cursor;
    WT_SESSION *session;
    int ret;

    /* Open a session handle for the database. */
    conn->open_session(conn, NULL, NULL, &session);

    /*! [access example table create] */
    session->create(session, "table:access", "key_format=I,value_format=I");
    /*! [access example table create] */

    /*! [access example cursor open] */
    ret = session->open_cursor(session, "table:access", NULL, "bulk", &cursor);
    std::cout << "error: " << wiredtiger_strerror(ret) << std::endl;
    assert(ret == 0);
    /*! [access example cursor open] */

//    /*! [access example cursor insert] */
//    for(int i=init; i<init+100; i++) {
//            cursor->set_key(cursor, i);
//            cursor->set_value(cursor, i);
//            cursor->insert(cursor);
//    }

    cursor->close(cursor); /* Close all handles. */
                                          /*! [access example close] */
}

int main(int argc, char *argv[])
{
    wiredtiger_open(home, NULL, "create", &conn);
    std::thread t1(access_example,0);
    std::thread t2(access_example,200);

    t1.join();
    t2.join();
    conn->close(conn, NULL);
    return 0;
}
