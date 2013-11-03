#include "multi.h"
#include "okapi.h"

extern char* CDE_ROOT_DIR;

void create_mirror_file_in_cde_package(char* filename_abspath, char* src_prefix, char* dst_prefix) {
  create_mirror_file(filename_abspath, src_prefix, dst_prefix);
}


// original_abspath must be an absolute path
// create all the corresponding 'mirror' directories within
// cde-package/cde-root/, MAKING SURE TO CREATE DIRECTORY SYMLINKS
// when necessary (sort of emulate "mkdir -p" functionality)
// if pop_one is non-zero, then pop last element before doing "mkdir -p"
void make_mirror_dirs_in_cde_package(char* original_abspath, int pop_one) {
  //TODO: check original_abspath with CDE_PACKAGE_DIR
  // so we won't make unnecessary path like: ~/assi/cde/mytest/bash/cde-package/pkg02/home/quanpt/assi/cde/mytest/
  //   bash/cde-package/cde-root/home/quanpt/assi/cde/mytest/bash
  create_mirror_dirs(original_abspath, (char*)"", CDE_ROOT_DIR, pop_one);
}
