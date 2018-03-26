/*******************************************************************************
module:   versioning_graph_test
author:   digimokan
date:     02 OCT 2017 (created)
purpose:  test cases for functions in strace-4.6/versioning_graph.c
*******************************************************************************/

#include "doctest.h"
#include "versioning_graph.h"

TEST_CASE("create_new_graph") {

  struct versioned_prov_graph* graph = NULL;

  SUBCASE("normal graph creation") {
    graph = create_new_graph();

    CHECK(graph);
    CHECK_FALSE(graph->node_table);
    CHECK_FALSE(graph->edge_table);
    CHECK_FALSE(graph->version_num_table);
  }

  clear_graph(&graph);

}

TEST_CASE("clear_graph") {

  struct versioned_prov_graph* graph = create_new_graph();
  VersionGraphAction act;

  SUBCASE("clear valid graph") {
    struct node_entry* node_table = graph->node_table;
    struct edge_entry* edge_table = graph->edge_table;
    struct version_num_entry* version_num_table = graph->version_num_table;

    act = clear_graph(&graph);

    CHECK(act == SUCCESS_VERSION_GRAPH_CLEARED);
    CHECK_FALSE(graph);
    CHECK_FALSE(node_table);
    CHECK_FALSE(edge_table);
    CHECK_FALSE(version_num_table);
  }

}

TEST_CASE("get_version_num_entry") {

  struct versioned_prov_graph* graph = create_new_graph();
  struct version_num_entry* entry = NULL;
  char* version_num_entry_keystr = strdup("some_file_path_str_OR_pid_str");

  SUBCASE("get version entry for non-existent file") {
    entry = get_version_num_entry(graph, version_num_entry_keystr);

    CHECK_FALSE(entry);
  }

  clear_graph(&graph);

}

TEST_CASE("add_version_num_entry") {

  struct versioned_prov_graph* graph = create_new_graph();
  struct version_num_entry* entry = NULL;
  struct version_num_entry* entry_from_get = NULL;
  char version_num_entry_keystr[] = "some_file_path_str_OR_pid_str";
  char version_num_entry_keystr2[] = "some_file_path_str_OR_pid_str2";
  char version_num_entry_keystr3[] = "some_file_path_str_OR_pid_str3";

  SUBCASE("add one entry: correct version_num_entry returned") {
    entry = add_version_num_entry(graph, version_num_entry_keystr);

    REQUIRE(entry);
    CHECK(strncmp(entry->keystr, version_num_entry_keystr, strlen(version_num_entry_keystr)) == 0);
    CHECK(strlen(entry->keystr) == strlen(version_num_entry_keystr));
    CHECK(entry->version_num == 1);

    entry_from_get = get_version_num_entry(graph, version_num_entry_keystr);

    CHECK(entry);
    CHECK(entry_from_get == entry);
  }

  SUBCASE("add three entries: correct version_num_entries returned") {
    add_version_num_entry(graph, version_num_entry_keystr);
    add_version_num_entry(graph, version_num_entry_keystr2);
    add_version_num_entry(graph, version_num_entry_keystr3);

    entry = get_version_num_entry(graph, version_num_entry_keystr);

    REQUIRE(entry);
    CHECK(strncmp(entry->keystr, version_num_entry_keystr, strlen(version_num_entry_keystr)) == 0);
    CHECK(strlen(entry->keystr) == strlen(version_num_entry_keystr));
    CHECK(entry->version_num == 1);

    entry = get_version_num_entry(graph, version_num_entry_keystr2);

    REQUIRE(entry);
    CHECK(strncmp(entry->keystr, version_num_entry_keystr2, strlen(version_num_entry_keystr2)) == 0);
    CHECK(strlen(entry->keystr) == strlen(version_num_entry_keystr2));
    CHECK(entry->version_num == 1);

    entry = get_version_num_entry(graph, version_num_entry_keystr3);

    REQUIRE(entry);
    CHECK(strncmp(entry->keystr, version_num_entry_keystr3, strlen(version_num_entry_keystr3)) == 0);
    CHECK(strlen(entry->keystr) == strlen(version_num_entry_keystr3));
    CHECK(entry->version_num == 1);
  }

  clear_graph(&graph);

}

TEST_CASE("get_or_add_version_num_entry") {

  struct versioned_prov_graph* graph = create_new_graph();
  struct version_num_entry* entry = NULL;
  struct version_num_entry* entry_from_get = NULL;
  char version_num_entry_keystr[] = "some_file_path_str_OR_pid_str";
  char version_num_entry_keystr2[] = "some_file_path_str_OR_pid_str2";
  char version_num_entry_keystr3[] = "some_file_path_str_OR_pid_str3";

  SUBCASE("get_or_add one entry: correct version_num_entry returned") {
    entry = get_or_add_version_num_entry(graph, version_num_entry_keystr);

    REQUIRE(entry);
    CHECK(strncmp(entry->keystr, version_num_entry_keystr, strlen(version_num_entry_keystr)) == 0);
    CHECK(strlen(entry->keystr) == strlen(version_num_entry_keystr));
    CHECK(entry->version_num == 1);

    entry_from_get = get_version_num_entry(graph, version_num_entry_keystr);

    CHECK(entry);
    CHECK(entry_from_get == entry);
  }

  SUBCASE("get_or_add same entry: original version_num_entry returned") {
    entry = get_or_add_version_num_entry(graph, version_num_entry_keystr);
    entry->version_num = 2;
    entry = get_or_add_version_num_entry(graph, version_num_entry_keystr);

    REQUIRE(entry);
    CHECK(strncmp(entry->keystr, version_num_entry_keystr, strlen(version_num_entry_keystr)) == 0);
    CHECK(strlen(entry->keystr) == strlen(version_num_entry_keystr));
    CHECK(entry->version_num == 2);
  }

  SUBCASE("get_or_add three entries: correct version_num_entries returned") {
    get_or_add_version_num_entry(graph, version_num_entry_keystr);
    get_or_add_version_num_entry(graph, version_num_entry_keystr2);
    get_or_add_version_num_entry(graph, version_num_entry_keystr3);

    entry = get_version_num_entry(graph, version_num_entry_keystr);

    REQUIRE(entry);
    CHECK(strncmp(entry->keystr, version_num_entry_keystr, strlen(version_num_entry_keystr)) == 0);
    CHECK(strlen(entry->keystr) == strlen(version_num_entry_keystr));
    CHECK(entry->version_num == 1);

    entry = get_version_num_entry(graph, version_num_entry_keystr2);

    REQUIRE(entry);
    CHECK(strncmp(entry->keystr, version_num_entry_keystr2, strlen(version_num_entry_keystr2)) == 0);
    CHECK(strlen(entry->keystr) == strlen(version_num_entry_keystr2));
    CHECK(entry->version_num == 1);

    entry = get_version_num_entry(graph, version_num_entry_keystr3);

    REQUIRE(entry);
    CHECK(strncmp(entry->keystr, version_num_entry_keystr3, strlen(version_num_entry_keystr3)) == 0);
    CHECK(strlen(entry->keystr) == strlen(version_num_entry_keystr3));
    CHECK(entry->version_num == 1);
  }

  clear_graph(&graph);

}

TEST_CASE("get_active_version_num") {

  struct versioned_prov_graph* graph = create_new_graph();
  struct version_num_entry* entry = NULL;
  char version_num_entry_keystr[] = "some_file_path_str_OR_pid_str";
  int vnum = 0;
  VersionGraphAction act;

  SUBCASE("get active_version_num for keystr with no version_entry") {
    act = get_active_version_num(graph, version_num_entry_keystr, &vnum);

    CHECK(act == ERR_VERSION_ENTRY_NOT_EXIST);
  }

  SUBCASE("get active_version_num for keystr with version_entry") {
    add_version_num_entry(graph, version_num_entry_keystr);

    act = get_active_version_num(graph, version_num_entry_keystr, &vnum);

    CHECK(act == SUCCESS_VERSION_ENTRY_RETURNED);
    CHECK(vnum == 1);
  }

  SUBCASE("get active_version_num for keystr with modified version_entry") {
    entry = add_version_num_entry(graph, version_num_entry_keystr);
    entry->version_num = 9;

    act = get_active_version_num(graph, version_num_entry_keystr, &vnum);

    CHECK(act == SUCCESS_VERSION_ENTRY_RETURNED);
    CHECK(vnum == 9);
  }

  clear_graph(&graph);

}

