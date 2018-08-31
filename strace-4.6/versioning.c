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
VersionAction versioned_open (char* executable_abspath, char* filename_abspath, OpenType otype) {

  if (graph == NULL)
    return (ERR_VERSIONING_NOT_INITIALIZED);

  struct node_entry* process_node = retrieve_latest_versioned_node(graph, executable_abspath, PROCESS_NODE);
  struct node_entry* file_node = retrieve_latest_versioned_node(graph, filename_abspath, FILE_NODE);

  if (otype == READ_ONLY) {
    connect(graph, file_node, process_node);
  } else if (otype == WRITE_ONLY) {
    connect(graph, process_node, file_node);
  } else if (otype == READ_WRITE) {
    connect(graph, file_node, process_node);
    connect(graph, process_node, file_node);
  } else {
    return (ERR_UNKNOWN_VERSION_ERR);
  }

  return (SUCCESS_FILE_VERSION_OPENED);

}

// provenance-version a process-opens-file operation and log to stdout
VersionAction log_versioned_open (char* executable_abspath, char* filename_abspath, OpenType otype) {

  const char* action =
    (otype == READ_ONLY) ? "read"
    : otype == WRITE_ONLY ? "write"
    : otype == READ_WRITE ? "read-write"
    : "UNKNOWN IO";
  printf("Process %s opened file %s for %s.\n", executable_abspath, filename_abspath, action);
  return versioned_open(executable_abspath, filename_abspath, otype);

}

// provenance-version a process-closes-file operation
// WARNING: assuming process only does one READ-open / WRITE-open / RW-open per file
VersionAction versioned_close (char* executable_abspath, char* filename_abspath, OpenType otype) {

  if (graph == NULL)
    return (ERR_VERSIONING_NOT_INITIALIZED);

  struct node_entry* process_node = retrieve_latest_versioned_node(graph, executable_abspath, PROCESS_NODE);
  struct node_entry* file_node = retrieve_latest_versioned_node(graph, filename_abspath, FILE_NODE);

  if (otype == READ_ONLY) {
    disconnect(graph, file_node, process_node);
  } else if (otype == WRITE_ONLY) {
    disconnect(graph, process_node, file_node);
  } else if (otype == READ_WRITE) {
    disconnect(graph, file_node, process_node);
    disconnect(graph, process_node, file_node);
  } else {
    return (ERR_UNKNOWN_VERSION_ERR);
  }

  return (SUCCESS_FILE_VERSION_CLOSED);

}

// provenance-version a process-closes-file operation and log to stdout
VersionAction log_versioned_close (char* executable_abspath, char* filename_abspath, OpenType otype) {

  const char* action =
    (otype == READ_ONLY) ? "read"
    : otype == WRITE_ONLY ? "write"
    : otype == READ_WRITE ? "read-write"
    : "UNKNOWN IO";
  printf("Process %s closed file %s (%s).\n", executable_abspath, filename_abspath, action);
  return versioned_close(executable_abspath, filename_abspath, otype);

}

// provenance-version a process-spawns-another-process operation
VersionAction versioned_spawn (char* parent_executable_abspath, char* child_executable_abspath) {

  if (graph == NULL)
    return (ERR_VERSIONING_NOT_INITIALIZED);

  struct node_entry* parent_node = retrieve_latest_versioned_node(graph, parent_executable_abspath, PROCESS_NODE);
  struct node_entry* child_node = retrieve_latest_versioned_node(graph, child_executable_abspath, PROCESS_NODE);

  connect(graph, parent_node, child_node);
  disconnect(graph, parent_node, child_node);

  return (SUCCESS_PROCESS_SPAWNED);

}

// provenance-version a process-closes-file operation and log to stdout
VersionAction log_versioned_spawn (char* parent_executable_abspath, char* child_executable_abspath) {

  printf("Process %s spawned process %s.\n", parent_executable_abspath, child_executable_abspath);
  return versioned_spawn(parent_executable_abspath, child_executable_abspath);

}

// print each versioning graph edge to stdout
void log_versioned_edges () {

  struct edge_entry *edge, *tmp;
  HASH_ITER(hh, graph->edge_table, edge, tmp) {
    struct node_entry* node1 = get_node_entry(graph, edge->node1_keystr);
    struct node_entry* node2 = get_node_entry(graph, edge->node2_keystr);

    printf("%s ---> %s\n", node1->keystr, node2->keystr);
  }

}

// query versioning graph to see if file or process modified since last prog run
VersionAction is_file_or_process_modified (char* filename_or_executable_abspath) {

  if (graph == NULL)
    return (ERR_VERSIONING_NOT_INITIALIZED);

  struct node_entry* entry = get_node_entry_by_version(graph, filename_or_executable_abspath, FIRST_VERSION_NUM);

  if (entry == NULL)
    return FILE_OR_PROCESS_NOT_EXIST;
  else if (entry->modflag == MODIFIED)
    return FILE_OR_PROCESS_MODIFIED;
  else if (entry->modflag == UNMODIFIED)
    return FILE_OR_PROCESS_NOT_MODIFIED;
  else
    return ERR_UNKNOWN_VERSION_ERR;

}

