/*******************************************************************************
module:   perftimers
author:   digimokan
date:     14 JUL 2017 (created)
purpose:  start, stop, and output specific pre-configured performance timers
timers:   AUDIT_FILE_COPYING (track total time spent copying files during audit)
*******************************************************************************/

#ifndef PERFTIMERS_H
#define PERFTIMERS_H

// the current set of pre-defined perf timers
typedef enum {
  AUDIT_FILE_COPYING =  0x01,
} PerfTimer;

// for enabling, disabling, or getting status of specific perf timer
typedef enum {
  DISABLED = 0,
  ENABLED =  1
} TimerStatus;

// for returning success of the enable/stop/start/get-total timer action
typedef enum {
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

// enable or disable specific perf timer and return success/error of the action
TimerAction set_perf_timer (const PerfTimer pt, const TimerStatus enable);

// start specific perf timer and return success/error of the action
TimerAction start_perf_timer (const PerfTimer pt);

// stop specific perf timer and return success/error of the action
TimerAction stop_perf_timer (const PerfTimer pt);

// get total accum time of specific perf timer and return success/error of the action
TimerAction get_total_perf_time (const PerfTimer pt, double* total_time);

#endif // PERFTIMERS_H

