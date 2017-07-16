/*******************************************************************************
module:   perftimers
author:   digimokan
date:     14 JUL 2017 (created)
purpose:  start, stop, and output specific pre-configured performance timers
timers:   AUDIT_FILE_COPYING (track total time spent copying files during audit)
*******************************************************************************/

// system includes
#include <stdbool.h>        // for bool data type

// user includes
#include "perftimers.h"

/*******************************************************************************
 * PRIVATE IMPLEMENTATION
 ******************************************************************************/

#define NO_TIMERS   0x00    // no perf timers are enabled
#define NUM_TIMERS  1       // current number of perf timers defined in header

static int timers_enabled = NO_TIMERS;  // track which perf timers are enabled
static int timers_running = NO_TIMERS;  // track which perf timers are running

// return true if specific perf timer is enabled
static inline bool timer_is_enabled (const PerfTimer pt) {
  return ( (bool) (timers_enabled & pt) );
}

// enable specific perf timer
static inline void timer_enable (const PerfTimer pt) {
  timers_enabled = timers_enabled | pt;
}

// disable specific perf timer
static inline void timer_disable (const PerfTimer pt) {
  timers_enabled = timers_enabled & (~pt);
}

// enable or disable specific perf timer and return success/error of the action
static inline TimerAction set_timer (const PerfTimer pt, const TimerStatus enable) {
  const bool pt_enabled = timer_is_enabled(pt);

  if ( enable && pt_enabled ) {
    return ERR_TIMER_ALREADY_ENABLED;
  } else if ( enable && (!pt_enabled) ) {
    timer_enable(pt);
    return SUCCESS_TIMER_ENABLED;
  } else if ( (!enable) && pt_enabled ) {
    timer_disable(pt);
    return SUCCESS_TIMER_DISBLED;
  } else if ( (!enable) && (!pt_enabled) ) {
    return ERR_TIMER_ALREADY_DISABLED;
  }
}

// start specific perf timer and return success/error of the action
static inline TimerAction start_timer (const PerfTimer pt) {
  return SUCCESS_TIMER_STARTED;
}

// stop specific perf timer and return success/error of the action
static inline TimerAction stop_timer (const PerfTimer pt) {
  return SUCCESS_TIMER_STOPPED;
}

// get total accum time of specific perf timer and return success/error of the action
static inline TimerAction get_total_time (const PerfTimer pt, double* total_time) {
  return SUCCESS_TIMER_TOTAL_RETURNED;
}

/*******************************************************************************
 * PUBLIC INTERFACE
 ******************************************************************************/

// enable or disable specific perf timer and return success/error of the action
inline TimerAction set_perf_timer (const PerfTimer pt, const TimerStatus enable) {
  return set_timer(pt, enable);
}

// start specific perf timer and return success/error of the action
inline TimerAction start_perf_timer (const PerfTimer pt) {
  return start_timer(pt);
}

// stop specific perf timer and return success/error of the action
inline TimerAction stop_perf_timer (const PerfTimer pt) {
  return stop_timer(pt);
}

// get total accum time of specific perf timer and return success/error of the action
inline TimerAction get_total_perf_time (const PerfTimer pt, double* total_time) {
  return get_total_time(pt, total_time);
}