TEST_CASE("get_node_entry") {

  struct versioned_prov_graph* graph = create_new_graph();
  struct node_entry* entry = NULL;
  char node_entry_keystr[] = "some_file_path_str_OR_pid_str_with_VNUM";

  SUBCASE("get node entry for non-existent versioned file/pid") {
    entry = get_node_entry(graph, node_entry_keystr);

    CHECK_FALSE(entry);
  }

  clear_graph(&graph);

}

TEST_CASE("add_node_entry") {

  struct versioned_prov_graph* graph = create_new_graph();
  struct node_entry* entry = NULL;
  struct node_entry* entry_from_get = NULL;
  char node_entry_label[] = "some_file_path_str";
  int node_version = 1;
  char node_entry_keystr[] = "some_file_path_str1";
  char node_entry_label2[] = "some_file_path_str";
  int node_version2 = 2;
  char node_entry_keystr2[] = "some_file_path_str2";
  char node_entry_label3[] = "some_pid_str";
  int node_version3 = 1;
  char node_entry_keystr3[] = "some_pid_str1";

  SUBCASE("add one entry: correct node_entry returned") {
    entry = add_node_entry(graph, node_entry_label, node_version, FILE_NODE);

    REQUIRE(entry);
    CHECK(strncmp(entry->label, node_entry_label, strlen(node_entry_label)) == 0);
    CHECK(strlen(entry->label) == strlen(node_entry_label));
    CHECK(strncmp(entry->keystr, node_entry_keystr, strlen(node_entry_keystr)) == 0);
    CHECK(strlen(entry->keystr) == strlen(node_entry_keystr));
    CHECK(entry->version_num == 1);
    CHECK(entry->mark == UNMARKED);
    CHECK(entry->modflag == UNMODIFIED);
    CHECK(entry->ntype == FILE_NODE);
    CHECK(slink_count(entry->edges) == 0);

    entry_from_get = get_node_entry(graph, node_entry_keystr);

    CHECK(entry_from_get);
    CHECK(entry_from_get == entry);
  }

  SUBCASE("add three entries: correct node_entries returned") {
    add_node_entry(graph, node_entry_label, node_version, FILE_NODE);
    add_node_entry(graph, node_entry_label2, node_version2, FILE_NODE);
    add_node_entry(graph, node_entry_label3, node_version3, PROCESS_NODE);

    entry = get_node_entry(graph, node_entry_keystr);

    REQUIRE(entry);
    CHECK(strncmp(entry->label, node_entry_label, strlen(node_entry_label)) == 0);
    CHECK(strlen(entry->label) == strlen(node_entry_label));
    CHECK(strncmp(entry->keystr, node_entry_keystr, strlen(node_entry_keystr)) == 0);
    CHECK(strlen(entry->keystr) == strlen(node_entry_keystr));
    CHECK(entry->version_num == 1);
    CHECK(entry->mark == UNMARKED);
    CHECK(entry->modflag == UNMODIFIED);
    CHECK(entry->ntype == FILE_NODE);
    CHECK(slink_count(entry->edges) == 0);

    entry = get_node_entry(graph, node_entry_keystr2);

    REQUIRE(entry);
    CHECK(strncmp(entry->label, node_entry_label2, strlen(node_entry_label2)) == 0);
    CHECK(strlen(entry->label) == strlen(node_entry_label2));
    CHECK(strncmp(entry->keystr, node_entry_keystr2, strlen(node_entry_keystr2)) == 0);
    CHECK(strlen(entry->keystr) == strlen(node_entry_keystr2));
    CHECK(entry->version_num == 2);
    CHECK(entry->mark == UNMARKED);
    CHECK(entry->modflag == UNMODIFIED);
    CHECK(entry->ntype == FILE_NODE);
    CHECK(slink_count(entry->edges) == 0);

    entry = get_node_entry(graph, node_entry_keystr3);

    REQUIRE(entry);
    CHECK(strncmp(entry->label, node_entry_label3, strlen(node_entry_label3)) == 0);
    CHECK(strlen(entry->label) == strlen(node_entry_label3));
    CHECK(strncmp(entry->keystr, node_entry_keystr3, strlen(node_entry_keystr3)) == 0);
    CHECK(strlen(entry->keystr) == strlen(node_entry_keystr3));
    CHECK(entry->version_num == 1);
    CHECK(entry->mark == UNMARKED);
    CHECK(entry->modflag == UNMODIFIED);
    CHECK(entry->ntype == PROCESS_NODE);
    CHECK(slink_count(entry->edges) == 0);
  }

  clear_graph(&graph);

}

TEST_CASE("get_or_add_node_entry") {

  struct versioned_prov_graph* graph = create_new_graph();
  struct node_entry* entry = NULL;
  struct node_entry* entry_from_get = NULL;
  char node_entry_label[] = "some_file_path_str";
  int node_version = 1;
  char node_entry_keystr[] = "some_file_path_str1";
  char node_entry_label2[] = "some_file_path_str";
  int node_version2 = 2;
  char node_entry_keystr2[] = "some_file_path_str2";
  char node_entry_label3[] = "some_pid_str";
  int node_version3 = 1;
  char node_entry_keystr3[] = "some_pid_str1";

  SUBCASE("get_or_add one entry: correct version_num_entry returned") {
    entry = get_or_add_node_entry(graph, node_entry_label, node_version, FILE_NODE);

    REQUIRE(entry);
    CHECK(strncmp(entry->label, node_entry_label, strlen(node_entry_label)) == 0);
    CHECK(strlen(entry->label) == strlen(node_entry_label));
    CHECK(strncmp(entry->keystr, node_entry_keystr, strlen(node_entry_keystr)) == 0);
    CHECK(strlen(entry->keystr) == strlen(node_entry_keystr));
    CHECK(entry->version_num == 1);
    CHECK(entry->mark == UNMARKED);
    CHECK(entry->modflag == UNMODIFIED);
    CHECK(entry->ntype == FILE_NODE);
    CHECK(slink_count(entry->edges) == 0);

    entry_from_get = get_node_entry(graph, node_entry_keystr);

    CHECK(entry);
    CHECK(entry_from_get == entry);
  }

  SUBCASE("get_or_add same entry: original version_num_entry returned") {
    entry = get_or_add_node_entry(graph, node_entry_label, node_version, FILE_NODE);
    entry->version_num = 2;
    entry = get_or_add_node_entry(graph, node_entry_label, node_version, FILE_NODE);

    REQUIRE(entry);
    CHECK(strncmp(entry->keystr, node_entry_keystr, strlen(node_entry_keystr)) == 0);
    CHECK(strlen(entry->keystr) == strlen(node_entry_keystr));
    CHECK(entry->version_num == 2);
  }

  SUBCASE("get_or_add three entries: correct version_num_entries returned") {
    get_or_add_node_entry(graph, node_entry_label, node_version, FILE_NODE);
    get_or_add_node_entry(graph, node_entry_label2, node_version2, FILE_NODE);
    get_or_add_node_entry(graph, node_entry_label3, node_version3, PROCESS_NODE);

    entry = get_node_entry(graph, node_entry_keystr);

    REQUIRE(entry);
    CHECK(strncmp(entry->label, node_entry_label, strlen(node_entry_label)) == 0);
    CHECK(strlen(entry->label) == strlen(node_entry_label));
    CHECK(strncmp(entry->keystr, node_entry_keystr, strlen(node_entry_keystr)) == 0);
    CHECK(strlen(entry->keystr) == strlen(node_entry_keystr));
    CHECK(entry->version_num == 1);
    CHECK(entry->mark == UNMARKED);
    CHECK(entry->modflag == UNMODIFIED);
    CHECK(entry->ntype == FILE_NODE);
    CHECK(slink_count(entry->edges) == 0);

    entry = get_node_entry(graph, node_entry_keystr2);

    REQUIRE(entry);
    CHECK(strncmp(entry->label, node_entry_label2, strlen(node_entry_label2)) == 0);
    CHECK(strlen(entry->label) == strlen(node_entry_label2));
    CHECK(strncmp(entry->keystr, node_entry_keystr2, strlen(node_entry_keystr2)) == 0);
    CHECK(strlen(entry->keystr) == strlen(node_entry_keystr2));
    CHECK(entry->version_num == 2);
    CHECK(entry->mark == UNMARKED);
    CHECK(entry->modflag == UNMODIFIED);
    CHECK(entry->ntype == FILE_NODE);
    CHECK(slink_count(entry->edges) == 0);

    entry = get_node_entry(graph, node_entry_keystr3);

    REQUIRE(entry);
    CHECK(strncmp(entry->label, node_entry_label3, strlen(node_entry_label3)) == 0);
    CHECK(strlen(entry->label) == strlen(node_entry_label3));
    CHECK(strncmp(entry->keystr, node_entry_keystr3, strlen(node_entry_keystr3)) == 0);
    CHECK(strlen(entry->keystr) == strlen(node_entry_keystr3));
    CHECK(entry->version_num == 1);
    CHECK(entry->mark == UNMARKED);
    CHECK(entry->modflag == UNMODIFIED);
    CHECK(entry->ntype == PROCESS_NODE);
    CHECK(slink_count(entry->edges) == 0);
  }

  clear_graph(&graph);

}

