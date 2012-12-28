/*
** server.c -- a stream socket server
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

#include "server.h"
#include "hashtable.h"

struct serverParm {
	int             connection_desc;
};

//global        variables
int             count = 0;
hashtable_t     tblOrginalIp, tblNewIp;

void           *server_thread(void *parm_ptr);
void            handleMessage(const char *msgBuf, char *msgRet);
void            getProcesssIp(char *process_hdl, char *new_ip, char *msgRet);
void            saveProcesssIp(char *process_hdl, char *ip, char *msgRet);

void
sigchld_handler(int s)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

//get sockaddr, IPv4 or IPv6:
void           *
get_in_addr(struct sockaddr * sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *) sa)->sin_addr);
	}
	return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

int
main(void)
{
	int             sockfd, new_fd;
	//listen on sock_fd, new connection on new_fd

		struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	//connector address information
		socklen_t sin_size;
	struct sigaction sa;
	int             yes = 1;
	char            s[INET6_ADDRSTRLEN];
	int             rv;
	struct serverParm *parm_ptr;
	pthread_t       thread_id;

	//init
		hashtable_init(&tblOrginalIp, hashtable_hash_fnv, (hashtable_len_function) strlen);
	hashtable_init(&tblNewIp, hashtable_hash_fnv, (hashtable_len_function) strlen);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	//use my IP

		if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	//loop through all the results and bind to the first we can
		for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				     p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
			       sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}
	freeaddrinfo(servinfo);
	//all done with this structure

		if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
	sa.sa_handler = sigchld_handler;
	//reap all dead processes
		sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
	printf("server: waiting for connections...\n");

	while (1) {
		//main accept() loop
			sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *) & their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}
		inet_ntop(their_addr.ss_family,
			  get_in_addr((struct sockaddr *) & their_addr),
			  s, sizeof s);
		printf("server: got connection from %s\n", s);

		/* Create a thread to actually handle this client */
		parm_ptr = (struct serverParm *) malloc(sizeof(struct serverParm));
		parm_ptr->connection_desc = new_fd;
		if (pthread_create(&thread_id, NULL, server_thread, (void *) parm_ptr)
		    != 0) {
			perror("Thread create error");
			close(new_fd);
			close(sockfd);
			exit(1);
		}
	}

	return 0;
}

void           *
server_thread(void *parm_ptr)
{
#define PARMPTR ((struct serverParm *) parm_ptr)
	int             recievedMsgLen;
	char            msgBuf[1025];
	char            msgRet[1025];

	if (PARMPTR->connection_desc < 0) {
		printf("Accept failed\n");
		return (0);	/* Exit thread */
	}
	/* Receive messages from sender... */
	while ((recievedMsgLen = read(PARMPTR->connection_desc, msgBuf, sizeof(msgBuf) - 1)) > 0) {
		recievedMsgLen[msgBuf] = '\0';
		handleMessage(msgBuf, msgRet);
		if (write(PARMPTR->connection_desc, msgRet, sizeof(msgRet)) < 0) {
			perror("Server: write error");
			return (0);
		}
	}
	dbprint("%s", "server_thread return\n");
	close(PARMPTR->connection_desc);	/* Avoid descriptor leaks */
	free(PARMPTR);		/* And memory leaks */
	return (0);		/* Exit thread */
}

void
handleMessage(const char *msgBuf, char *msgRet)
{
	int             cmd = 0;
	char            param1[512], param2[512];
	bzero(param1, sizeof(param1));
	bzero(param2, sizeof(param2));
  bzero(msgRet, sizeof(msgRet));
	dbprint("handle buf: %s\n", msgBuf);
	sscanf(msgBuf, "%d\t%s\t%s\n", &cmd, param1, param2);

	switch (cmd) {
	case ORG_IP:
//params:	process handle(created and saved), orginal ip
			saveProcesssIp(param1, param2, msgRet);
		break;
	case NEW_IP:
//params:	process handle(read from settings), new ip
// return:		original ip
			getProcesssIp(param1, param2, msgRet);
		break;
	default:
		sprintf(msgRet, "NACK");
		break;
	}
}

void
saveProcesssIp(char *process_hdl, char *ip, char *msgRet)
{
	dbprint("save: %s -> %s\n", process_hdl, ip);
	//TODO - lock
		hashtable_set(&tblOrginalIp, process_hdl, ip);
	sprintf(msgRet, "ACK 0\n");
	//TODO - unlock
}

void
getProcesssIp(char *process_hdl, char *new_ip, char *msgRet)
{
	char           *ip;

	dbprint("lookup: %s with %s ", process_hdl, new_ip);
	//TODO - lock
		hashtable_set(&tblNewIp, process_hdl, new_ip);
	if (hashtable_get(&tblOrginalIp, process_hdl, (void **) &ip) == 1) {
		dbprint("and get %s\n", ip);
		sprintf(msgRet, "ACK %s\n", ip);
	} else {
		dbprint("%s", " not found\n");
		sprintf(msgRet, "NACK\n");
	}
	//TODO - unlock
}
