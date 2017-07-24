/*******************************************************************************
module:   syslimits
author:   digimokan
date:     21 JUL 2017 (created)
purpose:  obtain various limits of the current operating system
ref:      Advanced Programming In The UNIX Environment, 3rd Ed, Section 2.6
*******************************************************************************/

/*******************************************************************************
 * SYSTEM INCLUDES
 ******************************************************************************/

#include <limits.h>   // OPEN_MAX (possibly)
#include <unistd.h>   // sysconf(), _SC_OPEN_MAX
#include <errno.h>    // errno()
#include <stdio.h>    // perror()

/*******************************************************************************
 * max_open_files()
 * return the max num of open files - at any given time - on this system
 *******************************************************************************/

// if OPEN_MAX is defined, we're done.  else obtain the max programatically.
#ifdef OPEN_MAX
static long max_open = OPEN_MAX;
#else
static long max_open = 0;
#endif

// if unable to get the max by any means, use this number, which is *probably*
// greater than the undiscoverable actual max on the system (according to APUE)
static const long max_open_fallback = 256;

long max_open_files (void) {

  // OPEN_MAX not defined: obtain it once, programatically.
  if (max_open == 0) {

    // reset errno
    errno = 0;

    if ((max_open = sysconf(_SC_OPEN_MAX)) < 0) {
      // sysconf did not set errno value: limit is indeterminate
      if (errno == 0) {
        max_open = max_open_fallback;
      // sysconf set an errno value: problem executing sysconf
      } else {
        perror("sysconf call in max_open_files()");
      }
    }

  }

  return max_open;
}

