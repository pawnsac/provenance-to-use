/*******************************************************************************
module:   perftimers
author:   digimokan
date:     14 JUL 2017 (created)
purpose:  start, stop, and output specific pre-configured performance timers
*******************************************************************************/

#include "perftimers.h"

int timers_enabled = NO_TIMERS;
int timers_running = NO_TIMERS;

TimerAction set_perf_timer (const PerfTimers pt, const int enabled_or_disabled) {
  return SUCCESS_TIMER_ENABLED;
}

TimerAction start_perf_timer (const PerfTimers pt) {
  return SUCCESS_TIMER_STARTED;
}

TimerAction stop_perf_timer (const PerfTimers pt) {
  return SUCCESS_TIMER_STOPPED;
}

TimerAction get_total_perf_time (const PerfTimers pt, double* total_time) {
  return SUCCESS_TIMER_TOTAL_RETURNED;
}

