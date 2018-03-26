/*******************************************************************************
module:   versioning_graph
author:   digimokan
date:     01 SEP 2017 (created)
purpose:  graph operations for a specially-versioned provenance graph
*******************************************************************************/

#ifndef VERSIONING_GRAPH_H
#define VERSIONING_GRAPH_H 1

// allow this header to be included from c++ source file
#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * USER INCLUDES
 ******************************************************************************/

#include "uthash.h"             // UT_hash_handle

/*******************************************************************************
 * PUBLIC TYPES / CONSTANTS / VARIABLES
 ******************************************************************************/

#define FIRST_VERSION_NUM 1

typedef enum {
  SUCCESS_VERSION_ENTRY_RETURNED,
  SUCCESS_VERSION_GRAPH_CLEARED,
  SUCCESS_EDGE_ADDED_TO_NODE,
  SUCCESS_NODES_DISCONNECTED,
  SUCCESS_NODE_AND_CHILDREN_MARKED,
  ERR_VERSION_ENTRY_NOT_EXIST,
  ERR_NODE_NOT_EXIST,
  ERR_CONNECTED_NODE_NOT_EXIST,
  ERR_NODE_NOT_EXIST_IN_EDGE,
  ERR_NO_ACTIVE_EDGE_BETWEEN_NODES,
  ERR_UNKNOWN_VERSION_GRAPH_ERR
} VersionGraphAction;

// type of the struct node_entry
typedef enum {
  FILE_NODE,
  PROCESS_NODE
} NodeType;

// mark field of the struct node_entry
typedef enum {
  MARKED,
  UNMARKED,
  MARKED_OR_UNMARKED
} Mark;

// modflag field of the struct node_entry
typedef enum {
  MODIFIED,
  UNMODIFIED,
  MODIFIED_OR_UNMODIFIED
} ModFlag;

// a version-numbered file/process node in the provenance graph
struct node_entry {
  char* keystr;                 // label + version_number
  char* label;                  // file/pid str
  int version_num;              // version num of this file/process
  Mark mark;                    // a node is marked when closed/inactive
  ModFlag modflag;                 // node (or parent) modified since prog last run
  NodeType ntype;               // file node or process node
  struct edge_entry_slink* edges;  // edges to/from this node (slink)
  UT_hash_handle hh;            // makes this structure uthash hashable
  UT_hash_handle hh_visited;    // make hashable (allow storage in mult tables)
  UT_hash_handle hh_collected;  // make hashable (allow storage in mult tables)
};

// label field of the struct edge_entry
typedef enum {
  ACTIVE,
  INACTIVE ,
  ACTIVE_OR_INACTIVE,
} EdgeLabel;

// direction of an edge
typedef enum {
  OUTBOUND,
  INBOUND ,
  OUTBOUND_OR_INBOUND,
} EdgeDirection;

// an edge in the provenance graph
struct edge_entry {
  char* keystr;                 // 1st node hash key + 2nd node hash key
  char* node1_keystr;           // 1st node hash key
  char* node2_keystr;           // 2nd node hash key
  EdgeLabel edge_label;         // active/inactive label relating to node marks
  UT_hash_handle hh;            // makes this structure uthash hashable
  UT_hash_handle hh_ne;         // make hashable (allow storage in mult tables)
};

// the latest/active version number for every pid/file
struct version_num_entry {
  char* keystr;                 // hash key: string pid or filename_abspath
  int version_num;              // increment on making new version
  UT_hash_handle hh;            // makes this structure uthash hashable
};

// node in a queue of node_entries
struct node_entry_node {
  struct node_entry* entry;
  struct node_entry_node* next;
};

// queue of node_entries
struct node_entry_queue {
  struct node_entry_node* head;
  struct node_entry_node* tail;
};

// node in a linked list of edge_entries
struct edge_entry_node {
  struct edge_entry* entry;
  struct edge_entry_node* next;
};

// singly-linked list of edge_entries
struct edge_entry_slink {
  struct edge_entry_node* head;
  struct edge_entry_node* tail;
};

// versioned provenance graph: set of nodes, edges, active node version nums
struct versioned_prov_graph {
  struct node_entry* node_table;
  struct edge_entry* edge_table;
  struct version_num_entry* version_num_table;
};