TEST_CASE("get_edge_entry") {

  struct versioned_prov_graph* graph = create_new_graph();
  struct edge_entry* entry = NULL;
  char edge_entry_keystr[] = "some_file_path_V1some_pid_V2";

  SUBCASE("get edge entry for non-existent edge") {
    entry = get_edge_entry(graph, edge_entry_keystr);

    CHECK_FALSE(entry);
  }

  clear_graph(&graph);

}

TEST_CASE("add_edge_entry") {

  struct versioned_prov_graph* graph = create_new_graph();
  struct edge_entry* entry = NULL;
  struct edge_entry* entry_from_get = NULL;
  char node_entry_keystr[] = "some_file_path_str_V1";
  char node_entry_keystr2[] = "some_file_path_str_V2";
  char edge_entry_keystr1[] = "some_file_path_str_V1some_file_path_str_V2";
  char node_entry_keystr3[] = "some_pid_str_V1";
  char node_entry_keystr4[] = "some_file_path_str_V3";
  char edge_entry_keystr2[] = "some_pid_str_V1some_file_path_str_V3";
  char node_entry_keystr5[] = "some_file_path_str_V4";
  char node_entry_keystr6[] = "some_pid_str_V2";
  char edge_entry_keystr3[] = "some_file_path_str_V4some_pid_str_V2";

  SUBCASE("add one entry: correct node_entry returned") {
    entry = add_edge_entry(graph, node_entry_keystr, node_entry_keystr2, ACTIVE);

    REQUIRE(entry);
    CHECK(strncmp(entry->keystr, edge_entry_keystr1, strlen(edge_entry_keystr1)) == 0);
    CHECK(strlen(entry->keystr) == strlen(edge_entry_keystr1));
    CHECK(strncmp(entry->node1_keystr, node_entry_keystr, strlen(node_entry_keystr)) == 0);
    CHECK(strlen(entry->node1_keystr) == strlen(node_entry_keystr));
    CHECK(strncmp(entry->node2_keystr, node_entry_keystr2, strlen(node_entry_keystr2)) == 0);
    CHECK(strlen(entry->node2_keystr) == strlen(node_entry_keystr2));
    CHECK(entry->edge_label == ACTIVE);

    entry_from_get = get_edge_entry(graph, edge_entry_keystr1);

    CHECK(entry_from_get);
    CHECK(entry_from_get == entry);
  }

  SUBCASE("add three entries: correct node_entries returned") {
    add_edge_entry(graph, node_entry_keystr, node_entry_keystr2, ACTIVE);
    add_edge_entry(graph, node_entry_keystr3, node_entry_keystr4, ACTIVE);
    add_edge_entry(graph, node_entry_keystr5, node_entry_keystr6, INACTIVE);

    entry = get_edge_entry(graph, edge_entry_keystr1);

    REQUIRE(entry);
    CHECK(strncmp(entry->keystr, edge_entry_keystr1, strlen(edge_entry_keystr1)) == 0);
    CHECK(strlen(entry->keystr) == strlen(edge_entry_keystr1));
    CHECK(strncmp(entry->node1_keystr, node_entry_keystr, strlen(node_entry_keystr)) == 0);
    CHECK(strlen(entry->node1_keystr) == strlen(node_entry_keystr));
    CHECK(strncmp(entry->node2_keystr, node_entry_keystr2, strlen(node_entry_keystr2)) == 0);
    CHECK(strlen(entry->node2_keystr) == strlen(node_entry_keystr2));
    CHECK(entry->edge_label == ACTIVE);

    entry = get_edge_entry(graph, edge_entry_keystr2);

    REQUIRE(entry);
    CHECK(strncmp(entry->keystr, edge_entry_keystr2, strlen(edge_entry_keystr2)) == 0);
    CHECK(strlen(entry->keystr) == strlen(edge_entry_keystr2));
    CHECK(strncmp(entry->node1_keystr, node_entry_keystr3, strlen(node_entry_keystr3)) == 0);
    CHECK(strlen(entry->node1_keystr) == strlen(node_entry_keystr3));
    CHECK(strncmp(entry->node2_keystr, node_entry_keystr4, strlen(node_entry_keystr4)) == 0);
    CHECK(strlen(entry->node2_keystr) == strlen(node_entry_keystr4));
    CHECK(entry->edge_label == ACTIVE);

    entry = get_edge_entry(graph, edge_entry_keystr3);

    REQUIRE(entry);
    CHECK(strncmp(entry->keystr, edge_entry_keystr3, strlen(edge_entry_keystr3)) == 0);
    CHECK(strlen(entry->keystr) == strlen(edge_entry_keystr3));
    CHECK(strncmp(entry->node1_keystr, node_entry_keystr5, strlen(node_entry_keystr5)) == 0);
    CHECK(strlen(entry->node1_keystr) == strlen(node_entry_keystr5));
    CHECK(strncmp(entry->node2_keystr, node_entry_keystr6, strlen(node_entry_keystr6)) == 0);
    CHECK(strlen(entry->node2_keystr) == strlen(node_entry_keystr6));
    CHECK(entry->edge_label == INACTIVE);

  }

  clear_graph(&graph);

}

