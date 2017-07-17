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

#include <stdio.h>

// user includes
#include "perftimers.h"

/*******************************************************************************
 * PRIVATE IMPLEMENTATION
 ******************************************************************************/

#define INT_NSEC_PER_SEC 1000000000 // nanosec per sec: for timespec arithmetic
#define DOUBLE_NSEC_PER_SEC 1E9     // nanosec per sec: for timespec arithmetic
#define NO_TIMERS 0                 // if no perf timers are enabled/started

static int timers_status = NO_TIMERS;   // bit flags track which timers enabled/disabled
static int timers_running = NO_TIMERS;  // bit flags track which timers started/stopped
static struct timespec fileops_start;   // save timer start times
static struct timespec fileops_total;   // save cumulative timer start-to-stop times

// return true if specific perf timer bit flag is "enabled"
static inline bool is_enabled (const PerfTimer pt) {
  return ( (bool) (timers_status & pt) );
}

// set specific perf timer "enabled" bit flag
static inline void set_enabled (const PerfTimer pt) {
  timers_status |= pt;
}

// unset specific perf timer "enabled" bit flag
static inline void set_disabled (const PerfTimer pt) {
  timers_status &= (~pt);
}

// return true if specific perf timer bit flag is "started"
static inline bool is_started (const PerfTimer pt) {
  return ( (bool) (timers_running & pt) );
}

// set specific perf timer "started" bit flag
static inline void set_started (const PerfTimer pt) {
  timers_running |= pt;
}

// unset specific perf timer "started" bit flag
static inline void set_stopped (const PerfTimer pt) {
  timers_running &= (~pt);
}

// subtract two timespecs: tstotal = tsa - tsb
static inline void sub_ts (struct timespec* const tstotal, const struct timespec* const tsa, const struct timespec* const tsb) {
  tstotal->tv_sec = tsa->tv_sec - tsb->tv_sec;
  tstotal->tv_nsec = tsa->tv_nsec - tsb->tv_nsec;
  if (tstotal->tv_nsec < 0) {
    tstotal->tv_sec--;
    tstotal->tv_nsec += INT_NSEC_PER_SEC;
  }
}

// add two timespecs: tstotal = tsa + tsb
static inline void add_ts (struct timespec* const tstotal, const struct timespec* const tsa, const struct timespec* const tsb) {
  tstotal->tv_sec = tsa->tv_sec + tsb->tv_sec;
  tstotal->tv_nsec = tsa->tv_nsec + tsb->tv_nsec;
  if (tstotal->tv_nsec > INT_NSEC_PER_SEC) {
    tstotal->tv_sec++;
    tstotal->tv_nsec -= INT_NSEC_PER_SEC;
  }
}

// enable or disable specific perf timer and return success/error of the action
static inline TimerAction set_timer (const PerfTimer pt, const TimerStatus stat_req) {
  const bool pt_enabled = is_enabled(pt);
  TimerAction act = ERR_UNKNOWN_ERROR;

  // trying to enable timer that's already enabled: return err
  if ( (stat_req == ENABLED) && pt_enabled ) {
    act = ERR_TIMER_ALREADY_ENABLED;
  // trying to enable timer that's currently disabled: enable it
  } else if ( (stat_req == ENABLED) && (!pt_enabled) ) {
    fileops_total.tv_sec = 0;
    fileops_total.tv_nsec = 0;
    set_enabled(pt);
    act = SUCCESS_TIMER_ENABLED;
    printf("timer enabled.\n");
  // trying to disable timer that's currently enabled: disable it
  } else if ( ((stat_req == DISABLED)) && pt_enabled ) {
    set_disabled(pt);
    act = SUCCESS_TIMER_DISBLED;
  // trying to disable timer that's already disabled: return err
  } else if ( ((stat_req == DISABLED)) && (!pt_enabled) ) {
    act = ERR_TIMER_ALREADY_DISABLED;
  }

  return act;
}

// start specific perf timer and return success/error of the action
static inline TimerAction start_timer (const PerfTimer pt) {
  TimerAction act = ERR_UNKNOWN_ERROR;

  // trying to start timer that's not yet enabled: return err
  if (!is_enabled(pt)) {
    act = ERR_TIMER_NOT_ENABLED;
    printf("start_timer: ERR_TIMER_NOT_ENABLED.\n");
  // trying to start timer that's already started: return err
  } else if (is_started(pt)) {
    act = ERR_TIMER_ALREADY_STARTED;
  // timer is enabled but not currently running: start it
  } else {
    clock_gettime(CLOCK_MONOTONIC, &fileops_start);
    set_started(pt);
    act = SUCCESS_TIMER_STARTED;
    printf("timer started.\n");
  }

  return act;
}

// stop specific perf timer and return success/error of the action
static inline TimerAction stop_timer (const PerfTimer pt) {
  TimerAction act = ERR_UNKNOWN_ERROR;
  struct timespec fileops_stop;
  struct timespec fileops_subtotal;

  // trying to stop timer that's not yet enabled: return err
  if (!is_enabled(pt)) {
    act = ERR_TIMER_NOT_ENABLED;
  // trying to stop timer that's not yet started: return err
  } else if (!is_started(pt)) {
    act = ERR_TIMER_ALREADY_STOPPED;
    printf("start_timer: ERR_TIMER_ALREADY_STOPPED.\n");
  // timer is enabled and running: stop it and accumulate the run time
  } else {
    clock_gettime(CLOCK_MONOTONIC, &fileops_stop);
    sub_ts(&fileops_subtotal, &fileops_stop, &fileops_start);
    add_ts(&fileops_total, &fileops_total, &fileops_subtotal);
    set_stopped(pt);
    act = SUCCESS_TIMER_STOPPED;
    printf("timer stopped.\n");
  }

  return act;
}

// get total accum time of specific perf timer and return success/error of the action
static inline TimerAction get_total_time (const PerfTimer pt, double* total_time) {
  TimerAction act = ERR_UNKNOWN_ERROR;

  // trying to get total time but timer not yet enabled: return err
  if (!is_enabled(pt)) {
    act = ERR_TIMER_NOT_ENABLED;
  // trying to get total time but timer is running: return err
  } else if (is_started(pt)) {
    act = ERR_TIMER_RUNNING;
  // timer is enabled and stopped: return the total accumulated run time
  } else {
    *total_time = (double)fileops_total.tv_sec + ( (double)fileops_total.tv_nsec / DOUBLE_NSEC_PER_SEC );
    act = SUCCESS_TIMER_TOTAL_RETURNED;
    printf("timer total returned.\n");
  }

  return act;
}

/*******************************************************************************
 * PUBLIC INTERFACE
 ******************************************************************************/

// enable or disable specific perf timer and return success/error of the action
// NOTE successful enable will zero out a timer's accumulated time
inline TimerAction set_perf_timer (const PerfTimer pt, const TimerStatus stat_req) {
  return set_timer(pt, stat_req);
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

