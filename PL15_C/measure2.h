#pragma once
#include <chrono>


using Clock = std::chrono::steady_clock;

struct Prog_Durations {
    // configuration
    int update_length;               // number of samples per window
    int warmup_loops;                // loops to ignore at startup

    // live state
    int   loop_count;                // total loops seen
    int   accum_count;               // samples accumulated in current window
    double time_delay_ms;            // last measured dt (ms)
    double time_delay_avg_ms;        // window average (ms)
    double time_delay_min_ms;        // window min (ms)
    double time_delay_max_ms;        // window max (ms)
    double time_delay_accum_ms;      // accumulator (ms)
    bool   Trigger;                  // true when avg/min/max updated this call

    // using clock = std::chrono::steady_clock;
    Clock::time_point t_prev;

    Prog_Durations();
    explicit Prog_Durations(int update_len, int warmup = 10);

    void Start_Delay_Timer();        // mark start
    double End_Delay_Timer();        // mark end, update stats, return last dt (ms)
};



inline long long GetDeltaTime(const Clock::time_point &start_time)
{
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start_time).count();
    return duration;
}


// Precise-ish sleep that avoids 100% CPU burn.
// Sleeps most of the interval, then spins for the final few hundred microseconds.
void Delay_Msec(double msec);
