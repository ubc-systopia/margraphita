#ifndef TIMES_H
#define TIMES_H

#include <chrono>

class Times
{
   public:
    Times() {}

    void start() { start_time = std::chrono::high_resolution_clock::now(); }

    void stop() { end_time = std::chrono::high_resolution_clock::now(); }

    double t_secs() const
    {
        return std::chrono::duration_cast<std::chrono::duration<double>>(
                   end_time - start_time)
            .count();
    }

    double t_millis() const
    {
        return std::chrono::duration_cast<
                   std::chrono::duration<double, std::milli>>(end_time -
                                                              start_time)
            .count();
    }

    double t_micros() const
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
