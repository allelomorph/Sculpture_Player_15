#include "measure2.h"
#include <thread>
#include <cmath>

using namespace std::chrono;

Prog_Durations::Prog_Durations()
: update_length(30),
  warmup_loops(10),
  loop_count(0),
  accum_count(0),
  time_delay_ms(0.0),
  time_delay_avg_ms(0.0),
  time_delay_min_ms(0.0),
  time_delay_max_ms(0.0),
  time_delay_accum_ms(0.0),
  Trigger(false),
  t_prev()
{}

Prog_Durations::Prog_Durations(int update_len, int warmup)
: update_length(update_len > 0 ? update_len : 30),
  warmup_loops(warmup >= 0 ? warmup : 10),
  loop_count(0),
  accum_count(0),
  time_delay_ms(0.0),
  time_delay_avg_ms(0.0),
  time_delay_min_ms(0.0),
  time_delay_max_ms(0.0),
  time_delay_accum_ms(0.0),
  Trigger(false),
  t_prev()
{}

void Prog_Durations::Start_Delay_Timer()
{
    t_prev = Clock::now();
}

double Prog_Durations::End_Delay_Timer()
{
    const auto now = Clock::now();
    const auto dt_us = duration_cast<microseconds>(now - t_prev).count();
    t_prev = now;

    time_delay_ms = static_cast<double>(dt_us) / 1000.0;
    ++loop_count;

    // Warm-up: ignore first N loops
    if (loop_count <= warmup_loops) {
        accum_count         = 0;
        time_delay_accum_ms = 0.0;
        time_delay_avg_ms   = 0.0;
        time_delay_min_ms   = 0.0;
        time_delay_max_ms   = 0.0;
        Trigger             = false;
        return 0.0;
    }

    // Start / continue this window
    Trigger = false;

    if (accum_count == 0) {
        // First sample in window initializes min/max
        time_delay_min_ms = time_delay_ms;
        time_delay_max_ms = time_delay_ms;
        time_delay_accum_ms = 0.0;
    } else {
        if (time_delay_ms < time_delay_min_ms) time_delay_min_ms = time_delay_ms;
        if (time_delay_ms > time_delay_max_ms) time_delay_max_ms = time_delay_ms;
    }

    time_delay_accum_ms += time_delay_ms;
    ++accum_count;

    if (accum_count >= update_length) {
        // Finalize this window
        time_delay_avg_ms = time_delay_accum_ms / static_cast<double>(update_length);
        accum_count = 0;
        time_delay_accum_ms = 0.0;
        Trigger = true; // caller can check this to print/report
    }

    return time_delay_ms;
}
