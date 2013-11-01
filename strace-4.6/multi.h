#ifndef MULTI_H_INCLUDED
#define MULTI_H_INCLUDED

// create mirror file in the corresponding repo
void create_mirror_file_in_cde_package(char* filename_abspath, char* src_prefix, char* dst_prefix);


// create mirror directories in the corresponding repo

// original_abspath must be an absolute path
// create all the corresponding 'mirror' directories within
// cde-package/cde-root/, MAKING SURE TO CREATE DIRECTORY SYMLINKS
// when necessary (sort of emulate "mkdir -p" functionality)
// if pop_one is non-zero, then pop last element before doing "mkdir -p"
void make_mirror_dirs_in_cde_package(char* original_abspath, int pop_one);
//  create_mirror_dirs(original_abspath, (char*)"", CDE_ROOT_DIR, pop_one);

#endif // MULTI_H_INCLUDED
