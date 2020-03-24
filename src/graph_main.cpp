#include <iostream>
#include <string>
#include "wiredtiger.h"

static const char *home;
int main (int argc, char** argv){
    std::cout<<"This is dummy main file for testing and writing the Makefile."<<std::endl;
    WT_CONNECTION *conn;
    WT_CURSOR *cursor;
    WT_SESSION *session;
    const char *key, *value;
    int ret;
    /* Open a connection to the database, creating it if necessary. */
    ret = wiredtiger_open(home, NULL, "create", &conn);
    /* Open a session handle for the database. */
    ret = conn->open_session(conn, NULL, NULL, &session);
}