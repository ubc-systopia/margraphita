#include <filesystem>
#include <iostream>
#include <string>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <wiredtiger.h>
#include <stdlib.h>
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

  bool val = true;
  cout << "val is" << int(val) << endl;
  val = false;
  cout << "val is " << int(val) << endl;
  vect.push_back(10);
  vect.push_back(20);
  vect.push_back(30); //keeping the packing buffer size the same, this fails if the vector values are changed to 100, 200, 300

  for (int x : vect)
    cout << x << endl;

  std::filesystem::path dirname = "./db/" + string(db_name);
  if (std::filesystem::exists(dirname))
  {
    filesystem::remove_all(dirname); // remove if exists;
  }
  std::filesystem::create_directory(dirname);

  WT_CONNECTION *conn;
  WT_SESSION *session;
  /* Open a connection to the database, creating it if necessary. */
  wiredtiger_open(dirname.c_str(), NULL, "create", &conn);
  /* Open a session for the current thread's work. */
  conn->open_session(conn, NULL, NULL, &session);
  /* Do some work... */
  /* Note: closing the connection implicitly closes open session(s). */
  char *buffer = (char *)malloc(1000000);
  size_t size = 1000000;

  WT_PACK_STREAM *psp;
  const char *fmt = "III";
  int ret = wiredtiger_pack_start(session, fmt, buffer, size, &psp);
  //wiredtiger_pack_str(psp, fmt);
  for (int x : vect)
  {
    ret = wiredtiger_pack_int(psp, x);
  }
  size_t used;
  cout << "used : " << used << endl;
  wiredtiger_pack_close(psp, &used);

  vector<int> unpack = unpack_int_vector(buffer, session);

  conn->close(conn, NULL);
  return 0;
}
