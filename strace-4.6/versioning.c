/*******************************************************************************
module:   versioning
author:   digimokan
date:     28 SEP 2017 (created)
purpose:  provenance versioning of spawned processes and accessed files
*******************************************************************************/

/*******************************************************************************
 * SYSTEM INCLUDES
 ******************************************************************************/

#include <stdio.h>              // printf()

/*******************************************************************************
 * USER INCLUDES
 ******************************************************************************/

#include "versioning.h"
#include "versioning_graph.h"   // versioned_prov_graph
#include "uthash.h"             // UT_hash_handle
#include "strutils.h"           // get_str_size_of_int()

/*******************************************************************************
 * PRIVATE TYPES / CONSTANTS / VARIABLES
 ******************************************************************************/

// private variables

static struct versioned_prov_graph* graph;

/*******************************************************************************
 * PRIVATE MACROS / FUNCTIONS
 ******************************************************************************/

// get latest versioned node for file, creating 1st node version if not exist
static struct node_entry* get_or_add_latest_versioned_node_for_file (char* filename_abspath) {
  return retrieve_latest_versioned_node(graph, filename_abspath, FILE_NODE);
}

// get latest versioned node for pid, creating 1st node version if not exist
static struct node_entry* get_or_add_latest_versioned_node_for_pid (int pid) {
  char* pid_str;
  malloc_str_from_int(&pid_str, pid);
  struct node_entry* entry = retrieve_latest_versioned_node(graph, pid_str, PROCESS_NODE);
  free(pid_str);
  return entry;
}

static int convert_proc_char_to_pid (char pc) {
  int pid;
  switch (pc) {
    case 'P': pid = 1; break;
    case 'Q': pid = 2; break;
    case 'R': pid = 3; break;
    case 'S': pid = 4; break;
    case 'T': pid = 5; break;
    case 'U': pid = 6; break;
    case 'V': pid = 7; break;
    case 'W': pid = 8; break;
    case 'X': pid = 9; break;
    default : pid = -1; break;
  }
  return pid;
}

static char convert_pid_char_to_proc_char (char* pid_label) {
  char proc_char;
  switch (pid_label[0]) {
    case '1' : proc_char = 'P'; break;
    case '2' : proc_char = 'Q'; break;
    case '3' : proc_char = 'R'; break;
    case '4' : proc_char = 'S'; break;
    case '5' : proc_char = 'T'; break;
    case '6' : proc_char = 'U'; break;
    case '7' : proc_char = 'V'; break;
    case '8' : proc_char = 'W'; break;
    case '9' : proc_char = 'X'; break;
    default  : proc_char = '?'; break;
  }
  return proc_char;
}

