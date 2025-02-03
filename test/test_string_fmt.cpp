#include <fmt/core.h>
#include <fmt/format.h>

#include <chrono>
#include <iostream>
#include <string>

// generate random strings
std::string random_string(size_t length)
{
  auto randchar = []() -> char
  {
    const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    const size_t max_index = (sizeof(charset) - 1);
    return charset[rand() % max_index];
  };
  std::string str(length, 0);
  std::generate_n(str.begin(), length, randchar);
  return str;
}

int main()
{
  auto times_sum = 0.0;
  for (int i = 0; i < 1000000; i++)
  {
    std::string a = random_string(10);
    std::string b = random_string(10);
    std::string c = random_string(10);

    auto start = std::chrono::high_resolution_clock::now();
    std::string final = fmt::format("a: {}, b: {}, c: {}", a, b, c);
    // std::cout << final << std::endl;
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    // time in microseconds
    auto x =
        std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(
            end - start)
            .count();
    times_sum += x;
    // std::cout << "time = " << x << std::endl;
  }
  std::cout << "average_time = " << (times_sum / 100000) << std::endl;

  for (int i = 0; i < 1000000; i++)
  {
    std::string a = random_string(10);
    std::string b = random_string(10);
    std::string c = random_string(10);

    auto start = std::chrono::high_resolution_clock::now();
    std::string final = "a: " + a + ", b: " + b + ",c: " + c;
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    // time in microseconds
    auto x =
        std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(
            end - start)
            .count();
    times_sum += x;
    // std::cout << "time = " << x << std::endl;
  }
  std::cout << "average_time = " << (times_sum / 100000) << std::endl;

  return 0;
}
