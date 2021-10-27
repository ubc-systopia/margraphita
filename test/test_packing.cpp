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

// std::vector<int> unpack_int_vector(char *to_unpack,
//                                    WT_SESSION *session)
// {
//     std::vector<int> unpacked_vector;
//     WT_PACK_STREAM *psp;
//     const char *fmt = "III";
//     size_t size = 1000000; // for lack of something clever
//     int ret = wiredtiger_unpack_start(session, fmt, to_unpack, size, &psp);
//     const char *format_str;
//     //ret = wiredtiger_unpack_str(psp, &format_str);
//     //cout << "init format is " << format_str << endl;
//     //wiredtiger_pack_close(psp, &size); // To reset the unpacking stream.

//     int64_t res;
//     for (int i = 0; i < 3; i++)
//     {
//         ret = wiredtiger_unpack_int(psp, &res);
//         cout << res << endl;
//         unpacked_vector.push_back(res);
//     }

//     wiredtiger_pack_close(psp, &size);
//     return unpacked_vector;
// }

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

    // std::filesystem::path dirname = "./db/" + string(db_name);
    // if (std::filesystem::exists(dirname))
    // {
    //     filesystem::remove_all(dirname); // remove if exists;
    // }
    // std::filesystem::create_directory(dirname);

    WT_CONNECTION *conn;
    WT_SESSION *session;
    // /* Open a connection to the database, creating it if necessary. */
    wiredtiger_open("./db", NULL, "create", &conn);
    // /* Open a session for the current thread's work. */
    conn->open_session(conn, NULL, NULL, &session);
    // /* Do some work... */
    // /* Note: closing the connection implicitly closes open session(s). */
    // char *buffer = (char *)malloc(1000000);
    // size_t size = 1000000;

    /*
    This is a definitely a problem with the API -- as an application developer, I need to know the size 
    of the buffer needed to hold the packed array before I pack it. 
    */

    WT_PACK_STREAM *psp, *psp1;
    WT_ITEM item, unpacked;
    //4299178224
    //
    void *buf = malloc(sizeof(long) * vec.size());
    // memcpy(buf, vec.data(), sizeof(long) * vec.size());
    // const void *ptr = buf;
    item.data = vec.data();
    item.size = sizeof(long) * vec.size();

    void *pack_buf = malloc(sizeof(long) * vec.size());

    // const char *fmt = "III";
    int ret = wiredtiger_pack_start(session, "u", pack_buf,
                                    sizeof(long) * vec.size(), &psp);

    wiredtiger_pack_item(psp, &item);
    size_t used;
    wiredtiger_pack_close(psp, &used);

    wiredtiger_unpack_start(session, "u", pack_buf, used, &psp1);
    wiredtiger_unpack_item(psp1, &unpacked);

        // for (int x : vect)
    // {
    //     ret = wiredtiger_pack_int(psp, x);
    // }
    // size_t used;
    // cout << "used : " << used << endl;
    // wiredtiger_pack_close(psp, &used);

    // vector<int> unpack = unpack_int_vector(buffer, session);

    // conn->close(conn, NULL);
    return 0;
}