TEST_CASE("add_edge_to_node") {

  struct versioned_prov_graph* graph = create_new_graph();
  VersionGraphAction act;

  struct node_entry* node1 = NULL;
  char node_keystr1[] = "some_file_path_str1";
  char node_label1[] = "some_file_path_str";
  int node_version1 = 1;

  struct node_entry* node2 = NULL;
  char node_keystr2[] = "some_other_file_path_str1";
  char node_label2[] = "some_other_file_path_str";
  int node_version2 = 1;

  struct node_entry* node3 = NULL;
  char node_keystr3[] = "some_another_file_path_str1";
  char node_label3[] = "some_another_file_path_str";
  int node_version3 = 1;

  struct edge_entry* edge1 = NULL;
  struct edge_entry* edge1to2 = NULL;
  struct edge_entry* edge1to3 = NULL;
  struct edge_entry* edge2to1 = NULL;
  struct edge_entry* edge2to3 = NULL;
  struct edge_entry* edge3to1 = NULL;
  struct edge_entry* edge3to2 = NULL;

  SUBCASE("node is 1st endpoint of edge, 2nd endpoint not exist") {
    node1 = add_node_entry(graph, node_label1, node_version1, FILE_NODE);
    edge1 = add_edge_entry(graph, node_keystr1, node_keystr2, ACTIVE);
    act = add_edge_to_node(graph, node1, edge1);

    CHECK(act == ERR_CONNECTED_NODE_NOT_EXIST);
  }

  SUBCASE("node is 2nd endpoint of edge, 1st endpoint not exist") {
    node1 = add_node_entry(graph, node_label1, node_version1, FILE_NODE);
    edge1 = add_edge_entry(graph, node_keystr2, node_keystr1, ACTIVE);
    act = add_edge_to_node(graph, node1, edge1);

    CHECK(act == ERR_CONNECTED_NODE_NOT_EXIST);
  }

  SUBCASE("node is not an endpoint of the edge") {
    node1 = add_node_entry(graph, node_label1, node_version1, FILE_NODE);
    edge1 = add_edge_entry(graph, node_keystr2, node_keystr3, ACTIVE);
    act = add_edge_to_node(graph, node1, edge1);

    CHECK(act == ERR_NODE_NOT_EXIST_IN_EDGE);
  }

  SUBCASE("node is 1st endpoint of edge, other endpoint exists in graph") {
    node1 = add_node_entry(graph, node_label1, node_version1, FILE_NODE);
    node2 = add_node_entry(graph, node_label2, node_version2, FILE_NODE);
    edge1 = add_edge_entry(graph, node_keystr1, node_keystr2, ACTIVE);
    act = add_edge_to_node(graph, node1, edge1);

    CHECK(act == SUCCESS_EDGE_ADDED_TO_NODE);
    CHECK(slink_find_by_edge_keystr(node1->edges, edge1->keystr) == edge1);
  }

  SUBCASE("node is 2nd endpoint of edge, other endpoint exists in graph") {
    node1 = add_node_entry(graph, node_label1, node_version1, FILE_NODE);
    node2 = add_node_entry(graph, node_label2, node_version2, FILE_NODE);
    edge1 = add_edge_entry(graph, node_keystr2, node_keystr1, ACTIVE);
    act = add_edge_to_node(graph, node1, edge1);

    CHECK(act == SUCCESS_EDGE_ADDED_TO_NODE);
    CHECK(slink_find_by_edge_keystr(node1->edges, edge1->keystr) == edge1);
  }

  SUBCASE("make \"connected\" graph of three nodes and six edges") {
    node1 = add_node_entry(graph, node_label1, node_version1, FILE_NODE);
    node2 = add_node_entry(graph, node_label2, node_version2, FILE_NODE);
    node3 = add_node_entry(graph, node_label3, node_version3, FILE_NODE);
    edge1to2 = add_edge_entry(graph, node_keystr1, node_keystr2, ACTIVE);
    edge1to3 = add_edge_entry(graph, node_keystr1, node_keystr3, INACTIVE);
    edge2to1 = add_edge_entry(graph, node_keystr2, node_keystr1, ACTIVE);
    edge2to3 = add_edge_entry(graph, node_keystr2, node_keystr3, INACTIVE);
    edge3to1 = add_edge_entry(graph, node_keystr3, node_keystr2, ACTIVE);
    edge3to2 = add_edge_entry(graph, node_keystr3, node_keystr1, INACTIVE);

    act = add_edge_to_node(graph, node1, edge1to2);
    CHECK(act == SUCCESS_EDGE_ADDED_TO_NODE);

    act = add_edge_to_node(graph, node1, edge1to3);
    CHECK(act == SUCCESS_EDGE_ADDED_TO_NODE);

    act = add_edge_to_node(graph, node2, edge2to1);
    CHECK(act == SUCCESS_EDGE_ADDED_TO_NODE);

    act = add_edge_to_node(graph, node2, edge2to3);
    CHECK(act == SUCCESS_EDGE_ADDED_TO_NODE);

    act = add_edge_to_node(graph, node3, edge3to1);
    CHECK(act == SUCCESS_EDGE_ADDED_TO_NODE);

    act = add_edge_to_node(graph, node3, edge3to2);
    CHECK(act == SUCCESS_EDGE_ADDED_TO_NODE);

    CHECK(slink_find_by_edge_keystr(node1->edges, edge1to2->keystr) == edge1to2);
    CHECK(slink_find_by_edge_keystr(node1->edges, edge1to3->keystr) == edge1to3);
    CHECK(slink_find_by_edge_keystr(node2->edges, edge2to1->keystr) == edge2to1);
    CHECK(slink_find_by_edge_keystr(node2->edges, edge2to3->keystr) == edge2to3);
    CHECK(slink_find_by_edge_keystr(node3->edges, edge3to1->keystr) == edge3to1);
    CHECK(slink_find_by_edge_keystr(node3->edges, edge3to2->keystr) == edge3to2);
  }

  clear_graph(&graph);

}

TEST_CASE("retrieve_latest_versioned_node") {

  struct versioned_prov_graph* graph = create_new_graph();
  struct node_entry* entry = NULL;
  struct node_entry* entry_same = NULL;
  char node_entry_label[] = "some_file_path_str";
  char node_entry_keystr[] = "some_file_path_str1";
  struct version_num_entry* vn_entry = NULL;

  SUBCASE("get versioned node for nonexistent node") {
    entry = retrieve_latest_versioned_node(graph, node_entry_label, FILE_NODE);

    REQUIRE(entry);
    CHECK(strncmp(entry->keystr, node_entry_keystr, strlen(node_entry_keystr)) == 0);
    CHECK(strlen(entry->keystr) == strlen(node_entry_keystr));
    CHECK(strncmp(entry->label, node_entry_label, strlen(node_entry_label)) == 0);
    CHECK(strlen(entry->label) == strlen(node_entry_label));
    CHECK(entry->version_num == 1);
    CHECK(entry->mark == UNMARKED);
    CHECK(entry->modflag == UNMODIFIED);
    CHECK(entry->ntype == FILE_NODE);
    CHECK(slink_count(entry->edges) == 0);

    vn_entry = get_version_num_entry(graph, node_entry_label);

    REQUIRE(vn_entry);
    CHECK(vn_entry->version_num == 1);
  }

  SUBCASE("get versioned node for existent node") {
    entry = retrieve_latest_versioned_node(graph, node_entry_label, FILE_NODE);
    entry_same = retrieve_latest_versioned_node(graph, node_entry_label, FILE_NODE);

    REQUIRE(entry_same);
    CHECK(strncmp(entry_same->keystr, node_entry_keystr, strlen(node_entry_keystr)) == 0);
    CHECK(strlen(entry_same->keystr) == strlen(node_entry_keystr));
    CHECK(strncmp(entry_same->label, node_entry_label, strlen(node_entry_label)) == 0);
    CHECK(strlen(entry_same->label) == strlen(node_entry_label));
    CHECK(entry_same->version_num == 1);
    CHECK(entry_same->mark == UNMARKED);
    CHECK(entry->modflag == UNMODIFIED);
    CHECK(entry_same->ntype == FILE_NODE);
    CHECK(slink_count(entry_same->edges) == 0);

    vn_entry = get_version_num_entry(graph, node_entry_label);

    REQUIRE(vn_entry);
    CHECK(vn_entry->version_num == 1);
  }

  clear_graph(&graph);

}

TEST_CASE("duplicate_node_entry") {

  struct versioned_prov_graph* graph = create_new_graph();
  struct node_entry* entry = NULL;
  struct node_entry* entry2 = NULL;
  char node_entry_label[] = "some_file_path_str";
  struct node_entry* dup_entry = NULL;
  char dup_entry_keystr[] = "some_file_path_str2";
  struct node_entry* dup_entry3 = NULL;
  char dup_entry_keystr3[] = "some_file_path_str4";
  struct version_num_entry* vn_entry = NULL;

  SUBCASE("duplicate one created node") {
    entry = retrieve_latest_versioned_node(graph, node_entry_label, FILE_NODE);
    dup_entry = duplicate_node_entry(graph, entry);

    REQUIRE(dup_entry);
    CHECK(strncmp(dup_entry->label, node_entry_label, strlen(node_entry_label)) == 0);
    CHECK(strlen(dup_entry->label) == strlen(node_entry_label));
    CHECK(strncmp(dup_entry->keystr, dup_entry_keystr, strlen(dup_entry_keystr)) == 0);
    CHECK(strlen(dup_entry->keystr) == strlen(dup_entry_keystr));
    CHECK(dup_entry->version_num == 2);
    CHECK(dup_entry->mark == UNMARKED);
    CHECK(entry->modflag == UNMODIFIED);
    CHECK(dup_entry->ntype == FILE_NODE);
    CHECK(slink_count(dup_entry->edges) == 0);

    vn_entry = get_version_num_entry(graph, node_entry_label);

    REQUIRE(vn_entry);
    CHECK(vn_entry->version_num == 2);
  }

  SUBCASE("duplicate one node three times from various versions") {
    entry = retrieve_latest_versioned_node(graph, node_entry_label, FILE_NODE);
    entry2 = duplicate_node_entry(graph, entry);
    duplicate_node_entry(graph, entry2);
    dup_entry3 = duplicate_node_entry(graph, entry);

    REQUIRE(dup_entry3);
    CHECK(strncmp(dup_entry3->label, node_entry_label, strlen(node_entry_label)) == 0);
    CHECK(strlen(dup_entry3->label) == strlen(node_entry_label));
    CHECK(strncmp(dup_entry3->keystr, dup_entry_keystr3, strlen(dup_entry_keystr3)) == 0);
    CHECK(strlen(dup_entry3->keystr) == strlen(dup_entry_keystr3));
    CHECK(dup_entry3->version_num == 4);
    CHECK(dup_entry3->mark == UNMARKED);
    CHECK(entry->modflag == UNMODIFIED);
    CHECK(dup_entry3->ntype == FILE_NODE);
    CHECK(slink_count(dup_entry3->edges) == 0);

    vn_entry = get_version_num_entry(graph, node_entry_label);

    REQUIRE(vn_entry);
    CHECK(vn_entry->version_num == 4);
  }

  clear_graph(&graph);

}

