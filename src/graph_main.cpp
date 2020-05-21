#include <iostream>
#include <string>
#include "wiredtiger.h"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>

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

    // serialize obj into an std::string
std::string serial_str;
boost::iostreams::back_insert_device<std::string> inserter(serial_str);
boost::iostreams::stream<boost::iostreams::back_insert_device<std::string> > s(inserter);
boost::archive::binary_oarchive oa(s);

vector<string> obj = {"hello","this", "sucks"};
// string obj = "hello";
oa << obj;

// don't forget to flush the stream to finish writing into the buffer
s.flush();

// now you get to const char* with serial_str.data() or serial_str.c_str()
// wrap buffer inside a stream and deserialize serial_str into obj
vector<string> deserialized;
boost::iostreams::basic_array_source<char> device(serial_str.data(), serial_str.size());
boost::iostreams::stream<boost::iostreams::basic_array_source<char> > s1(device);
boost::archive::binary_iarchive ia(s1);
ia >> deserialized;
    
// cout << "sent: " << obj << "received: " << deserialized; 

// ...
for (std::vector<string>::const_iterator i = deserialized.begin(); i != deserialized.end(); ++i)
    std::cout << *i << ' ';

}