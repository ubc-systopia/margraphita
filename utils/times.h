#ifndef TIMES_H
#define TIMES_H

#include <chrono>

class Times
{
   public:
    Times() = default;

    void start() { start_time = std::chrono::high_resolution_clock::now(); }

    void stop() { end_time = std::chrono::high_resolution_clock::now(); }

    [[maybe_unused]] [[nodiscard]] long double t_secs() const
    {
        return std::chrono::duration_cast<std::chrono::duration<double>>(
                   end_time - start_time)
            .count();
    }

    [[maybe_unused]] [[nodiscard]] long double t_millis() const
    {
        return std::chrono::duration_cast<
                   std::chrono::duration<double, std::milli>>(end_time -
                                                              start_time)
            .count();
    }

    [[maybe_unused]] [[nodiscard]] long double t_nanos() const
    {
        return std::chrono::duration_cast<
                   std::chrono::duration<double, std::nano>>(end_time -
                                                             start_time)
            .count();
    }

    [[nodiscard]] long double t_micros() const
    {
        return std::chrono::duration_cast<
                   std::chrono::duration<double, std::micro>>(end_time -
                                                              start_time)
            .count();
    }

   private:
    std::chrono::high_resolution_clock::time_point start_time, end_time;
};

#endif
