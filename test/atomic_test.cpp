#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

std::atomic<int> foo(0);
int bar = 0;
std::mutex mtx;

void set_foo()
{
    foo++;
    // foo.fetch_add(1, std::memory_order_relaxed);
}

void set_foo_mtx()
{
    mtx.lock();
    bar++;
    mtx.unlock();
}

int main()
{
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; ++i)
    {
        std::thread first(set_foo);
        std::thread second(set_foo);
        first.join();
        second.join();
    }
    std::cout << foo << std::endl;
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "time with atomic: "
              << std::chrono::duration_cast<
                     std::chrono::duration<double, std::micro>>(end - start)
                     .count()
              << std::endl;

    // Now with a mutex
    auto start_m = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < 100000; i++)
    {
        std::thread first(set_foo_mtx);
        std::thread second(set_foo_mtx);
        first.join();
        second.join();
    }
    std::cout << foo << std::endl;
    auto end_m = std::chrono::high_resolution_clock::now();
    std::cout << "time with mutex: "
              << std::chrono::duration_cast<
                     std::chrono::duration<double, std::micro>>(end_m - start_m)
                     .count();

    return 0;
}
