#include <stdlib.h>
#include <wiredtiger.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <vector>
using namespace std;
static const char *db_name = "test";

static char *pack_int_vector_wti(WT_SESSION *session,
                                 std::vector<int> to_pack,
                                 size_t *size);
static std::vector<int> unpack_int_vector_wti(WT_SESSION *session,
                                              size_t size,
                                              std::string packed_str);

std::vector<int> unpack_int_vector_wti(WT_SESSION *session,
                                       size_t size,
                                       char *packed_str)
{
    WT_PACK_STREAM *psp;
    WT_ITEM unpacked;
    size_t used;
    wiredtiger_unpack_start(session, "u", packed_str, size, &psp);
    wiredtiger_unpack_item(psp, &unpacked);
    wiredtiger_pack_close(psp, &used);
    std::vector<int> unpacked_vec;

    int vec_size = (int)size / sizeof(int);
    unpacked_vec.assign((int *)unpacked.data, (int *)unpacked.data + vec_size);
    return unpacked_vec;
}

std::vector<int> unpack_int_vector_wti_pushback(WT_SESSION *session,
                                                size_t size,
                                                char *packed_str)
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

std::vector<int> unpack_int_vector_wti_emplaceback(WT_SESSION *session,
                                                   size_t size,
                                                   char *packed_str)
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
        unpacked_vec.emplace_back(((int *)unpacked.data)[i]);
    return unpacked_vec;
}

char *pack_int_vector_wti(WT_SESSION *session,
                          std::vector<int> to_pack,
                          size_t *size)
{
    WT_PACK_STREAM *psp;
    WT_ITEM item;
    item.data = to_pack.data();
    item.size = sizeof(int) * to_pack.size();

    void *pack_buf = malloc(sizeof(int) * to_pack.size());
    int ret = wiredtiger_pack_start(session, "u", pack_buf, item.size, &psp);

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
    mt19937 mersenne_engine{rnd_device()};  // Generates random integers
    uniform_int_distribution<long> dist{2147483647, 3147483647};

    auto gen = [&dist, &mersenne_engine]() { return dist(mersenne_engine); };

    std::vector<int> vec(1000000);
    std::generate(begin(vec), end(vec), gen);

    WT_CONNECTION *conn;
    WT_SESSION *session;
    // /* Open a connection to the database, creating it if necessary. */
    wiredtiger_open("./db", NULL, "create", &conn);
    conn->open_session(conn, NULL, NULL, &session);

    auto start = std::chrono::steady_clock::now();

    size_t size;
    char *buf = pack_int_vector_wti(session, vec, &size);

    auto end = std::chrono::steady_clock::now();
    std::cout << "packed in : "
              << std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                       start)
                     .count()
              << "\n";
    std::cout << "Size used = " << size << "; Size of long " << sizeof(int)
              << "; size of vec " << vec.size() << "\n";

    std::cout << "--------------------------------------------\n" << std::endl;

    start = std::chrono::steady_clock::now();
    std::vector<int> unpacked_vec = unpack_int_vector_wti(session, size, buf);
    end = std::chrono::steady_clock::now();
    std::cout << "Assign unpacked in : "
              << std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                       start)
                     .count()
              << "\n";

    std::cout << "--------------------------------------------\n" << std::endl;
    start = std::chrono::steady_clock::now();
    std::vector<int> unpacked_vec1 =
        unpack_int_vector_wti_pushback(session, size, buf);
    end = std::chrono::steady_clock::now();
    std::cout << "pushback in : "
              << std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                       start)
                     .count()
              << "\n";

    std::cout << "--------------------------------------------\n" << std::endl;

    start = std::chrono::steady_clock::now();
    std::vector<int> unpacked_vec2 =
        unpack_int_vector_wti_emplaceback(session, size, buf);
    end = std::chrono::steady_clock::now();
    std::cout << "Emplace in : "
              << std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                       start)
                     .count()
              << "\n";
    std::cout << "--------------------------------------------\n" << std::endl;

    std::equal(vec.begin(), vec.end(), unpacked_vec.begin())
        ? std::cout << "Assign - true\n"
        : std::cout << "Assign - false\n";
    // vec == unpacked_vec1 ? std::cout << "Pushback -true\n"
    //                      : std::cout << "Pushback -false\n";

    std::equal(vec.begin(), vec.end(), unpacked_vec1.begin())
        ? std::cout << "Pushback - true\n"
        : std::cout << "Pushback - false\n";

    // vec == unpacked_vec2 ? std::cout << "Emplace- true\n"
    //                      : std::cout << "Emplace  - false\n";

    std::equal(vec.begin(), vec.end(), unpacked_vec2.begin())
        ? std::cout << "Emplace - true\n"
        : std::cout << "Emplace - false\n";
    // vec == unpacked_vec ? std::cout << "Assign- true\n"
    //                     : std::cout << "Assign  - false\n";
}