/*******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/

// create and return new versioned provenance graph
struct versioned_prov_graph* create_new_graph ();

// retrieve the current versioned provenance graph
VersionGraphAction clear_graph (struct versioned_prov_graph** graph);

// get version entry for file/pid from version_table
struct version_num_entry* get_version_num_entry (struct versioned_prov_graph* graph, char* version_num_entry_keystr);

// add version entry for file/pid to version_table
struct version_num_entry* add_version_num_entry (struct versioned_prov_graph* graph, char* version_num_entry_keystr);

// attempt to get a version entry for file/pid, add new entry if not exist
struct version_num_entry* get_or_add_version_num_entry (struct versioned_prov_graph* graph, char* version_num_entry_keystr);

// get current/active version number of a file
VersionGraphAction get_active_version_num (struct versioned_prov_graph* graph, char* version_num_entry_keystr, int* version_num);

// get node entry for versioned file/pid to node table
struct node_entry* get_node_entry_by_version (struct versioned_prov_graph* graph, char* node_label, int version_num);

// get node entry for versioned file/pid from node table
struct node_entry* get_node_entry (struct versioned_prov_graph* graph, char* node_entry_keystr);

// add node entry for versioned file/pid to node table
struct node_entry* add_node_entry (struct versioned_prov_graph* graph, char* node_label, int version_num, NodeType ntype);

// get current/active node entry for versioned file/pid, add new entry if not exist
struct node_entry* get_or_add_node_entry (struct versioned_prov_graph* graph, char* node_label, int version_num, NodeType ntype);

// get edge entry from edge table
struct edge_entry* get_edge_entry (struct versioned_prov_graph* graph, char* edge_entry_keystr);

// add edge entry to edge table
struct edge_entry* add_edge_entry (struct versioned_prov_graph* graph, char* node_entry1_keystr, char* node_entry2_keystr, EdgeLabel is_active);

// find edge entry from edge table by its node endpoints
struct edge_entry* find_edge_entry (struct versioned_prov_graph* graph, struct node_entry* node1, struct node_entry* node2);

// initialize queue of node_entries
struct node_entry_queue* make_node_entry_queue ();

// initialize singly-linked list of edge_entries
struct edge_entry_slink* make_edge_entry_slink ();

// create and return new node in node-entry-queue
struct node_entry_node* make_node_entry_node (struct node_entry* entry);

// create and return new node in edge-entry-list
struct edge_entry_node* make_edge_entry_node (struct edge_entry* entry);

// add node to tail of node-entry-queue
void enqueue (struct node_entry_queue* queue, struct node_entry* entry);

// add node to head of edge-entry-list
void slink_insert_at_head (struct edge_entry_slink* slink, struct edge_entry* entry);

// remove and return head of node-entry-queue
struct node_entry* dequeue (struct node_entry_queue* queue);

// get head node of edge-entry-list
struct edge_entry* slink_get_head (struct edge_entry_slink* slink);

// get node after input node in edge-entry-list
struct edge_entry* slink_get_next (struct edge_entry_node* een);

// return number of nodes in edge-entry list
struct edge_entry* slink_find_by_edge_keystr (struct edge_entry_slink* slink, char* keystr);

// return number of nodes in edge-entry list
size_t slink_count (struct edge_entry_slink* slink);

// clear/free all nodes of edge-entry-list (but not their edge_entry payloads)
void slink_clear (struct edge_entry_slink* slink);

// add an edge entry to a node entry's edge table
VersionGraphAction add_edge_to_node (struct versioned_prov_graph* graph, struct node_entry* node, struct edge_entry* edge);

// get latest version of a node by label, add it (and add version entry) if not exist
struct node_entry* retrieve_latest_versioned_node (struct versioned_prov_graph* graph, char* node_label, NodeType ntype);

// make and return a new incremented-version in the graph of the input node
struct node_entry* duplicate_node_entry (struct versioned_prov_graph* graph, struct node_entry* entry);

// add directed edge from node1 to node2 - add to graph and to node edge tables
struct edge_entry* link_nodes_with_edge (struct versioned_prov_graph* graph, struct node_entry* node1, struct node_entry* node2, EdgeLabel is_active);

// return table of is_marked nodes connected to start_node by is_active edge_direction edges
struct node_entry* collect_nodes_connected_by_target_edges (struct versioned_prov_graph* graph, struct node_entry* entry, Mark is_marked, EdgeLabel is_active, EdgeDirection edge_direction);

// connect one node to another node, versioning and creating nodes as required
void connect (struct versioned_prov_graph* graph, struct node_entry* node1, struct node_entry* node2);

// disconnect one node from another node, marking select related nodes as required
VersionGraphAction disconnect (struct versioned_prov_graph* graph, struct node_entry* node1, struct node_entry* node2);

// flag a node and descendents (i.e. connected by outbound edges) as mod/unmod
VersionGraphAction set_modflag_for_node_entry_and_descendents (struct versioned_prov_graph* graph, char* node_entry_keystr, ModFlag modflag);

// allow this header to be included from c++ source file
#ifdef __cplusplus
}
#endif

#endif // VERSIONING_GRAPH_H

