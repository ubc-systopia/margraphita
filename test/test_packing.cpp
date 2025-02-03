#include <stdlib.h>
#include <wiredtiger.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

using namespace std;
// static const char *db_name = "test";

static char *pack_int_vector_wti(WT_SESSION *session,
                                 std::vector<int> to_pack,
                                 size_t *size);

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
  wiredtiger_pack_start(session, "u", pack_buf, item.size, &psp);

  wiredtiger_pack_item(psp, &item);
  wiredtiger_pack_close(psp, size);

  return (char *)pack_buf;
}

int main()
{
#ifndef DEBUG
  std::cout << "DEBUG not defined\n";
  exit(1);
#endif
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

  /**
   * Test storing a vector of ints in a WT_ITEM without first packing it.
   */
  WT_CURSOR *cursor;
  session->create(
      session, "table:world", "key_format=i,value_format=u,columns=(src,dst)");
  session->open_cursor(session, "table:world", NULL, NULL, &cursor);

  for (int i = 0; i < 5000; i++)
  {
    cursor->set_key(cursor, i);
    WT_ITEM item;
    std::vector<int> vec = {i + 1, i + 2, i + 3, i + 4, i + 5};
    item.data = vec.data();
    item.size = vec.size() * sizeof(int);
    cursor->set_value(cursor, &item);
    cursor->insert(cursor);
  }
  WT_CURSOR *cursor1;
  session->create(
      session, "table:world1", "key_format=i,value_format=u,columns=(src,dst)");
  session->open_cursor(session, "table:world1", NULL, NULL, &cursor1);
  std::vector<int> tempo;
  for (int i = 0; i < 5000; i++)
  {
    cursor1->set_key(cursor1, i);
    WT_ITEM item;
    std::vector<int> vec = {i + 1, i + 2, i + 3, i + 4, i + 5};
    char *pack_buf = pack_int_vector_wti(session, vec, &size);
    item.data = pack_buf;
    item.size = size;
    cursor1->set_value(cursor1, &item);
    cursor1->insert(cursor1);
    delete pack_buf;
  }
  cursor1->close(cursor1);
  cursor->reset(cursor);
  std::vector<int> temp;
  int i = 0;
  while ((cursor->next(cursor)) == 0)
  {
    int key;
    WT_ITEM item;
    cursor->get_key(cursor, &key);
    cursor->get_value(cursor, &item);
    temp.assign((int *)item.data, (int *)item.data + item.size / sizeof(int));
    int j = 1;
    for (auto v : temp)
    {
      assert(v == i + j);
      j++;
    }
    i++;
  }
  std::cout << "Test to store and retrieve a vector in WT_ITEM passed\n";
  cursor->close(cursor);
  session->close(session, NULL);
  conn->close(conn, NULL);
}
