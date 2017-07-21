#ifndef _CDE_H
#define _CDE_H

extern char Cde_verbose_mode;    // print cde activity to stdout (-v option)
extern char Cde_exec_mode;       // false if auditing, true if running captured app
extern char Cde_app_dir[];       // abs path to cde app dir (contains cde-root)
extern char Cde_follow_ssh_mode;

// like an assert except that it always fires
#define EXITIF(x) do { \
  if (x) { \
    fprintf(stderr, "Fatal error in %s [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__); \
    exit(1); \
  } \
} while(0)

#endif // _CDE_H

