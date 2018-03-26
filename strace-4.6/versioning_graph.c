/*******************************************************************************
module:   versioning_graph
author:   digimokan
date:     01 SEP 2017 (created)
purpose:  graph operations for a specially-versioned provenance graph
*******************************************************************************/

/*******************************************************************************
 * SYSTEM INCLUDES
 ******************************************************************************/

#include <assert.h>             // ISOC: assert()

/*******************************************************************************
 * USER INCLUDES
 ******************************************************************************/

#include "versioning_graph.h"
#include "uthash.h"             // UT_hash_handle, HASH_ADD_KEYPTR(), HASH_FIND_STR(), HASH_DELETE(), HASH_ITER()
#include "strutils.h"           // malloc_str_from_two_strs()

/*******************************************************************************
 * PRIVATE MACROS / FUNCTIONS
 ******************************************************************************/

// initialize version_num_entry struct fields
static void init_version_num_entry (struct version_num_entry* entry, char* keystr) {
  entry->keystr = strdup(keystr);
  entry->version_num = 1;
}

// initialize node_entry struct fields
static void init_node_entry (struct node_entry* entry, char* label, int version_num,
    Mark is_marked, NodeType ntype) {
  malloc_str_from_str_plus_int(&(entry->keystr), label, version_num);
  entry->label = strdup(label);
  entry->version_num = version_num;
  entry->mark = is_marked;
  entry->modflag = UNMODIFIED;
  entry->ntype = ntype;
  entry->edges = make_edge_entry_slink();
}

