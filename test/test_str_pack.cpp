#include <filesystem>
#include <iostream>
#include <string>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <wiredtiger.h>
#include<stdlib.h>
using namespace std;
static const char *db_name = "test";
 WT_CONNECTION *conn;
  WT_SESSION *session;
  
void increment (int &a){
  cout<<endl<<a;
  a = a+1;
  cout<<endl<<a;
  return;
}

int get_table_cursor(string table, WT_CURSOR **cursor,
                                     bool is_random)
{
  std::string table_name = "table:" + table;
  const char *config = NULL;
  if (is_random)
  {
    config = "next_random=true";
  }
  if (int ret = session->open_cursor(session, table_name.c_str(), NULL, config,
                                     cursor) != 0)
  {
    fprintf(stderr, "Failed to get table cursor to %s\n", table_name.c_str());
    return ret;
  }
  return 0;
}

std::vector<std::string> unpack_string_vector(char *to_unpack,
                                               WT_SESSION *session) {
  std::vector<std::string> unpacked_vector;
  WT_PACK_STREAM *psp;
  const char *fmt = "S";
  const char *fmt_new = (char*) malloc(50*sizeof(char));
  size_t size = 1000; // for lack of something clever
  int ret = wiredtiger_unpack_start(session, fmt, to_unpack, size, &psp);
  ret = wiredtiger_unpack_str(psp, &fmt_new);
  cout <<"fmt_new"<<fmt_new<<endl;
  wiredtiger_pack_close(psp, &size); // To reset the unpacking stream.

  ret = wiredtiger_unpack_start(session, fmt_new, to_unpack, 1000, &psp);
  size_t fmt_len = strlen(fmt_new);
  cout <<"fmt_len"<<fmt_len<<endl;

  for (int i = 0; i < fmt_len; i++) {
    const char* res = (char*) malloc(50*sizeof(char)); 
    ret = wiredtiger_unpack_str(psp, &res);
    cout <<"in loop "<<res<<endl;

    if(i>0){
      unpacked_vector.push_back(res);
    }
  }
  return unpacked_vector;
}

std::vector<int> unpack_int_vector(char *to_unpack,
                                               WT_SESSION *session) {
  std::vector<int> unpacked_vector;
  WT_PACK_STREAM *psp;
  const char *fmt = "S";
  size_t size = INT64_MAX; // for lack of something clever
  int ret = wiredtiger_unpack_start(session, fmt, to_unpack, size, &psp);
  ret = wiredtiger_unpack_str(psp, &fmt);
  cout<<"got format" <<fmt<<endl;
  wiredtiger_pack_close(psp, &size); // To reset the unpacking stream.

  ret = wiredtiger_unpack_start(session, fmt, to_unpack, size, &psp);
  size_t fmt_len = strlen(fmt);
  for (int i = 0; i < fmt_len; i++) {
    if (i == 0) {
      const char *ret_fmt = (char *)malloc(fmt_len * sizeof(fmt_len));
      ret = wiredtiger_unpack_str(psp, &ret_fmt);
    }
    int64_t res;
    ret = wiredtiger_unpack_int(psp, &res);
    cout<<res;
    unpacked_vector.push_back(res);
  }
  return unpacked_vector;
}

int main() {
  // Create an empty vector
  vector<string> vect;

  vect.push_back("hello");
  vect.push_back("world");
  vect.push_back("testing");
  
  vector<int> vect1;
  vect1.push_back(10);
  vect1.push_back(20);
  vect1.push_back(30);
   //keeping the packing buffer size the same, this fails if the vector values are changed to 100, 200, 300

  for (string x : vect)
    cout << x << endl;

  cout<<"vect1"<<endl;
    for (int x : vect1)
    cout << x << endl;

  string dirname = "./db/" + string(db_name);
  // if (std::filesystem::exists(dirname)) {
  //   filesystem::remove_all(dirname); // remove if exists;
  // }
  // std::filesystem::create_directory(dirname);


 /* Open a connection to the database, creating it if necessary. */
  wiredtiger_open(dirname.c_str(), NULL, "create", &conn);
  /* Open a session for the current thread's work. */
  conn->open_session(conn, NULL, NULL, &session);
  /* Do some work... */
  /* Note: closing the connection implicitly closes open session(s). */
  char buffer[1000];
  
  session->create(session, "table:world",
      "key_format=r,value_format=5sii,"
      "columns=(id,country,population,area)");	
  
WT_CURSOR *cursor = nullptr;
int retval = get_table_cursor("world",&cursor,false);
if(cursor == nullptr){
cout <<retval <<" cursor open failed";
}else{
  cout<<"cursor open successful. retval = " << retval<<endl;
}
  


//   WT_PACK_STREAM *psp;
//   string fmt_string = "SSSS";
//   const char *fmt = fmt_string.c_str();
//   int ret = wiredtiger_pack_start(session, fmt, buffer, 500, &psp);
//   wiredtiger_pack_str(psp, fmt);
//     for (string x : vect)
//     {
//         const char *x_Str = x.c_str();
//         ret = wiredtiger_pack_str(psp, x_Str);
//         cout << "insert returned "<< ret << endl;
//     }
// size_t used;
// wiredtiger_pack_close(psp,&used);

// vector<string> unpacked = unpack_string_vector(buffer, session);
// for (string x : unpacked)
//     cout << x << endl;


  WT_PACK_STREAM *ps;
  string fmt_string = "Siii";
  const char *fmt1 = fmt_string.c_str();
  int ret = wiredtiger_pack_start(session, fmt1, buffer, 1000, &ps);
  wiredtiger_pack_str(ps, fmt1);
    for (int x : vect1)
    {
        
        ret = wiredtiger_pack_int(ps, x);
        cout << "insert returned "<< ret << endl;
    }
size_t used;
wiredtiger_pack_close(ps,&used);
cout<<"ussed is" <<used<<"buffer is" << buffer <<endl;

vector<int> unpacked_int = unpack_int_vector(buffer, session);
for (int x : unpacked_int)
    cout << x << endl;

conn->close(conn, NULL);
  return 0;
}