TEST_CASE("link_nodes_with_edge") {

  struct versioned_prov_graph* graph = create_new_graph();

  struct node_entry* node1 = NULL;
  char node_keystr1[] = "some_file_path_str1";
  char node_label1[] = "some_file_path_str";

  struct node_entry* node2 = NULL;
  char node_keystr2[] = "some_other_file_path_str1";
  char node_label2[] = "some_other_file_path_str";

  struct node_entry* node3 = NULL;
  char node_keystr3[] = "some_3other_file_path_str1";
  char node_label3[] = "some_3other_file_path_str";

  struct edge_entry* edge1to2 = NULL;
  struct edge_entry* edge1to3 = NULL;
  struct edge_entry* edge2to1 = NULL;
  struct edge_entry* edge2to3 = NULL;
  struct edge_entry* edge3to1 = NULL;
  struct edge_entry* edge3to2 = NULL;

  SUBCASE("link two nodes with an edge") {
    node1 = retrieve_latest_versioned_node(graph, node_label1, FILE_NODE);
    node2 = retrieve_latest_versioned_node(graph, node_label2, FILE_NODE);
    edge1to2 = link_nodes_with_edge(graph, node1, node2, INACTIVE);

    REQUIRE(edge1to2);
    CHECK(strncmp(edge1to2->node1_keystr, node_keystr1, strlen(node_keystr1)) == 0);
    CHECK(strlen(edge1to2->node1_keystr) == strlen(node_keystr1));
    CHECK(strncmp(edge1to2->node2_keystr, node_keystr2, strlen(node_keystr2)) == 0);
    CHECK(strlen(edge1to2->node2_keystr) == strlen(node_keystr2));
    CHECK(edge1to2->edge_label == INACTIVE);

    CHECK(slink_find_by_edge_keystr(node1->edges, edge1to2->keystr) == edge1to2);
    CHECK(slink_find_by_edge_keystr(node2->edges, edge1to2->keystr) == edge1to2);
  }

  SUBCASE("make \"connected\" graph of three nodes and six edges") {
    node1 = retrieve_latest_versioned_node(graph, node_label1, FILE_NODE);
    node2 = retrieve_latest_versioned_node(graph, node_label2, FILE_NODE);
    node3 = retrieve_latest_versioned_node(graph, node_label3, FILE_NODE);
    edge1to2 = link_nodes_with_edge(graph, node1, node2, ACTIVE);
    edge1to3 = link_nodes_with_edge(graph, node1, node3, INACTIVE);
    edge2to1 = link_nodes_with_edge(graph, node2, node1, ACTIVE);
    edge2to3 = link_nodes_with_edge(graph, node2, node3, INACTIVE);
    edge3to1 = link_nodes_with_edge(graph, node3, node1, ACTIVE);
    edge3to2 = link_nodes_with_edge(graph, node3, node2, INACTIVE);

    REQUIRE(edge1to2);
    CHECK(strncmp(edge1to2->node1_keystr, node_keystr1, strlen(node_keystr1)) == 0);
    CHECK(strlen(edge1to2->node1_keystr) == strlen(node_keystr1));
    CHECK(strncmp(edge1to2->node2_keystr, node_keystr2, strlen(node_keystr2)) == 0);
    CHECK(strlen(edge1to2->node2_keystr) == strlen(node_keystr2));
    CHECK(edge1to2->edge_label == ACTIVE);

    REQUIRE(edge1to3);
    CHECK(strncmp(edge1to3->node1_keystr, node_keystr1, strlen(node_keystr1)) == 0);
    CHECK(strlen(edge1to3->node1_keystr) == strlen(node_keystr1));
    CHECK(strncmp(edge1to3->node2_keystr, node_keystr3, strlen(node_keystr3)) == 0);
    CHECK(strlen(edge1to3->node2_keystr) == strlen(node_keystr3));
    CHECK(edge1to3->edge_label == INACTIVE);

    REQUIRE(edge2to1);
    CHECK(strncmp(edge2to1->node1_keystr, node_keystr2, strlen(node_keystr2)) == 0);
    CHECK(strlen(edge2to1->node1_keystr) == strlen(node_keystr2));
    CHECK(strncmp(edge2to1->node2_keystr, node_keystr1, strlen(node_keystr1)) == 0);
    CHECK(strlen(edge2to1->node2_keystr) == strlen(node_keystr1));
    CHECK(edge2to1->edge_label == ACTIVE);

    REQUIRE(edge2to3);
    CHECK(strncmp(edge2to3->node1_keystr, node_keystr2, strlen(node_keystr2)) == 0);
    CHECK(strlen(edge2to3->node1_keystr) == strlen(node_keystr2));
    CHECK(strncmp(edge2to3->node2_keystr, node_keystr3, strlen(node_keystr3)) == 0);
    CHECK(strlen(edge2to3->node2_keystr) == strlen(node_keystr3));
    CHECK(edge2to3->edge_label == INACTIVE);

    REQUIRE(edge3to1);
    CHECK(strncmp(edge3to1->node1_keystr, node_keystr3, strlen(node_keystr3)) == 0);
    CHECK(strlen(edge3to1->node1_keystr) == strlen(node_keystr3));
    CHECK(strncmp(edge3to1->node2_keystr, node_keystr1, strlen(node_keystr1)) == 0);
    CHECK(strlen(edge3to1->node2_keystr) == strlen(node_keystr1));
    CHECK(edge3to1->edge_label == ACTIVE);

    REQUIRE(edge3to2);
    CHECK(strncmp(edge3to2->node1_keystr, node_keystr3, strlen(node_keystr3)) == 0);
    CHECK(strlen(edge3to2->node1_keystr) == strlen(node_keystr3));
    CHECK(strncmp(edge3to2->node2_keystr, node_keystr2, strlen(node_keystr2)) == 0);
    CHECK(strlen(edge3to2->node2_keystr) == strlen(node_keystr2));
    CHECK(edge3to2->edge_label == INACTIVE);

    CHECK(slink_find_by_edge_keystr(node1->edges, edge1to2->keystr) == edge1to2);
    CHECK(slink_find_by_edge_keystr(node1->edges, edge2to1->keystr) == edge2to1);
    CHECK(slink_find_by_edge_keystr(node1->edges, edge1to3->keystr) == edge1to3);
    CHECK(slink_find_by_edge_keystr(node1->edges, edge3to1->keystr) == edge3to1);
    CHECK(slink_find_by_edge_keystr(node2->edges, edge2to1->keystr) == edge2to1);
    CHECK(slink_find_by_edge_keystr(node2->edges, edge1to2->keystr) == edge1to2);
    CHECK(slink_find_by_edge_keystr(node2->edges, edge2to3->keystr) == edge2to3);
    CHECK(slink_find_by_edge_keystr(node2->edges, edge3to2->keystr) == edge3to2);
    CHECK(slink_find_by_edge_keystr(node3->edges, edge3to1->keystr) == edge3to1);
    CHECK(slink_find_by_edge_keystr(node3->edges, edge1to3->keystr) == edge1to3);
    CHECK(slink_find_by_edge_keystr(node3->edges, edge3to2->keystr) == edge3to2);
    CHECK(slink_find_by_edge_keystr(node3->edges, edge2to3->keystr) == edge2to3);
  }

  clear_graph(&graph);

}

