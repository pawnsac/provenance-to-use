/*******************************************************************************
module:   versioning
author:   digimokan
date:     28 SEP 2017 (created)
purpose:  provenance versioning of spawned processes and accessed files
*******************************************************************************/

#ifndef VERSIONING_H
#define VERSIONING_H 1

// allow this header to be included from c++ source file
#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * PUBLIC TYPES / CONSTANTS / VARIABLES
 ******************************************************************************/

typedef enum {
  SUCCESS_VERSIONING_INITIALIZED,
  SUCCESS_VERSIONING_CLEARED,
  SUCCESS_FILE_VERSION_OPENED,
  SUCCESS_FILE_VERSION_CLOSED,
  SUCCESS_PROCESS_SPAWNED,
  FILE_OR_PROCESS_MODIFIED,
  FILE_OR_PROCESS_NOT_MODIFIED,
  FILE_OR_PROCESS_NOT_EXIST,
  ERR_VERSIONING_ALREADY_INITIALIZED,
  ERR_VERSIONING_NOT_INITIALIZED,
  ERR_PROCESS_ALREADY_EXIST,
  ERR_PARENT_PROCESS_NOT_EXIST,
  ERR_CHILD_PROCESS_ALREADY_EXIST,
  ERR_UNKNOWN_VERSION_ERR
} VersionAction;

typedef enum {
  READ_ONLY,
  WRITE_ONLY,
  READ_WRITE,
  UNKNOWN_OTYPE
} OpenType;

/*******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/

// initialize the data structures required for versioning operations
VersionAction init_versioning ();

// clear/delete the data structures required for versioning operations
VersionAction clear_versioning ();

// retrieve the versioning graph
struct versioned_prov_graph* get_versioning_graph ();

// provenance-version a process-opens-file operation
VersionAction versioned_open (char* executable_abspath, char* filename_abspath, OpenType otype);

// provenance-version a process-closes-file operation
// WARNING: assuming process only does one READ-open / WRITE-open / RW-open per file
VersionAction versioned_close (char* executable_abspath, char* filename_abspath, OpenType otype);

// provenance-version a process-spawns-another-process operation
VersionAction versioned_spawn (char* parent_executable_abspath, char* child_executable_abspath);

// provenance-version a process-opens-file operation and log to stdout
VersionAction log_versioned_open (char* executable_abspath, char* filename_abspath, OpenType otype);

// provenance-version a process-closes-file operation and log to stdout
VersionAction log_versioned_close (char* executable_abspath, char* filename_abspath, OpenType otype);

// provenance-version a process-closes-file operation and log to stdout
VersionAction log_versioned_spawn (char* parent_executable_abspath, char* child_executable_abspath);

// print each versioning graph edge to stdout
void log_versioned_edges ();

// query versioning graph to see if file or process modified since last prog run
VersionAction is_file_or_process_modified (char* filename_or_executable_abspath);

// allow this header to be included from c++ source file
#ifdef __cplusplus
}
#endif

#endif // VERSIONING_H

