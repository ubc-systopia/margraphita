#include <iostream>
#include <string>
#include "wiredtiger.h"

static const char *home;
using namespace std;

int main (int argc, char** argv){
    
    WT_CONNECTION *conn;
    WT_CURSOR *cursor;
    WT_SESSION *session;
    const char *key, *value;
    int ret;
    /* Open a connection to the database, creating it if necessary. */
    ret = wiredtiger_open(home, NULL, "create", &conn);
    if (ret !=0){
        fprintf(stderr, "Failed to open connection\n");
        return (ret);
    }
    /* Open a session handle for the database. */
    ret = conn->open_session(conn, NULL, NULL, &session);
    if (ret !=0){
        fprintf(stderr, "Failed to open session\n");
        return (ret);
    }
}