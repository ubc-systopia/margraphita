#include <wiredtiger.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "common_util.h"
#include "edgekey.h"

#define error_check(call)                                                \
    do                                                                   \
    {                                                                    \
        int __r;                                                         \
        if ((__r = (call)) != 0 && __r != ENOTSUP)                       \
            testutil_die(                                                \
                __r, "%s/%d: %s", __PRETTY_FUNCTION__, __LINE__, #call); \
    } while (0)

void testutil_die(int e, const char *fmt, ...)
{
    va_list ap;

    /* Flush output to be sure it doesn't mix with fatal errors. */
    (void)fflush(stdout);
    (void)fflush(stderr);

    fprintf(stderr, "%s: FAILED", __FILE__);
    if (fmt != NULL)
    {
        fprintf(stderr, ": ");
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
    if (e != 0) fprintf(stderr, ": %s", wiredtiger_strerror(e));
    fprintf(stderr, "\n");
    (void)fflush(stderr);

    /* Drop core. */
    fprintf(stderr, "%s: process aborting\n", __FILE__);
}

int main(int argc, char *argv[])
{
    WT_CONNECTION *conn;
    WT_CURSOR *cursor;
    WT_SESSION *session;

    const char *home = "WT_HOME";
    std::filesystem::create_directories(home);

    /* Open a connection to the database, creating it if necessary. */
    error_check(wiredtiger_open(home, NULL, "create,statistics=(fast)", &conn));

    /* Open a session for the current thread's work. */
    error_check(conn->open_session(conn, NULL, NULL, &session));

    error_check(session->create(session,
                                "table:world",
                                "key_format=ii,value_format=II,columns=(src,"
                                "dst,indeg, outdeg)"));

    error_check(
        session->create(session, "index:world:srcdst", "columns=(src,dst)"));

    error_check(
        session->open_cursor(session, "table:world", NULL, NULL, &cursor));

    for (int i = 0; i < 10; i++)
    {
        cursor->set_key(cursor, i, (5 * i));
        cursor->set_value(cursor, i + 10, i + 20);
        error_check(cursor->insert(cursor));
    }

    error_check(session->open_cursor(
        session, "index:world:srcdst", NULL, NULL, &cursor));

    int src, dst;

    while ((cursor->next(cursor)) == 0)
    {
        int a, b, c, d;
        error_check(cursor->get_key(cursor, &a, &b));
        error_check(cursor->get_value(cursor, &c, &d));
        printf("(%d, %d) : (%d, %d) \n", a, b, c, d);
    }
    cursor->close(cursor);

    error_check(
        session->create(session, "index:world:srcdst", "columns=(src,dst)"));

    error_check(session->open_cursor(
        session, "file:world_dstsrc.wti", NULL, NULL, &cursor));
    error_check(cursor->largest_key(cursor));
    error_check(cursor->get_key(cursor, &src, &dst));
    printf("largest key is (%d, %d) \n", src, dst);
    cursor->close(cursor);

    error_check(conn->close(conn, NULL));
    return 0;
    error_check(conn->close(conn, NULL));

    return (EXIT_SUCCESS);
}