TEST_CASE("collect_nodes_connected_by_target_edges") {

  struct versioned_prov_graph* graph = create_new_graph();

  struct node_entry* node1 = NULL;
  char node_label1[] = "some_file_path_str";

  struct node_entry* node2 = NULL;
  char node_label2[] = "some_2other_file_path_str";

  struct node_entry* node3 = NULL;
  char node_label3[] = "some_3other_file_path_str";

  struct node_entry* node4 = NULL;
  char node_label4[] = "some_4other_file_path_str";

  struct node_entry* node5 = NULL;
  char node_label5[] = "some_5other_file_path_str";

  struct node_entry* node6 = NULL;
  char node_label6[] = "some_6other_file_path_str";

  struct node_entry* node7 = NULL;
  char node_label7[] = "some_7other_file_path_str";

  struct node_entry* node8 = NULL;
  char node_label8[] = "some_8other_file_path_str";

  struct node_entry* node9 = NULL;
  char node_label9[] = "some_9other_file_path_str";

  struct node_entry* collected_nodes_table = NULL;
  struct node_entry* found_node = NULL;

  SUBCASE("single unconnected node") {
    node1 = retrieve_latest_versioned_node(graph, node_label1, FILE_NODE);
    node1->mark = MARKED;

    collected_nodes_table = collect_nodes_connected_by_target_edges(graph, node1, MARKED, ACTIVE, OUTBOUND);

    CHECK(HASH_CNT(hh_collected, collected_nodes_table) == 1);

    found_node = NULL;
    HASH_FIND(hh_collected, collected_nodes_table, node1->keystr, strlen(node1->keystr), found_node);

    CHECK(found_node == node1);

    collected_nodes_table = NULL;
    collected_nodes_table = collect_nodes_connected_by_target_edges(graph, node1, UNMARKED, ACTIVE, INBOUND);

    CHECK(HASH_CNT(hh_collected, collected_nodes_table) == 0);
  }

  SUBCASE("node connected to marked/unmarked inbound/outbound nodes") {
    node1 = retrieve_latest_versioned_node(graph, node_label1, FILE_NODE);
    node2 = retrieve_latest_versioned_node(graph, node_label2, FILE_NODE);
    node2->mark = MARKED;
    node3 = retrieve_latest_versioned_node(graph, node_label3, FILE_NODE);
    node3->mark = MARKED;
    node4 = retrieve_latest_versioned_node(graph, node_label4, FILE_NODE);
    node5 = retrieve_latest_versioned_node(graph, node_label5, FILE_NODE);
    link_nodes_with_edge(graph, node1, node2, INACTIVE);
    link_nodes_with_edge(graph, node1, node3, ACTIVE);
    link_nodes_with_edge(graph, node4, node1, INACTIVE);
    link_nodes_with_edge(graph, node5, node1, ACTIVE);

    collected_nodes_table = collect_nodes_connected_by_target_edges(graph, node1, MARKED, ACTIVE, OUTBOUND);

    CHECK(HASH_CNT(hh_collected, collected_nodes_table) == 1);

    found_node = NULL;
    HASH_FIND(hh_collected, collected_nodes_table, node3->keystr, strlen(node3->keystr), found_node);

    CHECK(found_node == node3);
  }

  SUBCASE("\"connected\" graph with three nodes") {
    node1 = retrieve_latest_versioned_node(graph, node_label1, FILE_NODE);
    node1->mark = MARKED;
    node2 = retrieve_latest_versioned_node(graph, node_label2, FILE_NODE);
    node2->mark = MARKED;
    node3 = retrieve_latest_versioned_node(graph, node_label3, FILE_NODE);
    node3->mark = MARKED;
    link_nodes_with_edge(graph, node1, node2, ACTIVE);
    link_nodes_with_edge(graph, node2, node3, ACTIVE);
    link_nodes_with_edge(graph, node3, node1, ACTIVE);
    link_nodes_with_edge(graph, node1, node3, ACTIVE);
    link_nodes_with_edge(graph, node3, node2, ACTIVE);
    link_nodes_with_edge(graph, node2, node1, ACTIVE);

    collected_nodes_table = collect_nodes_connected_by_target_edges(graph, node1, MARKED, ACTIVE, OUTBOUND);

    CHECK(HASH_CNT(hh_collected, collected_nodes_table) == 3);

    found_node = NULL;
    HASH_FIND(hh_collected, collected_nodes_table, node1->keystr, strlen(node1->keystr), found_node);

    CHECK(found_node == node1);

    found_node = NULL;
    HASH_FIND(hh_collected, collected_nodes_table, node2->keystr, strlen(node2->keystr), found_node);

    CHECK(found_node == node2);

    found_node = NULL;
    HASH_FIND(hh_collected, collected_nodes_table, node3->keystr, strlen(node3->keystr), found_node);

    CHECK(found_node == node3);
  }

  SUBCASE("node with many indirect connections") {
    node1 = retrieve_latest_versioned_node(graph, node_label1, FILE_NODE);
    node1->mark = MARKED;
    node2 = retrieve_latest_versioned_node(graph, node_label2, FILE_NODE);
    node2->mark = MARKED;
    node3 = retrieve_latest_versioned_node(graph, node_label3, FILE_NODE);
    node3->mark = MARKED;
    node4 = retrieve_latest_versioned_node(graph, node_label4, FILE_NODE);
    node4->mark = MARKED;
    node5 = retrieve_latest_versioned_node(graph, node_label5, FILE_NODE);
    node5->mark = MARKED;
    node6 = retrieve_latest_versioned_node(graph, node_label6, FILE_NODE);
    node6->mark = MARKED;
    node7 = retrieve_latest_versioned_node(graph, node_label7, FILE_NODE);
    node7->mark = MARKED;
    node8 = retrieve_latest_versioned_node(graph, node_label8, FILE_NODE);
    node8->mark = MARKED;
    node9 = retrieve_latest_versioned_node(graph, node_label9, FILE_NODE);
    node9->mark = MARKED;
    link_nodes_with_edge(graph, node1, node2, INACTIVE);
    link_nodes_with_edge(graph, node1, node3, ACTIVE);
    link_nodes_with_edge(graph, node6, node2, ACTIVE);
    link_nodes_with_edge(graph, node2, node7, ACTIVE);
    link_nodes_with_edge(graph, node4, node3, ACTIVE);
    link_nodes_with_edge(graph, node3, node5, ACTIVE);
    link_nodes_with_edge(graph, node4, node8, ACTIVE);
    link_nodes_with_edge(graph, node5, node7, ACTIVE);
    link_nodes_with_edge(graph, node9, node5, ACTIVE);
    link_nodes_with_edge(graph, node5, node1, ACTIVE);

    collected_nodes_table = collect_nodes_connected_by_target_edges(graph, node1, MARKED, ACTIVE, OUTBOUND);

    CHECK(HASH_CNT(hh_collected, collected_nodes_table) == 4);

    found_node = NULL;
    HASH_FIND(hh_collected, collected_nodes_table, node1->keystr, strlen(node1->keystr), found_node);

    CHECK(found_node == node1);

    found_node = NULL;
    HASH_FIND(hh_collected, collected_nodes_table, node3->keystr, strlen(node3->keystr), found_node);

    CHECK(found_node == node3);

    found_node = NULL;
    HASH_FIND(hh_collected, collected_nodes_table, node5->keystr, strlen(node5->keystr), found_node);

    CHECK(found_node == node5);

    found_node = NULL;
    HASH_FIND(hh_collected, collected_nodes_table, node7->keystr, strlen(node7->keystr), found_node);

    CHECK(found_node == node7);
  }

  HASH_CLEAR(hh_collected, collected_nodes_table);
  clear_graph(&graph);

}

