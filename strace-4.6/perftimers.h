/*******************************************************************************
module:   perftimers
author:   digimokan
date:     14 JUL 2017 (created)
purpose:  start, stop, and output specific pre-configured performance timers
*******************************************************************************/

#ifndef PERFTIMERS_H
#define PERFTIMERS_H

#define ENABLED 0
#define DISABLED 1

typedef enum perf_timers {
  NO_TIMERS =           0,
  AUDIT_FILE_COPYING =  0x01,
  NUM_PERF_TIMERS =     3
} PerfTimers;

typedef enum timer_action {
  SUCCESS_TIMER_ENABLED,
  SUCCESS_TIMER_DISBLED,
  SUCCESS_TIMER_STARTED,
  SUCCESS_TIMER_STOPPED,
  SUCCESS_TIMER_TOTAL_RETURNED,
  ERR_TIMER_NOT_ENABLED,
  ERR_TIMER_ALREADY_ENABLED,
  ERR_TIMER_ALREADY_STARTED,
  ERR_TIMER_ALREADY_STOPPED,
  ERR_TIMER_ALREADY_DISABLED
} TimerAction;

extern int timers_enabled;
extern int timers_running;

TimerAction set_perf_timer (const PerfTimers pt, const int enabled_or_disabled);
TimerAction start_perf_timer (const PerfTimers pt);
TimerAction stop_perf_timer (const PerfTimers pt);
TimerAction get_total_perf_time (const PerfTimers pt, double* total_time);

#endif /* PERFTIMERS_H */

