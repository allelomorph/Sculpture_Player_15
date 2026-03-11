#pragma once
#include <chrono>
#include <thread>
#include <math.h>


using namespace std::chrono;
using Clock = std::chrono::steady_clock;


inline void regulate_main_loop_timing(Clock::time_point &loop_start, int target_us)
{
    auto loop_end = Clock::now();
    auto elapsed_us = duration_cast<microseconds>(loop_end - loop_start).count();

    if (elapsed_us < target_us - 200)
        std::this_thread::sleep_for(microseconds(target_us - elapsed_us - 200));

    while (elapsed_us < target_us)
    {
        loop_end = Clock::now();
        elapsed_us = duration_cast<microseconds>(loop_end - loop_start).count();
    }

    if (elapsed_us > target_us + 150)
        std::cout << "  elapsed_us LONG !!!!!!!!!!!! " << elapsed_us << std::endl;

    loop_start = Clock::now();
}


// Sleep most of the interval, spin the tail for better accuracy.
// NOTE: still not RT; for sub-millisecond accuracy consider OS timers or RT tuning.
inline void Delay_Msec(double msec)
{
    if (msec <= 0.0) return;

    const auto start = Clock::now();
    const auto target = start + duration_cast<Clock::duration>(duration<double, std::milli>(msec));

    // Sleep for most of it
    // Leave ~0.3 ms margin for spin (tweakable).
    constexpr double spin_margin_ms = 0.3;
    const double sleep_ms = std::max(0.0, msec - spin_margin_ms);

    if (sleep_ms > 0.0) {
        std::this_thread::sleep_for(duration<double, std::milli>(sleep_ms));
    }

    // Spin until target
    while (Clock::now() < target) {
        // optional pause/yield on some platforms
        // std::this_thread::yield();
    }
}