TEST_CASE("connect") {

  struct versioned_prov_graph* graph = create_new_graph();

  struct node_entry* node1 = NULL;
  struct node_entry* node1dup = NULL;
  char node_label1[] = "some_file_path_str";

  struct node_entry* node2 = NULL;
  struct node_entry* node2dup = NULL;
  char node_label2[] = "some_2other_file_path_str";

  struct node_entry* node3 = NULL;
  struct node_entry* node3dup = NULL;
  char node_label3[] = "some_3other_file_path_str";

  struct edge_entry* edge1to2 = NULL;
  struct edge_entry* edge1to1dup = NULL;
  struct edge_entry* edge1to2dup = NULL;
  struct edge_entry* edge1dupto2dup = NULL;
  struct edge_entry* edge1to3 = NULL;
  struct edge_entry* edge1dupto3dup = NULL;
  struct edge_entry* edge2to1 = NULL;
  struct edge_entry* edge2to2dup = NULL;
  struct edge_entry* edge2dupto1dup = NULL;
  struct edge_entry* edge2to3 = NULL;
  struct edge_entry* edge2dupto3 = NULL;
  struct edge_entry* edge2dupto3dup = NULL;
  struct edge_entry* edge3to1 = NULL;
  struct edge_entry* edge3to2 = NULL;
  struct edge_entry* edge3to3dup = NULL;
  struct edge_entry* edge3dupto1dup = NULL;
  struct edge_entry* edge3dupto2dup = NULL;

  struct node_entry* collected_nodes_table = NULL;
  struct node_entry* found_node = NULL;

  SUBCASE("one unmarked node connects to one unmarked node") {
    node1 = retrieve_latest_versioned_node(graph, node_label1, FILE_NODE);
    node2 = retrieve_latest_versioned_node(graph, node_label2, FILE_NODE);

    connect(graph, node1, node2);

    CHECK(HASH_CNT(hh, graph->node_table) == 2);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, node1->keystr, strlen(node1->keystr), found_node);

    CHECK(found_node == node1);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, node2->keystr, strlen(node2->keystr), found_node);

    CHECK(found_node == node2);

    CHECK(HASH_CNT(hh, graph->edge_table) == 1);

    edge1to2 = find_edge_entry(graph, node1, node2);

    REQUIRE(edge1to2);
    CHECK(edge1to2->edge_label == ACTIVE);
  }

  SUBCASE("one unmarked node connects to one marked node") {
    node1 = retrieve_latest_versioned_node(graph, node_label1, FILE_NODE);
    node2 = retrieve_latest_versioned_node(graph, node_label2, FILE_NODE);
    node2->mark = MARKED;

    connect(graph, node1, node2);

    CHECK(HASH_CNT(hh, graph->node_table) == 3);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, node1->keystr, strlen(node1->keystr), found_node);

    CHECK(found_node == node1);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, node2->keystr, strlen(node2->keystr), found_node);

    CHECK(found_node == node2);

    node2dup = retrieve_latest_versioned_node(graph, node_label2, FILE_NODE);

    REQUIRE(node2dup);
    CHECK(node2dup->version_num == 2);

    CHECK(HASH_CNT(hh, graph->edge_table) == 2);

    edge1to2dup = find_edge_entry(graph, node1, node2dup);

    REQUIRE(edge1to2dup);
    CHECK(edge1to2dup->edge_label == ACTIVE);

    edge2to2dup = find_edge_entry(graph, node2, node2dup);

    REQUIRE(edge2to2dup);
    CHECK(edge2to2dup->edge_label == INACTIVE);
  }

  SUBCASE("marked A connects to marked B (B already has act edge to A)") {
    node1 = retrieve_latest_versioned_node(graph, node_label1, FILE_NODE);
    node1->mark = MARKED;
    node2 = retrieve_latest_versioned_node(graph, node_label2, FILE_NODE);
    node2->mark = MARKED;
    edge2to1 = link_nodes_with_edge(graph, node2, node1, ACTIVE);

    connect(graph, node1, node2);

    CHECK(HASH_CNT(hh, graph->node_table) == 4);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, node1->keystr, strlen(node1->keystr), found_node);

    CHECK(found_node == node1);

    node1dup = retrieve_latest_versioned_node(graph, node_label1, FILE_NODE);

    REQUIRE(node1dup);
    CHECK(node1dup->version_num == 2);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, node2->keystr, strlen(node2->keystr), found_node);

    CHECK(found_node == node2);

    node2dup = retrieve_latest_versioned_node(graph, node_label2, FILE_NODE);

    REQUIRE(node2dup);
    CHECK(node2dup->version_num == 2);

    CHECK(HASH_CNT(hh, graph->edge_table) == 5);

    edge2to1 = find_edge_entry(graph, node2, node1);

    REQUIRE(edge2to1);
    CHECK(edge2to1->edge_label == INACTIVE);

    edge1to1dup = find_edge_entry(graph, node1, node1dup);

    REQUIRE(edge1to1dup);
    CHECK(edge1to1dup->edge_label == INACTIVE);

    edge2to2dup = find_edge_entry(graph, node2, node2dup);

    REQUIRE(edge2to2dup);
    CHECK(edge2to2dup->edge_label == INACTIVE);

    edge1dupto2dup = find_edge_entry(graph, node1dup, node2dup);

    REQUIRE(edge1dupto2dup);
    CHECK(edge1dupto2dup->edge_label == ACTIVE);

    edge2dupto1dup = find_edge_entry(graph, node2dup, node1dup);

    REQUIRE(edge2dupto1dup);
    CHECK(edge2dupto1dup->edge_label == ACTIVE);
  }

  SUBCASE("A connects to marked B (already have A==>B==>C, both active)") {
    node1 = retrieve_latest_versioned_node(graph, node_label1, FILE_NODE);
    node2 = retrieve_latest_versioned_node(graph, node_label2, FILE_NODE);
    node2->mark = MARKED;
    node3 = retrieve_latest_versioned_node(graph, node_label3, FILE_NODE);
    edge1to2 = link_nodes_with_edge(graph, node1, node2, ACTIVE);
    edge2to3 = link_nodes_with_edge(graph, node2, node3, ACTIVE);

    connect(graph, node1, node2);

    CHECK(HASH_CNT(hh, graph->node_table) == 4);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, node1->keystr, strlen(node1->keystr), found_node);

    CHECK(found_node == node1);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, node2->keystr, strlen(node2->keystr), found_node);

    CHECK(found_node == node2);

    node2dup = retrieve_latest_versioned_node(graph, node_label2, FILE_NODE);

    REQUIRE(node2dup);
    CHECK(node2dup->version_num == 2);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, node3->keystr, strlen(node3->keystr), found_node);

    CHECK(found_node == node3);

    CHECK(HASH_CNT(hh, graph->edge_table) == 5);

    edge1to2 = find_edge_entry(graph, node1, node2);

    REQUIRE(edge1to2);
    CHECK(edge1to2->edge_label == INACTIVE);

    edge1to2dup = find_edge_entry(graph, node1, node2dup);

    REQUIRE(edge1to2dup);
    CHECK(edge1to2dup->edge_label == ACTIVE);

    edge2to2dup = find_edge_entry(graph, node2, node2dup);

    REQUIRE(edge2to2dup);
    CHECK(edge2to2dup->edge_label == INACTIVE);

    edge2to3 = find_edge_entry(graph, node2, node3);

    REQUIRE(edge2to3);
    CHECK(edge2to3->edge_label == INACTIVE);

    edge2dupto3 = find_edge_entry(graph, node2dup, node3);

    REQUIRE(edge2dupto3);
    CHECK(edge2dupto3->edge_label == ACTIVE);
  }

  SUBCASE("A connects to B (have fully connected/marked/active A, B, C)") {
    node1 = retrieve_latest_versioned_node(graph, node_label1, FILE_NODE);
    node1->mark = MARKED;
    node2 = retrieve_latest_versioned_node(graph, node_label2, FILE_NODE);
    node2->mark = MARKED;
    node3 = retrieve_latest_versioned_node(graph, node_label3, FILE_NODE);
    node3->mark = MARKED;
    edge1to2 = link_nodes_with_edge(graph, node1, node2, ACTIVE);
    edge2to1 = link_nodes_with_edge(graph, node2, node1, ACTIVE);
    edge2to3 = link_nodes_with_edge(graph, node2, node3, ACTIVE);
    edge3to2 = link_nodes_with_edge(graph, node3, node2, ACTIVE);
    edge3to1 = link_nodes_with_edge(graph, node3, node1, ACTIVE);
    edge1to3 = link_nodes_with_edge(graph, node1, node3, ACTIVE);

    connect(graph, node1, node2);

    CHECK(HASH_CNT(hh, graph->node_table) == 6);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, node1->keystr, strlen(node1->keystr), found_node);

    CHECK(found_node == node1);

    node1dup = retrieve_latest_versioned_node(graph, node_label1, FILE_NODE);

    REQUIRE(node1dup);
    CHECK(node1dup->version_num == 2);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, node2->keystr, strlen(node2->keystr), found_node);

    CHECK(found_node == node2);

    node2dup = retrieve_latest_versioned_node(graph, node_label2, FILE_NODE);

    REQUIRE(node2dup);
    CHECK(node2dup->version_num == 2);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, node3->keystr, strlen(node3->keystr), found_node);

    CHECK(found_node == node3);

    node3dup = retrieve_latest_versioned_node(graph, node_label3, FILE_NODE);

    REQUIRE(node3dup);
    CHECK(node3dup->version_num == 2);

    CHECK(HASH_CNT(hh, graph->edge_table) == 15);

    edge1to2 = find_edge_entry(graph, node1, node2);

    REQUIRE(edge1to2);
    CHECK(edge1to2->edge_label == INACTIVE);

    edge1to3 = find_edge_entry(graph, node1, node3);

    REQUIRE(edge1to3);
    CHECK(edge1to3->edge_label == INACTIVE);

    edge1to1dup = find_edge_entry(graph, node1, node1dup);

    REQUIRE(edge1to1dup);
    CHECK(edge1to1dup->edge_label == INACTIVE);

    edge1dupto2dup = find_edge_entry(graph, node1dup, node2dup);

    REQUIRE(edge1dupto2dup);
    CHECK(edge1dupto2dup->edge_label == ACTIVE);

    edge1dupto3dup = find_edge_entry(graph, node1dup, node3dup);

    REQUIRE(edge1dupto3dup);
    CHECK(edge1dupto3dup->edge_label == ACTIVE);

    edge2to1 = find_edge_entry(graph, node2, node1);

    REQUIRE(edge2to1);
    CHECK(edge2to1->edge_label == INACTIVE);

    edge2to3 = find_edge_entry(graph, node2, node3);

    REQUIRE(edge2to3);
    CHECK(edge2to3->edge_label == INACTIVE);

    edge2to2dup = find_edge_entry(graph, node2, node2dup);

    REQUIRE(edge2to2dup);
    CHECK(edge2to2dup->edge_label == INACTIVE);

    edge2dupto1dup = find_edge_entry(graph, node2dup, node1dup);

    REQUIRE(edge2dupto1dup);
    CHECK(edge2dupto1dup->edge_label == ACTIVE);

    edge2dupto3dup = find_edge_entry(graph, node2dup, node3dup);

    REQUIRE(edge2dupto3dup);
    CHECK(edge2dupto3dup->edge_label == ACTIVE);

    edge3to1 = find_edge_entry(graph, node3, node1);

    REQUIRE(edge3to1);
    CHECK(edge3to1->edge_label == INACTIVE);

    edge3to2 = find_edge_entry(graph, node3, node2);

    REQUIRE(edge3to2);
    CHECK(edge3to2->edge_label == INACTIVE);

    edge3to3dup = find_edge_entry(graph, node3, node3dup);

    REQUIRE(edge3to3dup);
    CHECK(edge3to3dup->edge_label == INACTIVE);

    edge3dupto1dup = find_edge_entry(graph, node3dup, node1dup);

    REQUIRE(edge3dupto1dup);
    CHECK(edge3dupto1dup->edge_label == ACTIVE);

    edge3dupto2dup = find_edge_entry(graph, node3dup, node2dup);

    REQUIRE(edge3dupto2dup);
    CHECK(edge3dupto2dup->edge_label == ACTIVE);
  }

  HASH_CLEAR(hh_collected, collected_nodes_table);
  clear_graph(&graph);

}