// initialize edge_entry struct fields
static void init_edge_entry (struct edge_entry* entry, char* keystr1, char* keystr2,
    EdgeLabel is_active) {
  char* edge_keystr = NULL;
  malloc_str_from_two_strs(&edge_keystr, keystr1, keystr2);
  entry->keystr = edge_keystr;
  entry->node1_keystr = strdup(keystr1);
  entry->node2_keystr = strdup(keystr2);
  assert((is_active == ACTIVE) || (is_active == INACTIVE));
  entry->edge_label = is_active;
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/

// create, init, and return new versioned provenance graph
struct versioned_prov_graph* create_new_graph () {

  struct versioned_prov_graph* graph = malloc(sizeof(struct versioned_prov_graph));
  graph->node_table = NULL;
  graph->edge_table = NULL;
  graph->version_num_table = NULL;

  return graph;

}

// retrieve the current versioned provenance graph
VersionGraphAction clear_graph (struct versioned_prov_graph** graph) {

  // clear/free graph if it exists
  if (*graph) {

    // clear/free graph nodes that exist
    if ((*graph)->node_table) {
      struct node_entry* node;
      struct node_entry* tmp;
      HASH_ITER(hh, (*graph)->node_table, node, tmp) {
        HASH_DELETE(hh, (*graph)->node_table, node);
        free(node->label);
        free(node->keystr);
        free(node);
        slink_clear(node->edges);
        free(node->edges);
      }
      (*graph)->node_table = NULL;
    }

    // clear/free graph edges that exist
    if ((*graph)->edge_table) {
      struct edge_entry* edge;
      struct edge_entry* tmp;
      HASH_ITER(hh, (*graph)->edge_table, edge, tmp) {
        HASH_DELETE(hh, (*graph)->edge_table, edge);
        free(edge->node1_keystr);
        free(edge->node2_keystr);
        free(edge->keystr);
        free(edge);
      }
      (*graph)->edge_table = NULL;
    }

    // clear/free graph node-version-tracking entries that exist
    if ((*graph)->version_num_table) {
      struct version_num_entry* version_num;
      struct version_num_entry* tmp;
      HASH_ITER(hh, (*graph)->version_num_table, version_num, tmp) {
        HASH_DELETE(hh, (*graph)->version_num_table, version_num);
        free(version_num->keystr);
        free(version_num);
      }
      (*graph)->version_num_table = NULL;
    }

    free(*graph);
    *graph = NULL;

  }

  return (SUCCESS_VERSION_GRAPH_CLEARED);

}

// attempt to retrieve entry for file/pid from version_table
struct version_num_entry* get_version_num_entry (struct versioned_prov_graph* graph,
    char* version_num_entry_keystr) {

  struct version_num_entry* entry = NULL;
  HASH_FIND_STR(graph->version_num_table, version_num_entry_keystr, entry);
  return entry;

}

// add version entry for file/pid to version_table
struct version_num_entry* add_version_num_entry (struct versioned_prov_graph* graph,
    char* version_num_entry_keystr) {

  struct version_num_entry* entry = (struct version_num_entry*) malloc(sizeof(struct version_num_entry));
  init_version_num_entry(entry, version_num_entry_keystr);
  HASH_ADD_KEYPTR(hh, graph->version_num_table, entry->keystr, strlen(entry->keystr), entry);
  return entry;

}

// attempt to retrieve a version entry for file/pid, add new entry if not exist
struct version_num_entry* get_or_add_version_num_entry (struct versioned_prov_graph* graph,
    char* version_num_entry_keystr) {

  struct version_num_entry* entry = NULL;
  HASH_FIND_STR(graph->version_num_table, version_num_entry_keystr, entry);
  if (entry == NULL)
    entry = add_version_num_entry(graph, version_num_entry_keystr);
  return entry;

}

// get current/active version number of a file
VersionGraphAction get_active_version_num (struct versioned_prov_graph* graph,
    char* version_num_entry_keystr, int* version_num) {

  // hash table entry for this filename_abspath
  struct version_num_entry* entry = get_version_num_entry(graph, version_num_entry_keystr);

  if (entry == NULL) {
    return ERR_VERSION_ENTRY_NOT_EXIST;

  // version_num entry found in version_num table: set file_version_num
  } else {
    *version_num = entry->version_num;
    return SUCCESS_VERSION_ENTRY_RETURNED;
  }

}

// get node entry for versioned file/pid from node table
struct node_entry* get_node_entry (struct versioned_prov_graph* graph,
    char* node_entry_keystr) {

  struct node_entry* entry = NULL;
  HASH_FIND(hh, graph->node_table, node_entry_keystr, strlen(node_entry_keystr), entry);
  return entry;

}

// add node entry for versioned file/pid to node table
struct node_entry* add_node_entry (struct versioned_prov_graph* graph, char* node_label,
    int version_num, NodeType ntype) {

  struct node_entry* entry = (struct node_entry*) malloc(sizeof(struct node_entry));
  init_node_entry(entry, node_label, version_num, UNMARKED, ntype);
  HASH_ADD_KEYPTR(hh, graph->node_table, entry->keystr, strlen(entry->keystr), entry);
  return entry;

}

// get current/active node entry for versioned file/pid, add new entry if not exist
struct node_entry* get_or_add_node_entry (struct versioned_prov_graph* graph,
    char* node_label, int version_num, NodeType ntype) {

  char* node_entry_keystr = NULL;
  malloc_str_from_str_plus_int(&node_entry_keystr, node_label, version_num);
  struct node_entry* entry = NULL;
  HASH_FIND(hh, graph->node_table, node_entry_keystr, strlen(node_entry_keystr), entry);
  if (entry == NULL)
    entry = add_node_entry(graph, node_label, version_num, ntype);

  free(node_entry_keystr);
  return entry;

}

// get edge entry from edge table
struct edge_entry* get_edge_entry (struct versioned_prov_graph* graph, char* edge_entry_keystr) {

  struct edge_entry* entry = NULL;
  HASH_FIND(hh, graph->edge_table, edge_entry_keystr, strlen(edge_entry_keystr), entry);
  return entry;

}

// add edge entry to edge table
struct edge_entry* add_edge_entry (struct versioned_prov_graph* graph,
    char* node_entry1_keystr, char* node_entry2_keystr, EdgeLabel is_active) {

  char* edge_keystr;
  malloc_str_from_two_strs(&edge_keystr, node_entry1_keystr, node_entry2_keystr);
  struct edge_entry* entry = get_edge_entry(graph, edge_keystr);
  free(edge_keystr);

  if (entry) {
    assert((is_active == ACTIVE) || (is_active == INACTIVE));
    entry->edge_label = is_active;
  } else {
    entry = (struct edge_entry*) malloc(sizeof(struct edge_entry));
    init_edge_entry(entry, node_entry1_keystr, node_entry2_keystr, is_active);
    HASH_ADD_KEYPTR(hh, graph->edge_table, entry->keystr, strlen(entry->keystr), entry);
  }

  return entry;

}

// find edge entry from edge table by its node endpoints
struct edge_entry* find_edge_entry (struct versioned_prov_graph* graph, struct node_entry* node1, struct node_entry* node2) {

  char* edge_keystr;
  malloc_str_from_two_strs(&edge_keystr, node1->keystr, node2->keystr);
  struct edge_entry* entry = get_edge_entry(graph, edge_keystr);
  free(edge_keystr);
  return entry;

}

// initialize queue of node_entries
struct node_entry_queue* make_node_entry_queue () {

  struct node_entry_queue* queue = (struct node_entry_queue*) malloc(sizeof(struct node_entry_queue));
  queue->head = NULL;
  queue->tail = NULL;
  return (queue);

}

// initialize singly-linked list of edge_entries
struct edge_entry_slink* make_edge_entry_slink () {

  struct edge_entry_slink* slink = (struct edge_entry_slink*) malloc(sizeof(struct edge_entry_slink));
  slink->head = NULL;
  slink->tail = NULL;
  return (slink);

}

// create and return new node in node-entry-queue
struct node_entry_node* make_node_entry_node (struct node_entry* entry) {

  struct node_entry_node* nen = (struct node_entry_node*) malloc(sizeof(struct node_entry_node));
  nen->entry = entry;
  nen->next = NULL;
  return (nen);

}

// create and return new node in edge-entry-list
struct edge_entry_node* make_edge_entry_node (struct edge_entry* entry) {

  struct edge_entry_node* een = (struct edge_entry_node*) malloc(sizeof(struct edge_entry_node));
  een->entry = entry;
  een->next = NULL;
  return (een);

}

// add node to tail of node-entry-queue
void enqueue (struct node_entry_queue* queue, struct node_entry* entry) {

  if (queue->head == NULL) {
    queue->head = make_node_entry_node(entry);
    queue->tail = queue->head;
  } else {
    queue->tail->next = make_node_entry_node(entry);
    queue->tail = queue->tail->next;
  }

}

// add node to head of edge-entry-list
void slink_insert_at_head (struct edge_entry_slink* slink, struct edge_entry* entry) {

  struct edge_entry_node* new_node = make_edge_entry_node(entry);
  new_node->next = slink->head;
  slink->head = new_node;
  if (slink->tail == NULL)
    slink->tail = new_node;

}

// return number of nodes in edge-entry list
struct edge_entry* slink_find_by_edge_keystr (struct edge_entry_slink* slink, char* keystr) {

  struct edge_entry* found_entry = NULL;
  struct edge_entry_node* curr_entry = slink->head;
  while (curr_entry) {
    if (strncmp(curr_entry->entry->keystr, keystr, strlen(keystr)) == 0)
      return curr_entry->entry;
    curr_entry = curr_entry->next;
  }
  return (found_entry);

}

// return number of nodes in edge-entry list
size_t slink_count (struct edge_entry_slink* slink) {

  size_t count = 0;
  struct edge_entry_node* entry = slink->head;
  while (entry) {
    count++;
    entry = entry->next;
  }
  return (count);

}

// remove and return head of node-entry-queue
struct node_entry* dequeue (struct node_entry_queue* queue) {

  if (queue->head == NULL)
    return (NULL);
  else if (queue->head == queue->tail) {
    struct node_entry* entry = queue->head->entry;
    free(queue->head);
    queue->head = NULL;
    queue->tail = NULL;
    return (entry);
  } else {
    struct node_entry_node* nen = queue->head;
    struct node_entry* entry = queue->head->entry;
    queue->head = queue->head->next;
    free(nen);
    return (entry);
  }

}

// get head node of edge-entry-list
struct edge_entry* slink_get_head (struct edge_entry_slink* slink) {
  if (slink->head)
    return (slink->head->entry);
  else
    return (NULL);
}

// get node after input node in edge-entry-list
struct edge_entry* slink_get_next (struct edge_entry_node* een) {
  if (een->next == NULL)
    return (NULL);
  else
    return (een->next->entry);
}

// clear/free all nodes of edge-entry-list (but not their edge_entry payloads)
void slink_clear (struct edge_entry_slink* slink) {

  struct edge_entry_node* entry = slink->head;
  while (entry) {
    struct edge_entry_node* tmp = entry;
    entry = entry->next;
    free(tmp);
  }
  slink->head = NULL;
  slink->tail = NULL;

}

// add an edge entry to a node entry's edge table
VersionGraphAction add_edge_to_node (struct versioned_prov_graph* graph, struct node_entry* node,
    struct edge_entry* edge) {

  VersionGraphAction act = SUCCESS_EDGE_ADDED_TO_NODE;
  struct node_entry* connected_node = NULL;

  // node is 1st endpoint of edge: ensure 2nd endpoint exists in graph
  if (strncmp(node->keystr, edge->node1_keystr, strlen(node->keystr)) == 0) {
    HASH_FIND(hh, graph->node_table, edge->node2_keystr, strlen(edge->node2_keystr), connected_node);
    if (connected_node == NULL)
      act = ERR_CONNECTED_NODE_NOT_EXIST;

  // node is 2nd endpoint of edge: ensure 1st endpoint exists in graph
  } else if (strncmp(node->keystr, edge->node2_keystr, strlen(node->keystr)) == 0) {
    HASH_FIND(hh, graph->node_table, edge->node1_keystr, strlen(edge->node1_keystr), connected_node);
    if (connected_node == NULL)
      act = ERR_CONNECTED_NODE_NOT_EXIST;

  // node is not an endpoint of the edge: error
  } else {
    act = ERR_NODE_NOT_EXIST_IN_EDGE;
  }

  // node is endpoint of edge, and other endpoint exists in graph
  if (act == SUCCESS_EDGE_ADDED_TO_NODE)
    slink_insert_at_head(node->edges, edge);

  return (act);

}


// get latest version of a node by label, add it (and add version entry) if not exist
struct node_entry* retrieve_latest_versioned_node (struct versioned_prov_graph* graph,
    char* node_label, NodeType ntype) {

  struct version_num_entry* vn_entry = get_or_add_version_num_entry(graph, node_label);
  struct node_entry* n_entry = get_or_add_node_entry(graph, node_label, vn_entry->version_num, ntype);

  return (n_entry);

}

// make and return a new incremented-version in the graph of the input node
struct node_entry* duplicate_node_entry (struct versioned_prov_graph* graph, struct node_entry* entry) {

  struct version_num_entry* vn_entry = get_version_num_entry(graph, entry->label);
  vn_entry->version_num += 1;
  struct node_entry* dup_entry = add_node_entry(graph, entry->label, vn_entry->version_num, entry->ntype);

  return dup_entry;
}

// add directed edge from node1 to node2 - add to graph and to node edge tables
struct edge_entry* link_nodes_with_edge (struct versioned_prov_graph* graph,
    struct node_entry* node1, struct node_entry* node2, EdgeLabel is_active) {

  assert((is_active == ACTIVE) || (is_active == INACTIVE));
  struct edge_entry* edge = add_edge_entry(graph, node1->keystr, node2->keystr, is_active);
  add_edge_to_node(graph, node1, edge);
  add_edge_to_node(graph, node2, edge);

  return (edge);
}

// return table of is_marked nodes connected to start_node by is_active edge_direction edges
struct node_entry* collect_nodes_connected_by_target_edges (
    struct versioned_prov_graph* graph, struct node_entry* start_node,
    Mark is_marked, EdgeLabel is_active, EdgeDirection edge_direction) {

  struct node_entry_queue* search_queue = make_node_entry_queue();  // nodes to BFS
  struct node_entry* visited_table = NULL;    // nodes already BFSed
  struct node_entry* collected_table = NULL;  // the target nodes

  // enqueue 1st node in breadth-first-search queue and add to visited table
  enqueue(search_queue, start_node);
  HASH_ADD_KEYPTR(hh_visited, visited_table, start_node->keystr, strlen(start_node->keystr), start_node);

  for (struct node_entry* snode = dequeue(search_queue); snode; snode = dequeue(search_queue)) {

    // dequeued node matches is_marked param: add to coll table
    if ((is_marked == MARKED_OR_UNMARKED) || (snode->mark == is_marked))
      HASH_ADD_KEYPTR(hh_collected, collected_table, snode->keystr, strlen(snode->keystr), snode);

    // search dequeued node edges
    for (struct edge_entry_node* een = snode->edges->head; een != NULL; een = een->next) {
      struct edge_entry* n_edge = een->entry;  // dequeued node edge

      // dequeued node is 1st or 2nd endpoint of edge, depending on is_outbound
      char* dq_endpoint = (edge_direction == OUTBOUND)
        ? n_edge->node1_keystr
        : n_edge->node2_keystr;
      char* other_endpoint = (edge_direction == OUTBOUND)
        ? n_edge->node2_keystr
        : n_edge->node1_keystr;

      // edge matches is_active param and matches is_outbound from dequeued node
      if (((is_active == ACTIVE_OR_INACTIVE) || (n_edge->edge_label == is_active)) &&
          (strncmp(dq_endpoint, snode->keystr, strlen(snode->keystr)) == 0)) {
        // see if matching-edge-neighbor was already visited
        struct node_entry* neighbor = NULL;
        HASH_FIND(hh_visited, visited_table, other_endpoint, strlen(other_endpoint), neighbor);
        // was not already visited: add to queue
        if (neighbor == NULL) {
          neighbor = get_node_entry(graph, other_endpoint);
          enqueue(search_queue, neighbor);
          HASH_ADD_KEYPTR(hh_visited, visited_table, neighbor->keystr, strlen(neighbor->keystr), neighbor);
        }
      }

    }

  }

  free(search_queue);
  HASH_CLEAR(hh_visited, visited_table);

  return (collected_table);

}

// connect one node to another node, versioning and creating nodes as required
void connect (struct versioned_prov_graph* graph, struct node_entry* node1, struct node_entry* node2) {

  // collect marked nodes connected to node2 (INCLUDING node2) by active outbound edges from node2
  struct node_entry* collected_table = collect_nodes_connected_by_target_edges(graph, node2, MARKED, ACTIVE, OUTBOUND);

  // make new duplicated version of each collected node
  struct node_entry *coll_node, *tmp = NULL;
  HASH_ITER(hh_collected, collected_table, coll_node, tmp)
    duplicate_node_entry(graph, coll_node);

  // iterate through collected nodes
  HASH_ITER(hh_collected, collected_table, coll_node, tmp) {

    // create inactive edge from the collected node to its newly-duplicated version
    struct node_entry* dup_version = retrieve_latest_versioned_node(graph, coll_node->label, coll_node->ntype);
    link_nodes_with_edge(graph, coll_node, dup_version, INACTIVE);

    // iterate through collected node's edges
    for (struct edge_entry_node* een = coll_node->edges->head; een != NULL; een = een->next) {
      struct edge_entry* cn_edge = een->entry;  // an edge of coll_node

      // only iterate through active edges
      if (cn_edge->edge_label == INACTIVE)
        continue;

      // deactivate the active edge
      cn_edge->edge_label = INACTIVE;

      // collected node is 1st endpoint of edge
      if (strncmp(coll_node->keystr, cn_edge->node1_keystr, strlen(coll_node->keystr)) == 0) {

        // get 2nd endpoint node of edge
        struct node_entry* other_endpoint = get_node_entry(graph, cn_edge->node2_keystr);

        // check if 2nd endpoint node is in the set of collected nodes
        struct node_entry* other_in_collected = NULL;
        HASH_FIND(hh_collected, collected_table, other_endpoint->keystr, strlen(other_endpoint->keystr), other_in_collected);

        // 2nd endpoint IS in set: make edge from duped coll_node to 2nd endpoint's dup
        if (other_in_collected) {
          struct node_entry* other_dup_version = retrieve_latest_versioned_node(graph, other_endpoint->label, other_endpoint->ntype);
          link_nodes_with_edge(graph, dup_version, other_dup_version, ACTIVE);
        // 2nd endpoint node is NOT in set of collected nodes: make edge to 2nd endpoint
        } else {
          link_nodes_with_edge(graph, dup_version, other_endpoint, ACTIVE);
        }

      // collected node is 2nd endpoint of edge
      } else {

        // get 1st endpoint node of edge
        struct node_entry* other_endpoint = get_node_entry(graph, cn_edge->node1_keystr);

        // check if 1st endpoint node is in the set of collected nodes
        struct node_entry* other_in_collected = NULL;
        HASH_FIND(hh_collected, collected_table, other_endpoint->keystr, strlen(other_endpoint->keystr), other_in_collected);

        // 1st endpoint IS in set: make edge from 1st endpoint's dup to duped coll_node
        if (other_in_collected) {
          struct node_entry* other_dup_version = retrieve_latest_versioned_node(graph, other_endpoint->label, other_endpoint->ntype);
          link_nodes_with_edge(graph, other_dup_version, dup_version, ACTIVE);
        // 1st endpoint node is NOT in set of collected nodes: make edge from 1st endpoint
        } else {
          link_nodes_with_edge(graph, other_endpoint, dup_version, ACTIVE);
        }

      }

    }

  }

  // check if node1 param is in collected set: if so, use it
  struct node_entry* node1_in_collected = NULL;
  HASH_FIND(hh_collected, collected_table, node1->keystr, strlen(node1->keystr), node1_in_collected);
  struct node_entry* node1_prime = node1_in_collected
    ? retrieve_latest_versioned_node(graph, node1->label, node1->ntype)
    : node1;

  // check if node2 param is in collected set: if so, use it
  struct node_entry* node2_in_collected = NULL;
  HASH_FIND(hh_collected, collected_table, node2->keystr, strlen(node2->keystr), node2_in_collected);
  struct node_entry* node2_prime = node2_in_collected
    ? retrieve_latest_versioned_node(graph, node2->label, node2->ntype)
    : node2;

  // make edge from node1_prime to node2_prime
  link_nodes_with_edge(graph, node1_prime, node2_prime, ACTIVE);

  HASH_CLEAR(hh_collected, collected_table);

}

// disconnect one node from another node, marking select related nodes as required
VersionGraphAction disconnect (struct versioned_prov_graph* graph, struct node_entry* node1, struct node_entry* node2) {

  struct edge_entry* target_edge = NULL;

  // iterate through edges between node1 and node2
  for (struct edge_entry_node* een = node1->edges->head; een != NULL; een = een->next) {
    struct edge_entry* edge_1to2 = een->entry;  // an edge of node1

    // find the first active edge
    if (edge_1to2->edge_label == ACTIVE)
      target_edge = edge_1to2;
  }

  // no active edge between node1 and node2: discon performed before connect?
  if (target_edge == NULL)
    return (ERR_NO_ACTIVE_EDGE_BETWEEN_NODES);

  // collect unmarked nodes connected to node1 (INCLUDING node1) by active inbound edges
  struct node_entry* collected_table = collect_nodes_connected_by_target_edges(graph, node1, UNMARKED, ACTIVE, INBOUND);

  // mark all the collected nodes
  struct node_entry *coll_node, *tmp = NULL;
  HASH_ITER(hh_collected, collected_table, coll_node, tmp)
    coll_node->mark = MARKED;

  // deactivate the active edge between node1 and node2
  target_edge->edge_label = INACTIVE;

  HASH_CLEAR(hh_collected, collected_table);

  return SUCCESS_NODES_DISCONNECTED;

}

// flag a node and descendents (i.e. connected by outbound edges) as mod/unmod
VersionGraphAction set_modflag_for_node_and_descendents (struct versioned_prov_graph* graph, char* node_entry_keystr, ModFlag modflag) {
  struct node_entry* node = get_node_entry(graph, node_entry_keystr);

  assert((modflag == MODIFIED) || (modflag == UNMODIFIED));

  if (node == NULL)
    return ERR_NODE_NOT_EXIST;

  node->modflag = modflag;

  return SUCCESS_NODE_AND_CHILDREN_MARKED;
}

