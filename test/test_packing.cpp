#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <iterator>
#include <wiredtiger.h>
#include <stdlib.h>
using namespace std;
static const char *db_name = "test";

int main()
{
    // Create an empty vector
    vector<int> vect;

    random_device rnd_device;
    // Specify the engine and distribution.
    mt19937 mersenne_engine{rnd_device()}; // Generates random integers
    uniform_int_distribution<long> dist{2147483647, 3147483647};

    auto gen = [&dist, &mersenne_engine]()
    {
        return dist(mersenne_engine);
    };

    vector<long> vec(10);
    generate(begin(vec), end(vec), gen);

    for (long x : vec)
        cout << x << endl;

    WT_CONNECTION *conn;
    WT_SESSION *session;
    // /* Open a connection to the database, creating it if necessary. */
    wiredtiger_open("./db", NULL, "create", &conn);
    conn->open_session(conn, NULL, NULL, &session);

    /*
    This is a definitely a problem with the API -- as an application developer, I need to know the size 
    of the buffer needed to hold the packed array before I pack it. 
    */
    WT_PACK_STREAM *psp, *psp1;
    WT_ITEM item;
    item.data = vec.data();
    item.size = sizeof(long) * vec.size();

    void *pack_buf = malloc(sizeof(long) * vec.size());

    int ret = wiredtiger_pack_start(session, "u", pack_buf,
                                    sizeof(long) * vec.size(), &psp);

    wiredtiger_pack_item(psp, &item);
    size_t used;
    wiredtiger_pack_close(psp, &used);

    WT_ITEM unpacked;
    wiredtiger_unpack_start(session, "u", pack_buf, used, &psp1);
    wiredtiger_unpack_item(psp1, &unpacked);
    cout << "\n"
         << used << endl;
    for (int i = 0; i < vec.size(); i++)
        cout << ((long *)unpacked.data)[i] << "\t";
    return 0;
}
