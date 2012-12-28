#ifndef SERVER_H
#define SERVER_H

#define PORT "53530"  // the port users will be connecting to
#define BACKLOG 10	 // how many pending connections queue will hold

// list of client to server command
#define ORG_IP 1
#define NEW_IP 2

// list of server to client command
#define MAP_IP_SRC 11
#define MAP_ID_DST 12

#define DEBUG 1
#define dbprint(fmt, ...) \
  do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

#endif SERVER_H

