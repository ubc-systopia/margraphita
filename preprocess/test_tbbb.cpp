#include <tbb/concurrent_hash_map.h>
#include <cstdint>
#include <iostream>

struct degree {
  uint32_t a;
  uint32_t b;
};

using MapType = tbb::concurrent_hash_map<int32_t, degree>;

int main() {
  MapType myMap;

  int32_t key = 42;
  degree initialValue = {10, 20}; // a = 10, b = 20

  // Insert the initial value
  {
    MapType::accessor acc;
    myMap.insert(acc, key);
    acc->second = initialValue;
  } // Accessor automatically releases lock

  // Update only `b` while keeping `a` unchanged
  {
    MapType::accessor acc;
    if (myMap.find(acc, key)) {
      acc->second.b = 99; // Update `b`
    }
  } // Accessor releases lock

  // Verify update
  {
    MapType::const_accessor acc;
    if (myMap.find(acc, key)) {
      std::cout << "Key: " << key << ", a: " << acc->second.a << ", b: " << acc->second.b << std::endl;
    }
  }

  return 0;
}