TEST_CASE("disconnect") {

  struct versioned_prov_graph* graph = create_new_graph();
  VersionGraphAction act;

  struct node_entry* node1 = NULL;
  char node_label1[] = "some_file_path_str";

  struct node_entry* node2 = NULL;
  char node_label2[] = "some_2other_file_path_str";

  struct edge_entry* edge1to2 = NULL;

  struct node_entry* collected_nodes_table = NULL;
  struct node_entry* found_node = NULL;

  SUBCASE("A disconnects from B (no existing edge between A and B)") {
    node1 = retrieve_latest_versioned_node(graph, node_label1, FILE_NODE);
    node2 = retrieve_latest_versioned_node(graph, node_label2, FILE_NODE);

    act = disconnect(graph, node1, node2);

    CHECK(act == ERR_NO_ACTIVE_EDGE_BETWEEN_NODES);
    CHECK(HASH_CNT(hh, graph->node_table) == 2);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, node1->keystr, strlen(node1->keystr), found_node);

    CHECK(found_node == node1);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, node2->keystr, strlen(node2->keystr), found_node);

    CHECK(found_node == node2);

    CHECK(HASH_CNT(hh, graph->edge_table) == 0);
  }

  SUBCASE("A disconnects from B (existing inactive edge between A and B)") {
    node1 = retrieve_latest_versioned_node(graph, node_label1, FILE_NODE);
    node2 = retrieve_latest_versioned_node(graph, node_label2, FILE_NODE);
    edge1to2 = link_nodes_with_edge(graph, node1, node2, INACTIVE);

    act = disconnect(graph, node1, node2);

    CHECK(act == ERR_NO_ACTIVE_EDGE_BETWEEN_NODES);
    CHECK(HASH_CNT(hh, graph->node_table) == 2);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, node1->keystr, strlen(node1->keystr), found_node);

    CHECK(found_node == node1);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, node2->keystr, strlen(node2->keystr), found_node);

    CHECK(found_node == node2);

    CHECK(HASH_CNT(hh, graph->edge_table) == 1);

    edge1to2 = find_edge_entry(graph, node1, node2);

    REQUIRE(edge1to2);
    CHECK(edge1to2->edge_label == INACTIVE);
  }

  SUBCASE("A disconnects from B (existing active edge between A and B)") {
    node1 = retrieve_latest_versioned_node(graph, node_label1, FILE_NODE);
    node2 = retrieve_latest_versioned_node(graph, node_label2, FILE_NODE);
    edge1to2 = link_nodes_with_edge(graph, node1, node2, ACTIVE);

    act = disconnect(graph, node1, node2);

    CHECK(act == SUCCESS_NODES_DISCONNECTED);
    CHECK(HASH_CNT(hh, graph->node_table) == 2);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, node1->keystr, strlen(node1->keystr), found_node);

    REQUIRE(found_node == node1);
    CHECK(node1->mark == MARKED);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, node2->keystr, strlen(node2->keystr), found_node);

    REQUIRE(found_node == node2);
    CHECK(node2->mark == UNMARKED);

    CHECK(HASH_CNT(hh, graph->edge_table) == 1);

    edge1to2 = find_edge_entry(graph, node1, node2);

    REQUIRE(edge1to2);
    CHECK(edge1to2->edge_label == INACTIVE);
  }

  HASH_CLEAR(hh_collected, collected_nodes_table);
  clear_graph(&graph);

}

TEST_CASE("set_modflag_for_node_and_descendents") {

  struct versioned_prov_graph* graph = create_new_graph();

  char node_keystr1[] = "some_file_path_str1";
  char node_label1[] = "some_file_path_str";
  int node_version1 = 1;
  struct node_entry* node1 = add_node_entry(graph, node_label1, node_version1, FILE_NODE);
  REQUIRE(node1);

  char node_keystr2[] = "some_file_path_str2";
  char node_label2[] = "some_file_path_str";
  int node_version2 = 2;
  struct node_entry* node2 = add_node_entry(graph, node_label2, node_version2, FILE_NODE);
  REQUIRE(node2);

  char node_keystr3[] = "some_file_path_str3";
  char node_label3[] = "some_file_path_str";
  int node_version3 = 3;
  struct node_entry* node3 = add_node_entry(graph, node_label3, node_version3, FILE_NODE);
  REQUIRE(node3);

  struct edge_entry* edge1to2 = link_nodes_with_edge(graph, node1, node2, ACTIVE);
  struct edge_entry* edge2to3 = link_nodes_with_edge(graph, node2, node3, ACTIVE);

  SUBCASE("mod-flag last descendent in chain") {
    set_modflag_for_node_and_descendents(graph, node_keystr3, MODIFIED);

    CHECK(node1->modflag == UNMODIFIED);
    CHECK(node2->modflag == UNMODIFIED);
    CHECK(node3->modflag == MODIFIED);
  }

  clear_graph(&graph);

}

