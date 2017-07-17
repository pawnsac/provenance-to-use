/*******************************************************************************
module:   perftimers
author:   digimokan
date:     14 JUL 2017 (created)
purpose:  start, stop, and output specific pre-configured performance timers
timers:   AUDIT_FILE_COPYING (track total time spent copying files during audit)
*******************************************************************************/

// system includes
#include <stdbool.h>        // for bool data type
#include <time.h>           // for struct timespec, clock_gettime

// user includes
#include "perftimers.h"

/*******************************************************************************
 * PRIVATE IMPLEMENTATION
 ******************************************************************************/

#define NO_TIMERS    0x00    // no perf timers are enabled
#define NUM_TIMERS   1       // current number of perf timers defined in header
#define NSEC_PER_SEC 1000000000 // nanosec per sec: for timespec arithmetic

static int timers_enabled = NO_TIMERS;  // timer flags track which are enabled
static int timers_started = NO_TIMERS;  // timer flags track which are running
static struct timespec fileops_start;   // save timer start times
static struct timespec fileops_total;   // save cumulative timer start-to-stop times

// return true if specific perf timer flag is set
static inline bool is_set (int timer_flags, const PerfTimer pt) {
  return ( (bool) (timer_flags & pt) );
}

// set specific perf timer flag
static inline void set_flag (int timer_flags, const PerfTimer pt) {
  timer_flags = timer_flags | pt;
}

// unset specific perf timer flag
static inline void unset_flag (int timer_flags, const PerfTimer pt) {
  timer_flags = timer_flags & (~pt);
}

// subtract two timespecs: tstotal = tsa - tsb
static inline void sub_ts (struct timespec* const tstotal, const struct timespec* const tsa, const struct timespec* const tsb) {
  tstotal->tv_sec = tsa->tv_sec - tsb->tv_sec;
  tstotal->tv_nsec = tsa->tv_nsec - tsb->tv_nsec;
  if (tstotal->tv_nsec < 0) {
    tstotal->tv_sec--;
    tstotal->tv_nsec += NSEC_PER_SEC;
  }
}

// add two timespecs: tstotal = tsa + tsb
static inline void add_ts (struct timespec* const tstotal, const struct timespec* const tsa, const struct timespec* const tsb) {
  tstotal->tv_sec = tsa->tv_sec + tsb->tv_sec;
  tstotal->tv_nsec = tsa->tv_nsec + tsb->tv_nsec;
  if (tstotal->tv_nsec > NSEC_PER_SEC) {
    tstotal->tv_sec++;
    tstotal->tv_nsec -= NSEC_PER_SEC;
  }
}

// enable or disable specific perf timer and return success/error of the action
static inline TimerAction set_timer (const PerfTimer pt, const TimerStatus enable) {
  const bool pt_enabled = is_set(timers_enabled, pt);
  TimerAction act = ERR_UNKNOWN_ERROR;

  // trying to enable timer that's already enabled: return err
  if ( enable && pt_enabled ) {
    act = ERR_TIMER_ALREADY_ENABLED;
  // trying to enable timer that's currently disabled: enable it
  } else if ( enable && (!pt_enabled) ) {
    fileops_total.tv_sec = 0;
    fileops_total.tv_nsec = 0;
    set_flag(timers_enabled, pt);
    act = SUCCESS_TIMER_ENABLED;
  // trying to disable timer that's currently enabled: disable it
  } else if ( (!enable) && pt_enabled ) {
    unset_flag(timers_enabled, pt);
    act = SUCCESS_TIMER_DISBLED;
  // trying to disable timer that's already disabled: return err
  } else if ( (!enable) && (!pt_enabled) ) {
    act = ERR_TIMER_ALREADY_DISABLED;
  }

  return act;
}

// start specific perf timer and return success/error of the action
static inline TimerAction start_timer (const PerfTimer pt) {
  TimerAction act = ERR_UNKNOWN_ERROR;

  // trying to start timer that's not yet enabled: return err
  if (!is_set(timers_enabled, pt)) {
    act = ERR_TIMER_NOT_ENABLED;
  // trying to start timer that's already started: return err
  } else if (is_set(timers_started, pt)) {
    act = ERR_TIMER_ALREADY_STARTED;
  // timer is enabled but not currently running: start it
  } else {
    clock_gettime(CLOCK_MONOTONIC, &fileops_start);
    set_flag(timers_started, pt);
    act = SUCCESS_TIMER_STARTED;
  }

  return act;
}

// stop specific perf timer and return success/error of the action
static inline TimerAction stop_timer (const PerfTimer pt) {
  TimerAction act = ERR_UNKNOWN_ERROR;
  struct timespec fileops_stop;
  struct timespec fileops_subtotal;

  // trying to stop timer that's not yet enabled: return err
  if (!is_set(timers_enabled, pt)) {
    act = ERR_TIMER_NOT_ENABLED;
  // trying to stop timer that's not yet started: return err
  } else if (!is_set(timers_started, pt)) {
    act = ERR_TIMER_ALREADY_STOPPED;
  // timer is enabled and running: stop it and accumulate the run time
  } else {
    clock_gettime(CLOCK_MONOTONIC, &fileops_stop);
    sub_ts(&fileops_subtotal, &fileops_stop, &fileops_start);
    add_ts(&fileops_total, &fileops_total, &fileops_subtotal);
    unset_flag(timers_started, pt);
    act = SUCCESS_TIMER_STOPPED;
  }

  return act;
}

// get total accum time of specific perf timer and return success/error of the action
static inline TimerAction get_total_time (const PerfTimer pt, double* total_time) {
  return SUCCESS_TIMER_TOTAL_RETURNED;
}

/*******************************************************************************
 * PUBLIC INTERFACE
 ******************************************************************************/

// enable or disable specific perf timer and return success/error of the action
// NOTE successful enable will zero out a timer's accumulated time
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
  /*clock_gettime(CLOCK_REALTIME, &fileops_stop);*/
  /*audit_file_ops += ( ( ((float)fileops_stop.tv_sec) - ((float)fileops_start.tv_sec) ) +*/
                      /*( (((float)fileops_stop.tv_nsec) - ((float)fileops_start.tv_nsec)) / 1E9F) );*/
}

// get total accum time of specific perf timer and return success/error of the action
inline TimerAction get_total_perf_time (const PerfTimer pt, double* total_time) {
  return get_total_time(pt, total_time);
}