static char get_proc_char_from_node (struct node_entry* node) {
  char proc_char = (node->ntype == FILE_NODE)
  ? node->label[0]
  : convert_pid_char_to_proc_char(node->label);
  return proc_char;
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/

// initialize the data structures required for versioning operations
VersionAction init_versioning () {

  if (graph == NULL) {
    graph = create_new_graph();
    return (SUCCESS_VERSIONING_INITIALIZED);

  } else {
    return (ERR_VERSIONING_ALREADY_INITIALIZED);
  }

}

// clear/delete the data structures required for versioning operations
VersionAction clear_versioning () {

  if (graph == NULL) {
    return (ERR_VERSIONING_NOT_INITIALIZED);

  } else {
    clear_graph(&graph);
    return (SUCCESS_VERSIONING_CLEARED);
  }

}

// retrieve the versioning graph
struct versioned_prov_graph* get_versioning_graph () {
  return graph;
}

// provenance-version a process-opens-file operation
VersionAction versioned_open (int pid, char* filename_abspath, OpenType otype) {

  if (graph == NULL)
    return (ERR_VERSIONING_NOT_INITIALIZED);

  struct node_entry* file_node = get_or_add_latest_versioned_node_for_file(filename_abspath);
  struct node_entry* pid_node = get_or_add_latest_versioned_node_for_pid(pid);

  if (otype == READ_ONLY) {
    connect(graph, file_node, pid_node);
  } else if (otype == WRITE_ONLY) {
    connect(graph, pid_node, file_node);
  } else if (otype == READ_WRITE) {
    connect(graph, file_node, pid_node);
    connect(graph, pid_node, file_node);
  } else {
    return (ERR_UNKNOWN_VERSION_ERR);
  }

  return (SUCCESS_FILE_VERSION_OPENED);

}

// provenance-version a process-opens-file operation and log to stdout
VersionAction log_versioned_open (char proc, char* filename_abspath, OpenType otype) {

  int pid = convert_proc_char_to_pid(proc);
  const char* action =
    (otype == READ_ONLY) ? "read"
    : otype == WRITE_ONLY ? "write"
    : otype == READ_WRITE ? "read-write"
    : "UNKNOWN IO";
  printf("Process %c opened file %s for %s.\n", proc, filename_abspath, action);
  return versioned_open(pid, filename_abspath, otype);

}

// provenance-version a process-closes-file operation
// WARNING: assuming process only does one READ-open / WRITE-open / RW-open per file
VersionAction versioned_close (int pid, char* filename_abspath, OpenType otype) {

  if (graph == NULL)
    return (ERR_VERSIONING_NOT_INITIALIZED);

  struct node_entry* file_node = get_or_add_latest_versioned_node_for_file(filename_abspath);
  struct node_entry* pid_node = get_or_add_latest_versioned_node_for_pid(pid);

  if (otype == READ_ONLY) {
    disconnect(graph, file_node, pid_node);
  } else if (otype == WRITE_ONLY) {
    disconnect(graph, pid_node, file_node);
  } else if (otype == READ_WRITE) {
    disconnect(graph, file_node, pid_node);
    disconnect(graph, pid_node, file_node);
  } else {
    return (ERR_UNKNOWN_VERSION_ERR);
  }

  return (SUCCESS_FILE_VERSION_CLOSED);

}

// provenance-version a process-closes-file operation and log to stdout
VersionAction log_versioned_close (char proc, char* filename_abspath, OpenType otype) {

  int pid = convert_proc_char_to_pid(proc);
  const char* action =
    (otype == READ_ONLY) ? "read"
    : otype == WRITE_ONLY ? "write"
    : otype == READ_WRITE ? "read-write"
    : "UNKNOWN IO";
  printf("Process %c closed file %s (%s).\n", proc, filename_abspath, action);
  return versioned_close(pid, filename_abspath, otype);

}

// provenance-version a process-spawns-another-process operation
VersionAction versioned_spawn (int parent_pid, int child_pid) {

  if (graph == NULL)
    return (ERR_VERSIONING_NOT_INITIALIZED);

  struct node_entry* parent_node = get_or_add_latest_versioned_node_for_pid(parent_pid);
  struct node_entry* child_node = get_or_add_latest_versioned_node_for_pid(child_pid);

  connect(graph, parent_node, child_node);
  disconnect(graph, parent_node, child_node);

  return (SUCCESS_PROCESS_SPAWNED);

}

// provenance-version a process-closes-file operation and log to stdout
VersionAction log_versioned_spawn (char parent_proc, char child_proc) {

  int parent_pid = convert_proc_char_to_pid(parent_proc);
  int child_pid = convert_proc_char_to_pid(child_proc);
  printf("Process %c spawned process %c.\n", parent_proc, child_proc);
  return versioned_spawn(parent_pid, child_pid);

}

// print each versioning graph edge to stdout
void log_versioned_edges () {

  struct edge_entry *edge, *tmp;
  HASH_ITER(hh, graph->edge_table, edge, tmp) {
    struct node_entry* node1 = get_node_entry(graph, edge->node1_keystr);
    char node1_char = get_proc_char_from_node(node1);
    struct node_entry* node2 = get_node_entry(graph, edge->node2_keystr);
    char node2_char = get_proc_char_from_node(node2);

    printf("%c%d ---> %c%d\n", node1_char, node1->version_num, node2_char, node2->version_num);
  }

}

// query versioning graph to see if file modified since last prog run
VersionAction is_file_modified (char* filename_abspath) {

  if (graph == NULL)
    return (ERR_VERSIONING_NOT_INITIALIZED);

  struct node_entry* entry = get_node_entry_by_version(graph, filename_abspath, FIRST_VERSION_NUM);

  if (entry == NULL)
    return FILE_NOT_EXIST;
  else if (entry->modflag == MODIFIED)
    return FILE_MODIFIED;
  else if (entry->modflag == UNMODIFIED)
    return FILE_NOT_MODIFIED;
  else
    return ERR_UNKNOWN_VERSION_ERR;

}

// query versioning graph to see if process modified since last prog run
VersionAction is_process_modified (int pid) {

  if (graph == NULL)
    return (ERR_VERSIONING_NOT_INITIALIZED);

  char* pid_str;
  malloc_str_from_int(&pid_str, pid);
  struct node_entry* entry = get_node_entry_by_version(graph, pid_str, FIRST_VERSION_NUM);
  free(pid_str);

  if (entry == NULL)
    return PROCESS_NOT_EXIST;
  else if (entry->modflag == MODIFIED)
    return PROCESS_MODIFIED;
  else if (entry->modflag == UNMODIFIED)
    return PROCESS_NOT_MODIFIED;
  else
    return ERR_UNKNOWN_VERSION_ERR;

}

