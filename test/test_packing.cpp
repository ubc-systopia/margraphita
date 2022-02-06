#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <iterator>
#include <wiredtiger.h>
#include <stdlib.h>
#include <chrono>
using namespace std;
static const char *db_name = "test";

static char *pack_int_vector_wti(WT_SESSION *session, std::vector<int> to_pack, size_t *size);
static std::vector<int> unpack_int_vector_wti(WT_SESSION *session, size_t size, std::string packed_str);

std::vector<int> unpack_int_vector_wti(WT_SESSION *session, size_t size, char *packed_str)
{
    WT_PACK_STREAM *psp;
    WT_ITEM unpacked;
    size_t used;
    wiredtiger_unpack_start(session, "u", packed_str, size, &psp);
    wiredtiger_unpack_item(psp, &unpacked);
    wiredtiger_pack_close(psp, &used);
    std::vector<int> unpacked_vec;

    int vec_size = (int)size / sizeof(int);
    for (int i = 0; i < vec_size; i++)
        unpacked_vec.push_back(((int *)unpacked.data)[i]);
    return unpacked_vec;
}

char *pack_int_vector_wti(WT_SESSION *session, std::vector<int> to_pack, size_t *size)
{
    WT_PACK_STREAM *psp;
    WT_ITEM item;
    item.data = to_pack.data();
    item.size = sizeof(int) * to_pack.size();

    void *pack_buf = malloc(sizeof(int) * to_pack.size());
    int ret = wiredtiger_pack_start(session, "u", pack_buf,
                                    item.size, &psp);

    wiredtiger_pack_item(psp, &item);
    wiredtiger_pack_close(psp, size);

    return (char *)pack_buf;
}

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

    vector<int> vec(10000);
    generate(begin(vec), end(vec), gen);

    // for (long x : vec)
    //     cout << x << endl;

    WT_CONNECTION *conn;
    WT_SESSION *session;
    // /* Open a connection to the database, creating it if necessary. */
    wiredtiger_open("./db", NULL, "create", &conn);
    conn->open_session(conn, NULL, NULL, &session);

    /*
    This is a definitely a problem with the API -- as an application developer, I need to know the size 
    of the buffer needed to hold the packed array before I pack it. 
    */
    auto start = std::chrono::steady_clock::now();
    // WT_PACK_STREAM *psp, *psp1;
    // WT_ITEM item;
    // item.data = vec.data();
    // item.size = sizeof(long) * vec.size();

    // void *pack_buf = malloc(sizeof(long) * vec.size());

    // int vec_size = sizeof(long) * vec.size();
    // int ret = wiredtiger_pack_start(session, "u", pack_buf,
    //                                 vec_size, &psp);

    // wiredtiger_pack_item(psp, &item);
    // size_t used;
    // wiredtiger_pack_close(psp, &used);
    size_t size;
    char *buf = pack_int_vector_wti(session, vec, &size);

    auto end = std::chrono::steady_clock::now();
    std::cout << "packed in : "
              << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
              << "\n";
    std::cout << "Size used = " << size << "; Size of long " << sizeof(int) << "; size of vec " << vec.size() << "\n";

    start = std::chrono::steady_clock::now();

    // //----------------
    // WT_ITEM unpacked;
    // int64_t size_val;

    // wiredtiger_unpack_start(session, "u", pack_buf, used, &psp1);
    // wiredtiger_unpack_item(psp1, &unpacked);
    // wiredtiger_pack_close(psp1, &used);

    // std::cout << "size_val : " << size_val << "\nbuf_size: " << unpacked.size << "\nused = " << used;

    // std::vector<long> unpacked_vec;
    // for (int i = 0; i < vec.size(); i++)
    //     unpacked_vec.push_back(((long *)unpacked.data)[i]);
    // // // cout << ((long *)unpacked.data)[i] << "\t";
    // // // return 0;

    std::vector<int> unpacked_vec = unpack_int_vector_wti(session, size, buf);
    end = std::chrono::steady_clock::now();

    vec == unpacked_vec ? std::cout << "true" : std::cout << "false";
    std::cout << "unpacked in : "
              << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
              << "\n";
}
