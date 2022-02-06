#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <iterator>
#include <wiredtiger.h>
#include <stdlib.h>
#include <chrono>
#include "common.h"
using namespace std;
static const char *db_name = "test";

void increment(int &a)
{
  cout << endl
       << a;
  a = a + 1;
  cout << endl
       << a;
  return;
}

std::vector<int> unpack_int_vector(char *to_unpack,
                                   WT_SESSION *session)
{
  std::vector<int> unpacked_vector;
  WT_PACK_STREAM *psp;
  const char *fmt = "III";
  size_t size = 1000000; // for lack of something clever
  int ret = wiredtiger_unpack_start(session, fmt, to_unpack, size, &psp);
  const char *format_str;
  //ret = wiredtiger_unpack_str(psp, &format_str);
  //cout << "init format is " << format_str << endl;
  //wiredtiger_pack_close(psp, &size); // To reset the unpacking stream.

  int64_t res;
  for (int i = 0; i < 3; i++)
  {
    ret = wiredtiger_unpack_int(psp, &res);
    cout << res << endl;
    unpacked_vector.push_back(res);
  }

  wiredtiger_pack_close(psp, &size);
  return unpacked_vector;
}

int main()
{
  // Create an empty vector
  vector<int> vect;

  random_device rnd_device;
  // Specify the engine and distribution.
  mt19937 mersenne_engine{rnd_device()}; // Generates random integers
  uniform_int_distribution<long> dist{214748, 3147483};

  auto gen = [&dist, &mersenne_engine]()
  {
    return dist(mersenne_engine);
  };

  vector<int> vec(1000);
  generate(begin(vec), end(vec), gen);
  size_t size;
  auto start = std::chrono::steady_clock::now();
  std::string packed = CommonUtil::pack_int_vector_std(vec, &size);
  auto end = std::chrono::steady_clock::now();
  std::cout << "packed in : "
            << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
            << "\n";

  start = std::chrono::steady_clock::now();
  std::vector<int> unpacked = CommonUtil::unpack_int_vector_std(packed);
  end = std::chrono::steady_clock::now();
  std::cout << "unpacked in : "
            << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
            << "\n";
}
