/*

CDEnet: Distributed Code, Data, and Environment packaging, network record and replay for Linux
Quan Pham

CDEnet is currently licensed under GPL v3:

  Copyright (c) 2010-2011 Quan Pham<quanpt@cs.uchicago.edu>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

*/

// headers copied from net.c

#include "config.h"
#include "defs.h"
#include "provenance.h"
#include "cdenet.h"
#include "../leveldb-1.14.0/include/leveldb/c.h"
#include "const.h"
#include "db.h"

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

// TODO: we probably don't need most of these #includes
#include <sys/select.h>
#include <sys/user.h> // a user told me that user.h should go after select.h to fix a compile error
#include <sys/time.h>
#include <string.h>
#include <utime.h>
//#include <sys/ptrace.h>
//#include <linux/ptrace.h>   /* For constants ORIG_EAX etc */
//#include <sys/syscall.h>   /* For constants SYS_write etc */
#include <linux/types.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

//#define _GNU_SOURCE // for vasprintf (now we include _GNU_SOURCE in Makefile)
#include <stdio.h>

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/mman.h>
#include <linux/ipc.h>
#include <linux/shm.h>
#include <sys/stat.h>
#include <sys/param.h>

#if defined(HAVE_SIN6_SCOPE_ID_LINUX)
#define in6_addr in6_addr_libc
#define ipv6_mreq ipv6_mreq_libc
#define sockaddr_in6 sockaddr_in6_libc
#endif

#include <netinet/in.h>
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifdef HAVE_NETINET_UDP_H
#include <netinet/udp.h>
#endif
#ifdef HAVE_NETINET_SCTP_H
#include <netinet/sctp.h>
#endif
#include <arpa/inet.h>
#include <net/if.h>
#if defined(LINUX)
#include <asm/types.h>
#if defined(__GLIBC__) && (__GLIBC__ >= 2) && (__GLIBC__ + __GLIBC_MINOR__ >= 3)
#  include <netipx/ipx.h>
#else
#  include <linux/ipx.h>
#endif
#endif /* LINUX */

#if defined (__GLIBC__) && (((__GLIBC__ < 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 1)) || defined(HAVE_SIN6_SCOPE_ID_LINUX))
#if defined(HAVE_LINUX_IN6_H)
#if defined(HAVE_SIN6_SCOPE_ID_LINUX)
#undef in6_addr
#undef ipv6_mreq
#undef sockaddr_in6
#define in6_addr in6_addr_kernel
#define ipv6_mreq ipv6_mreq_kernel
#define sockaddr_in6 sockaddr_in6_kernel
#endif
#include <linux/in6.h>
#if defined(HAVE_SIN6_SCOPE_ID_LINUX)
#undef in6_addr
#undef ipv6_mreq
#undef sockaddr_in6
#define in6_addr in6_addr_libc
#define ipv6_mreq ipv6_mreq_libc
#define sockaddr_in6 sockaddr_in6_kernel
#endif
#endif
#endif

#if defined(HAVE_SYS_UIO_H)
#include <sys/uio.h>
#endif

#if defined(HAVE_LINUX_ICMP_H)
#include <linux/icmp.h>
#endif

#ifndef PF_UNSPEC
#define PF_UNSPEC AF_UNSPEC
#endif

#if UNIXWARE >= 7
#define HAVE_SENDMSG    1    /* HACK - *FIXME* */
#endif

#ifdef LINUX
/* Under Linux these are enums so we can't test for them with ifdef. */
#define IPPROTO_EGP IPPROTO_EGP
#define IPPROTO_PUP IPPROTO_PUP
#define IPPROTO_IDP IPPROTO_IDP
#define IPPROTO_IGMP IPPROTO_IGMP
#define IPPROTO_RAW IPPROTO_RAW
#define IPPROTO_MAX IPPROTO_MAX
#endif

#if defined(AF_PACKET) /* from e.g. linux/if_packet.h */
static const struct xlat af_packet_types[] = {
#if defined(PACKET_HOST)
	{ PACKET_HOST,			"PACKET_HOST"		},
#endif
#if defined(PACKET_BROADCAST)
	{ PACKET_BROADCAST,		"PACKET_BROADCAST"	},
#endif
#if defined(PACKET_MULTICAST)
	{ PACKET_MULTICAST,		"PACKET_MULTICAST"	},
#endif
#if defined(PACKET_OTHERHOST)
	{ PACKET_OTHERHOST,		"PACKET_OTHERHOST"	},
#endif
#if defined(PACKET_OUTGOING)
	{ PACKET_OUTGOING,		"PACKET_OUTGOING"	},
#endif
#if defined(PACKET_LOOPBACK)
	{ PACKET_LOOPBACK,		"PACKET_LOOPBACK"	},
#endif
#if defined(PACKET_FASTROUTE)
	{ PACKET_FASTROUTE,		"PACKET_FASTROUTE"	},
#endif
	{ 0,				NULL			},
};
#endif /* defined(AF_PACKET) */
// end of copy from net.c

// adapted from systrace

enum LINUX_CALL_TYPES {
	LINUX64 = 0,
	LINUX32 = 1,
	LINUX_NUM_VERSIONS = 2
};

#ifdef X86_64
//~ static enum LINUX_CALL_TYPES
enum LINUX_CALL_TYPES
linux_call_type(long codesegment) 
{
	if (codesegment == 0x33)
		return (LINUX64);
	else if (codesegment == 0x23)
		return (LINUX32);
        else {
		fprintf(stderr, "%s:%d: unknown code segment %lx\n",
		    __FILE__, __LINE__, codesegment);
		assert(0);
	}
}
#endif

#ifdef X86_64
#define ISLINUX32(x)		(linux_call_type((x)->cs) == LINUX32)
#define SYSCALL_NUM(x)		(x)->orig_rax
#define SET_RETURN_CODE(x, v)	(x)->rax = (v)
#define RETURN_CODE(x)		(ISLINUX32(x) ? (long)(int)(x)->rax : (x)->rax)
#define ARGUMENT_0(x)		(ISLINUX32(x) ? (x)->rbx : (x)->rdi)
#define ARGUMENT_1(x)		(ISLINUX32(x) ? (x)->rcx : (x)->rsi)
#define ARGUMENT_2(x)		(ISLINUX32(x) ? (x)->rdx : (x)->rdx)
#define ARGUMENT_3(x)		(ISLINUX32(x) ? (x)->rsi : (x)->rcx)
#define ARGUMENT_4(x)		(ISLINUX32(x) ? (x)->rdi : (x)->r8)
#define ARGUMENT_5(x)		(ISLINUX32(x) ? (x)->rbp : (x)->r9)
#define SET_ARGUMENT_0(x, v)	if (ISLINUX32(x)) (x)->rbx = (v); else (x)->rdi = (v)
#define SET_ARGUMENT_1(x, v)	if (ISLINUX32(x)) (x)->rcx = (v); else (x)->rsi = (v)
#define SET_ARGUMENT_2(x, v)	if (ISLINUX32(x)) (x)->rdx = (v); else (x)->rdx = (v)
#define SET_ARGUMENT_3(x, v)	if (ISLINUX32(x)) (x)->rsi = (v); else (x)->rcx = (v)
#define SET_ARGUMENT_4(x, v)	if (ISLINUX32(x)) (x)->rdi = (v); else (x)->r8 = (v)
#define SET_ARGUMENT_5(x, v)	if (ISLINUX32(x)) (x)->rbp = (v); else (x)->r9 = (v)
#else
#define SYSCALL_NUM(x)		(x)->orig_eax
#define SET_RETURN_CODE(x, v)	(x)->eax = (v)
#define RETURN_CODE(x)		(x)->eax
#define ARGUMENT_0(x)		(x)->ebx
#define ARGUMENT_1(x)		(x)->ecx
#define ARGUMENT_2(x)		(x)->edx
#define ARGUMENT_3(x)		(x)->esi
#define ARGUMENT_4(x)		(x)->edi
#define ARGUMENT_5(x)		(x)->ebp
#define SET_ARGUMENT_0(x, v)	(x)->ebx = (v)
#define SET_ARGUMENT_1(x, v)	(x)->ecx = (v)
#define SET_ARGUMENT_2(x, v)	(x)->edx = (v)
#define SET_ARGUMENT_3(x, v)	(x)->esi = (v)
#define SET_ARGUMENT_4(x, v)	(x)->edi = (v)
#define SET_ARGUMENT_5(x, v)	(x)->ebp = (v)
#endif /* !X86_64 */

// variables from cde.c
extern int CDE_exec_mode;
extern int CDE_provenance_mode;
extern int CDE_verbose_mode;

// function from cde.c
extern void memcpy_to_child(int pid, char* dst_child, char* src, int size);
extern void vbprintf(const char *fmt, ...);

// global parameters
char CDE_nw_mode = 0; // 1 if we simulate all network sockets, 0 otherwise (-N)
char *DB_NAME = NULL;
char *PIDKEY = NULL; // pidkey of the starting pid for replay (not neccessary the db root)
extern char cde_pseudo_pkg_dir[MAXPATHLEN];
extern char* CDE_ROOT_NAME;

lvldb_t *netdb, *currdb;
char* netdb_root;

extern char* getMappedPid(char* pidkey);

// TODO: read from external file / socket on initialization
int N_SIN = 1;
struct sockaddr_in sin_key[1];
struct sockaddr_in sin_value[1];

// local function signatures
void CDEnet_begin_connect(struct tcb* tcp);
void CDEnet_end_connect(struct tcb* tcp);
void CDEnet_begin_bind(struct tcb *tcp);
void CDEnet_end_bind(struct tcb *tcp);
void CDEnet_begin_listen(struct tcb* tcp);
void CDEnet_end_listen(struct tcb* tcp);
void CDEnet_begin_accept(struct tcb* tcp);
void CDEnet_end_accept(struct tcb* tcp);
void CDEnet_begin_send(struct tcb* tcp);
void CDEnet_end_send(struct tcb* tcp);
void CDEnet_begin_recv(struct tcb* tcp);
void CDEnet_end_recv(struct tcb* tcp);

// delete these later
void CDEnet_begin_sendmsg(struct tcb* tcp);
void CDEnet_end_sendmsg(struct tcb* tcp);
void CDEnet_begin_recvmsg(struct tcb* tcp);
void CDEnet_end_recvmsg(struct tcb* tcp);


void CDEnet_sin_dict_load() {
  // TODO: implement by loading from a config file or from a server
  sin_key[0].sin_family = AF_INET;
  inet_aton("128.135.250.118", &(sin_key[0].sin_addr)); // gabri ip adress
  sin_value[0].sin_family = AF_INET;
  inet_aton("128.135.24.221", &(sin_value[0].sin_addr)); // convert to ankaa ip address
  // TODO: keep port mapping as well?
}

// return 1 if converted, 0 if keep intact
int CDEnet_convert_sin(struct sockaddr_in *sin) {
  // TODO: check against a dictionary, alter ip and port, and consider using inet_pton
  int i;
  for (i=0; i<N_SIN; i++) {
    if (sin_key[i].sin_addr.s_addr == sin->sin_addr.s_addr) {
      printf("Convert %s", inet_ntoa(sin->sin_addr));
      printf(" to %s\n", inet_ntoa(sin_value[i].sin_addr));
      sin->sin_addr.s_addr = sin_value[i].sin_addr.s_addr;
      // TODO: port mapping if implemented in sin_dict_load
      return 1; // success
    }
  }
  return 0;
}

typedef struct socketdata {
  unsigned short saf;
  unsigned int port;
  union ipdata {
    unsigned long ipv4;
    unsigned char ipv6[16];   /* IPv6 address */
  } ip;
} socketdata_t;

int getPort(union sockaddr_t *addrbuf) {
  if (addrbuf->sa.sa_family == AF_INET) {
    return ntohs(addrbuf->sin.sin_port);
  }
#ifdef HAVE_INET_NTOP
  if (addrbuf->sa.sa_family == AF_INET6) {
    return ntohs(addrbuf->sa6.sin6_port);
  }
#endif
  return -1;
}

int getsockinfo(struct tcb *tcp, char* addr, socketdata_t *psock) {
	union sockaddr_t *addrbuf = (union sockaddr_t*) addr;
	//union sockaddr_t *addrbuf;// = (union sockaddr_t*)addr;
	//~ char string_addr[100];

	if (addr == 0) {
		return -1;
	}

	//~ if (addrlen < 2 || addrlen > sizeof(addrbuf))
		//~ addrlen = sizeof(addrbuf);
//~ 
	//~ memset(&addrbuf, 0, sizeof(addrbuf));
	//~ if (umoven(tcp, addr, addrlen, addrbuf.pad) < 0) {
		//~ return -1;
	//~ }
	addrbuf->pad[sizeof(addrbuf->pad) - 1] = '\0';

	psock->saf = addrbuf->sa.sa_family;

	switch (addrbuf->sa.sa_family) {
	case AF_UNIX:
	  // AF_FILE is also a synonym for AF_UNIX
	  // these are file operations
		break;
	case AF_INET:
		//tprintf("sin_port=htons(%u), sin_addr=inet_addr(\"%s\")",
			//ntohs(addrbuf.sin.sin_port), inet_ntoa(addrbuf.sin.sin_addr));
		psock->port = ntohs(addrbuf->sin.sin_port);
		psock->ip.ipv4 = addrbuf->sin.sin_addr.s_addr;
		return 4;
		break;
#ifdef HAVE_INET_NTOP
	case AF_INET6:
		//~ inet_ntop(AF_INET6, &addrbuf->sa6.sin6_addr, string_addr, sizeof(string_addr));
		//tprintf("sin6_port=htons(%u), inet_pton(AF_INET6, \"%s\", &sin6_addr), sin6_flowinfo=%u",
		//		ntohs(addrbuf.sa6.sin6_port), string_addr,
		//		addrbuf.sa6.sin6_flowinfo);
		psock->port = ntohs(addrbuf->sa6.sin6_port);
		memcpy(&addrbuf->sa6.sin6_addr, &psock->ip.ipv6, 16);
		return 6;
		break;
#endif

  /* Quan - not handle AF_IPX AF_APACKET AF_NETLINK */
	/* AF_AX25 AF_APPLETALK AF_NETROM AF_BRIDGE AF_AAL5
	AF_X25 AF_ROSE etc. still need to be done */

	default:
		break;
	}
	return -1;
}

int isCurrCapturedSock(int sockfd) {
  return db_isCapturedSock(currdb, sockfd);
}

void printSockInfo(struct tcb* tcp, int op, \
    unsigned int d_port, unsigned long d_ipv4, int sk) {
  struct sockaddr_in localAddr;
  socklen_t len = sizeof(localAddr);
  
  localAddr.sin_port = 0;
  localAddr.sin_addr.s_addr = 0;

  if (op != SOCK_BIND && op != SOCK_ACCEPT) {
    if (getsockname(sk, (struct sockaddr*)&localAddr, &len)<0)
      printf("error: %s on %d\n", strerror(errno), sk);
  }
  print_newsock_prov(tcp, op, ntohs(localAddr.sin_port), \
      localAddr.sin_addr.s_addr, d_port, d_ipv4, sk);
}

// int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
void denySyscall(long pid) {
  struct user_regs_struct regs;
  EXITIF(ptrace(PTRACE_GETREGS, pid, NULL, &regs)<0);
  if (CDE_verbose_mode>=2) {
    vbprintf("[%ld-net] denySyscall %d\n", pid, SYSCALL_NUM(&regs));
  }
  SYSCALL_NUM(&regs) = 0xbadca11;
  EXITIF(ptrace(PTRACE_SETREGS, pid, NULL, &regs)<0);
}

// int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
//     on connection or binding succeeds, zero is returned; on error, -1 is returned.
void CDEnet_begin_bindconnect(struct tcb* tcp, int isConnect) {
  vb(1);
  if (CDE_nw_mode) {
    char addrbuf[KEYLEN];
    if (umoven(tcp, tcp->u_arg[1], tcp->u_arg[2], addrbuf) < 0) return;
    if (isConnect) {
      int port = getPort((void*)addrbuf);
      vbp(3, "Port %d\n", port);
      if (port == 53 || port == 22) return;
    }
    
    db_setCapturedSock(currdb, tcp->u_arg[0]);
    denySyscall(tcp->pid);
  }
}

void setBindConnectReturnValue(struct tcb* tcp) {
  int sockfd = tcp->u_arg[0];
  char *pidkey = db_read_pid_key(currdb, tcp->pid);
  char* prov_pid = getMappedPid(pidkey);	// convert this pid to corresponding prov_pid
  ull_t sockid = db_getConnectCounterInc(currdb, pidkey);
  int u_rval = db_getSockResult(netdb, prov_pid, sockid); // get the result of a connect call
  //~ vbp(3, "here\n");
  
  // return recorded result
  struct user_regs_struct regs;
  long pid = tcp->pid;
  EXITIF(ptrace(PTRACE_GETREGS, pid, NULL, &regs)<0);
  SET_RETURN_CODE(&regs, u_rval);
  vbp(3, "return %d\n", u_rval);
  if (u_rval < 0) {
    // set errno? TODO
  } else {
    db_setSockConnectId(currdb, pidkey, sockfd, sockid);	// map this sock number to given sock in tcp
    // TODO: keep the addrbuf.pad memory as well
  }
  EXITIF(ptrace(PTRACE_SETREGS, pid, NULL, &regs)<0);
  
  // init variables for this socket
  db_setupSockConnectCounter(currdb, pidkey, sockfd, sockid);
  
  free(prov_pid);
  free(pidkey);
}

void CDEnet_end_bindconnect(struct tcb* tcp, int isConnect) {
  int sockfd = tcp->u_arg[0];
  long addr = tcp->u_arg[1];
  int addrlen = tcp->u_arg[2];
  union sockaddr_t addrbuf;
  
  vb(2);

  if (addr == 0) return;

  if (addrlen < 2 || addrlen > sizeof(addrbuf)) addrlen = sizeof(addrbuf);

  if (umoven(tcp, addr, addrlen, addrbuf.pad) < 0) return;
  addrbuf.pad[sizeof(addrbuf.pad) - 1] = '\0';
  
  //~ printf("sock %d, family %d inet %d\n", sockfd, addrbuf.sa.sa_family, AF_INET);
  //~ if (CDE_provenance_mode && addrbuf.sa.sa_family == AF_INET) {
  if (CDE_provenance_mode) {
    int port = getPort((void*)addrbuf.pad);
    char buf[KEYLEN];
    bzero(buf, sizeof(buf));
    vbp(3, "Port %d return %ld\n", port, tcp->u_rval);
    if (port == 53 || port == 22) return;
    if (tcp->u_rval >= 0 && isConnect && addrbuf.sa.sa_family == AF_INET) {
      struct stat fdstat;
      char path[KEYLEN];
      sprintf(path, "/proc/%d/fd/%d", tcp->pid, sockfd);
      if (stat(path, &fdstat) >= 0) {
	char cmd[KEYLEN], sip[9], sport[5], dip[9], dport[5];
	ino_t inode;
	sprintf(cmd, "cat /proc/net/tcp | grep %ld | head -n 1", fdstat.st_ino);
	FILE *fd = popen(cmd, "r");
	if (fd != NULL) {
	  fscanf(fd, "%*d: %8s:%4s %8s:%4s %*2s %*8s:%*8s %*2s:%*8s %*8s %*d %*d %ld", 
	      sip, sport, dip, dport, &inode);
	  if (inode == fdstat.st_ino) {
	    sprintf(buf, "%s.%s.%s.%s", sip, sport, dip, dport);
	    vbp(3, "%d %d %s:%s %s:%s %ld %ld\n", tcp->pid, sockfd, sip, sport, dip, dport, inode, fdstat.st_ino);
	  }
	  pclose(fd);
	}
      }
    }
    print_connect_prov(tcp, sockfd, addrbuf.pad, tcp->u_arg[2], tcp->u_rval, buf);
  }
  if (CDE_nw_mode) { // return my own network socket connect result from netdb
    if (isConnect) if (!isCurrCapturedSock(sockfd)) return;
    setBindConnectReturnValue(tcp);
  }
}

int socket_data_handle(struct tcb* tcp, int action) {
  int sockfd = tcp->u_arg[0];
  if (CDE_provenance_mode) {
    long len = tcp->u_rval;
    char *buf = NULL;
    if (len >0) {
      buf = malloc(len);
      if (umoven(tcp, tcp->u_arg[1], len, buf) < 0) {
	free(buf);
	return -1;
      }
    }
    print_sock_action(tcp, sockfd, buf, tcp->u_arg[2], tcp->u_arg[3], len, action, NULL);
    if (buf != NULL) free(buf);
  }
  return 0;
}

/* receiving side
 * ssize_t recv(int sockfd, void *buf, size_t len, int flags);
 * size_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen);
 * ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);
 */

void CDEnet_begin_recv(struct tcb* tcp) {
  if (CDE_nw_mode && db_isCapturedSock(currdb, tcp->u_arg[0])) {
    vb(2);
    denySyscall(tcp->pid);
  }
}
void CDEnet_end_recv(struct tcb* tcp) {
  int sockfd = tcp->u_arg[0];
  if (CDE_provenance_mode && isProvCapturedSock(sockfd)) {
    vb(2);
    socket_data_handle(tcp, SOCK_RECV);
  }
  if (CDE_nw_mode && db_isCapturedSock(currdb, sockfd)) {
    vb(2);
    long pid = tcp->pid;
    char *pidkey, *sockid;
    db_get_pid_sock(currdb, pid, sockfd, &pidkey, &sockid);
    if (pidkey == NULL || sockid == NULL) {
      vbp(0, "error");
      return;
    }
    ull_t sendid = db_getPkgCounterInc(currdb, pidkey, sockid, SOCK_RECV);
    char* prov_pid = getMappedPid(pidkey);	// convert this pid to corresponding prov_pid
    ull_t u_rval;
    char *buff = db_getSendRecvResult(netdb, SOCK_RECV, prov_pid, sockid, sendid, &u_rval, NULL); // get recorded result
    if (buff == NULL && u_rval>0) {
      vbp(2, "buff == NULL && u_rval (%lld) > 0", u_rval);
      u_rval = 0;
    }
    struct user_regs_struct regs;
    EXITIF(ptrace(PTRACE_GETREGS, pid, NULL, &regs)<0);
    SET_RETURN_CODE(&regs, u_rval);
    EXITIF(ptrace(PTRACE_SETREGS, pid, NULL, &regs)<0);
    if (u_rval <= 0) {
      // set errno? TODO
    } else { // successed call, copy to buffer as well
      //~ printf("mem: %zd %d\n %s", u_rval, u_rval > 0, buff);
      memcpy_to_child(pid, (char*) tcp->u_arg[1], buff, u_rval);
    }
    
    if (buff != NULL) free(buff);
    free(prov_pid);
    free(sockid);
    free(pidkey);
  }
}

//~ struct iovec {                    /* Scatter/gather array items */
   //~ void  *iov_base;              /* Starting address */
   //~ size_t iov_len;               /* Number of bytes to transfer */
//~ };
//~ 
//~ struct msghdr {
   //~ void         *msg_name;       /* optional address */
   //~ socklen_t     msg_namelen;    /* size of address */
   //~ struct iovec *msg_iov;        /* scatter/gather array */
   //~ size_t        msg_iovlen;     /* # elements in msg_iov */
   //~ void         *msg_control;    /* ancillary data, see below */
   //~ size_t        msg_controllen; /* ancillary data buffer len */
   //~ int           msg_flags;      /* flags on received message */
//~ };
void CDEnet_begin_recvmsg(struct tcb* tcp) { //TODO
  if (CDE_nw_mode && db_isCapturedSock(currdb, tcp->u_arg[0])) {
    vb(2);
    denySyscall(tcp->pid);
  }
}
void CDEnet_end_recvmsg(struct tcb* tcp) {
  int sockfd = tcp->u_arg[0];
  struct msghdr mh;
  struct iovec *msg_iov = NULL;
  char memop_ok = 1;
  if (CDE_provenance_mode && isProvCapturedSock(sockfd)) {
    int len = tcp->u_rval;
    //~ char *msg_name = NULL, *msg_control = NULL;
    char *storage = NULL;
    
    vb(2);
    
    if (umoven(tcp, tcp->u_arg[1], sizeof(struct msghdr), (char*) &mh) < 0) return;
    if (len > 0) {
      storage = malloc(len);
      memop_ok &= storage != NULL;
    }
    //~ if (memop_ok && mh.msg_namelen > 0) {
      //~ msg_name = malloc(mh.msg_namelen);
      //~ memop_ok &= msg_name != NULL;
      //~ if (memop_ok)
	//~ memop_ok &= umoven(tcp, (long) mh.msg_name, mh.msg_namelen, msg_name) >= 0;
    //~ }
    if (memop_ok && mh.msg_iovlen > 0) {
      int memlen = mh.msg_iovlen * sizeof(struct iovec);
      msg_iov = malloc(memlen);
      memop_ok &= msg_iov != NULL;
      if (memop_ok)
        memop_ok &= umoven(tcp, (long) mh.msg_iov, memlen , (void*) msg_iov) >= 0;
    }
    //~ if (memop_ok && mh.msg_controllen > 0) {
      //~ msg_control = malloc(mh.msg_controllen);
      //~ memop_ok &= msg_control != NULL;
      //~ if (memop_ok)
	//~ memop_ok &= umoven(tcp, (long) mh.msg_control, mh.msg_controllen, msg_control) >= 0;
    //~ }
    if (memop_ok) {
      char *it = storage;
      long int read_len, i;
      for (i=0; i<mh.msg_iovlen && memop_ok; i++) {
        read_len = len + storage - it < msg_iov[i].iov_len ?
        len + storage - it : msg_iov[i].iov_len;
        memop_ok &= umoven(tcp, (long) msg_iov[i].iov_base, read_len, it) >= 0;
        if (memop_ok)
        it = it + read_len;
      }
      if (memop_ok) {
        print_sock_action(tcp, sockfd, storage, 0, tcp->u_arg[2], len, SOCK_RECVMSG, &mh);
        vbp(2, "recorded\n");
      }
    }
    freeifnn(storage); 
    //~ freeifnn(msg_control); 
    freeifnn(msg_iov); 
    //~ freeifnn(msg_name);
  }
  if (CDE_nw_mode && isCurrCapturedSock(sockfd)) {
    vb(2);
    long pid = tcp->pid;
    char *pidkey, *sockid;
    db_get_pid_sock(currdb, pid, sockfd, &pidkey, &sockid);
    if (pidkey == NULL || sockid == NULL) {
      vbp(0, "error");
      return;
    }
    ull_t sendid = db_getPkgCounterInc(currdb, pidkey, sockid, SOCK_RECV);
    char* prov_pid = getMappedPid(pidkey);	// convert this pid to corresponding prov_pid
    ull_t u_rval;
    struct msghdr ret_mh;
    char *buff = db_getSendRecvResult(netdb, SOCK_RECVMSG, prov_pid, sockid, sendid, &u_rval, &ret_mh); // get recorded result
    struct user_regs_struct regs;
    
    if (buff == NULL) {
      vbp(0, "recvmsg not captured!\n");
      //~ while (1) sleep(1);
      EXITIF(ptrace(PTRACE_GETREGS, pid, NULL, &regs)<0);
      SET_RETURN_CODE(&regs, -1);
      EXITIF(ptrace(PTRACE_SETREGS, pid, NULL, &regs)<0);
    
    } else {
    
      EXITIF(ptrace(PTRACE_GETREGS, pid, NULL, &regs)<0);
      SET_RETURN_CODE(&regs, u_rval);
      EXITIF(ptrace(PTRACE_SETREGS, pid, NULL, &regs)<0);
      if (u_rval <= 0) {
	// set errno? TODO
      } else { // successed call, copy to buffer as well
	
	// get the calling mh structure
	memop_ok &= umoven(tcp, tcp->u_arg[1], sizeof(struct msghdr), (char*) &mh) >= 0;
	if (memop_ok && mh.msg_iovlen > 0) {
	  int memlen = mh.msg_iovlen * sizeof(struct iovec);
	  msg_iov = malloc(memlen);
	  memop_ok &= msg_iov != NULL;
	  if (memop_ok)
	    memop_ok &= umoven(tcp, (long) mh.msg_iov, memlen , (void*) msg_iov) >= 0;
	}
	
	// and copy stuff back to that mh
	if (memop_ok) {
	  // data
	  char *it = buff;
	  long int read_len, i;
	  for (i=0; i<mh.msg_iovlen && memop_ok; i++) {
	    read_len = u_rval + buff - it < msg_iov[i].iov_len ?
		u_rval + buff - it : msg_iov[i].iov_len;
	    memcpy_to_child(pid, msg_iov[i].iov_base, it, read_len);
	    it = it + read_len;
	  }
	  // others
	  mh.msg_flags = ret_mh.msg_flags;
	  memcpy_to_child(pid, (char*) tcp->u_arg[1], (char*) &mh, sizeof(mh));
	}
	freeifnn(msg_iov);
      }
    }
    
    freeifnn(buff);
    free(prov_pid);
    free(sockid);
    free(pidkey);
  }
}

/* sending side
 * ssize_t send(int sockfd, const void *buf, size_t len, int flags);
 *
 * ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
 *                     const struct sockaddr *dest_addr, socklen_t addrlen);
 *                     // require: dest_addr == NULL && addrlen == 0
 * ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);
 */

void CDEnet_begin_send(struct tcb* tcp) {
  if (CDE_nw_mode && db_isCapturedSock(currdb, tcp->u_arg[0])) {
    vb(2);
    denySyscall(tcp->pid);
  }
}

void CDEnet_end_send(struct tcb* tcp) {
  int sockfd = tcp->u_arg[0];
  if (CDE_provenance_mode && isProvCapturedSock(sockfd)) {
    vb(2);
    if (socket_data_handle(tcp, SOCK_SEND) < 0) {
      // TODO
    }
  }
  if (CDE_nw_mode && db_isCapturedSock(currdb, sockfd)) {
    vb(2);
    if (CDE_verbose_mode >= 3) {
      char buff[KEYLEN];
      size_t buflength = tcp->u_arg[2];
      if (umoven(tcp, tcp->u_arg[1], buflength, buff) < 0) {
	return;
      }
      buff[tcp->u_arg[2]] = '\0';
      vbp(3, "action %d [%ld] checksum %u ", 
	  SOCK_SEND, tcp->u_arg[2], checksum(buff, buflength));
      if (CDE_verbose_mode >= 3) printbuf(buff, buflength);
    }
    long pid = tcp->pid;
    char *pidkey, *sockid;
    db_get_pid_sock(currdb, pid, sockfd, &pidkey, &sockid);
    if (pidkey == NULL || sockid == NULL) {
      vbp(0, "error");
      return;
    }
    ull_t sendid = db_getPkgCounterInc(currdb, pidkey, sockid, SOCK_SEND);
    char* prov_pid = getMappedPid(pidkey);	// convert this pid to corresponding prov_pid
    ull_t u_rval;
    db_getSendRecvResult(netdb, SOCK_SEND, prov_pid, sockid, sendid, &u_rval, NULL); // get recorded result
    struct user_regs_struct regs;
    EXITIF(ptrace(PTRACE_GETREGS, pid, NULL, &regs)<0);
    SET_RETURN_CODE(&regs, u_rval);
    if (u_rval < 0) {
      // set errno? TODO
    }
    EXITIF(ptrace(PTRACE_SETREGS, pid, NULL, &regs)<0);
    
    free(prov_pid);
    free(sockid);
    free(pidkey);
  }
}

void CDEnet_begin_sendmsg(struct tcb* tcp) { //TODO
  vb(2);
  if (CDE_nw_mode && isCurrCapturedSock(tcp->u_arg[0])) {
    denySyscall(tcp->pid);
  }
}
void CDEnet_end_sendmsg(struct tcb* tcp) { //TODO
  vb(2);
}

void CDEnet_begin_getsockname(struct tcb* tcp) {
  vb(2);
  if (CDE_nw_mode) {
    denySyscall(tcp->pid);
  }
}
void CDEnet_end_getsockname(struct tcb* tcp) {
  static int count = 1;
  vbp(0, "%ld\n", tcp->u_rval);
  if (CDE_provenance_mode) {
    print_getsockname_prov(tcp);
  }
  if (CDE_nw_mode) {
    long pid = tcp->pid;
    //~ char *pidkey = db_read_real_pid_key(currdb, pid);
    //~ ull_t listenid = db_getListenId(currdb, pidkey, tcp->u_arg[0]);
    //~ ull_t acceptid = db_getAcceptCounterInc(currdb, pidkey, listenid);
    //~ char* prov_pid = getMappedPid(pidkey);	// convert this pid to corresponding prov_pid
    //~ int u_rval; // get the result of a accept call
    //~ char *addr = NULL;
    
    struct user_regs_struct regs;
    EXITIF(ptrace(PTRACE_GETREGS, pid, NULL, &regs)<0);
    SET_RETURN_CODE(&regs, 0);
    EXITIF(ptrace(PTRACE_SETREGS, pid, NULL, &regs)<0);
    
    socklen_t addrlen = sizeof(struct sockaddr);
    struct sockaddr sa;
    struct sockaddr_in *sain = (struct sockaddr_in*) &(sa.sa_data);
    struct in_addr *inaddr = &(sain->sin_addr);
    
    inet_pton(AF_INET, "0.0.0.0", &(inaddr->s_addr));
    sain->sin_family = AF_INET;
    sain->sin_port = htons(2000+(count++)); // RANDOM
    sa.sa_family = AF_INET;
    
    memcpy_to_child(pid, (char*) tcp->u_arg[1], (char*) &sa, addrlen);
    memcpy_to_child(pid, (char*) tcp->u_arg[2], (char*) &addrlen, sizeof(addrlen)); // TODO: big/little endian
  }
}

// int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
// Return accepted socket fd and fill addr, addrlen
// On error, -1 is returned, and errno is set appropriately.
void CDEnet_begin_accept(struct tcb* tcp) { // TODO
  // ignore! No value is set up yet (ip, port, etc.)
  // we only care of the return of accept
  // which is handled in accept_exit
  vb(2);
  if (CDE_nw_mode) {
    denySyscall(tcp->pid);
  }
}
char* db_getAcceptResult(lvldb_t *mydb, char* pidkey, ull_t listenid, ull_t acceptid, 
    socklen_t *paddrlen, int *presult) {
  char key[KEYLEN];
  char *addrbuf;
  ull_t res;
  size_t len;
  
  sprintf(key, "prv.pid.%s.listenid.%llu.accept.%llu.addr", pidkey, listenid, acceptid);
  addrbuf = db_nread(mydb, key, &len);
  *paddrlen = len;
  
  sprintf(key, "prv.pid.%s.listenid.%llu.accept.%llu", pidkey, listenid, acceptid);
  db_read_ull(mydb, key, &res);
  *presult = res;
  
  return addrbuf;
}
void CDEnet_end_accept(struct tcb* tcp) {
  vb(2);
  if (CDE_provenance_mode) {
    print_accept_prov(tcp);
  }
  if (CDE_nw_mode) {
    long pid = tcp->pid;
    char *pidkey = db_read_real_pid_key(currdb, pid);
    ull_t listenid = db_getListenId(currdb, pidkey, tcp->u_arg[0]);
    ull_t acceptid = db_getAcceptCounterInc(currdb, pidkey, listenid);
    char* prov_pid = getMappedPid(pidkey);	// convert this pid to corresponding prov_pid
    int u_rval; // get the result of a accept call
    socklen_t addrlen;
    char *addr = db_getAcceptResult(netdb, prov_pid, listenid, acceptid, &addrlen, &u_rval);
    if (u_rval > 0) { // create a sock
      //u_rval = socket(AF_INET , SOCK_STREAM , 0);
      u_rval = open("/dev/null", O_RDWR);
    }
    db_setCapturedSock(currdb, u_rval);
    
    // return recorded result
    struct user_regs_struct regs;
    EXITIF(ptrace(PTRACE_GETREGS, pid, NULL, &regs)<0);
    SET_RETURN_CODE(&regs, u_rval);
    if (u_rval <= 0) {
      // set errno? TODO
    } else {
      memcpy_to_child(pid, (char*) tcp->u_arg[1], addr, u_rval);
      memcpy_to_child(pid, (char*) tcp->u_arg[2], (char*) &addrlen, sizeof(addrlen)); // TODO: big/little endian
      db_setSockAcceptId(currdb, pidkey, u_rval, listenid, acceptid);
      db_setupSockAcceptCounter(currdb, pidkey, u_rval, listenid, acceptid);
    }
    EXITIF(ptrace(PTRACE_SETREGS, pid, NULL, &regs)<0);
    
    free(addr);
    free(prov_pid);
    free(pidkey);
  }
}

// int listen(int sockfd, int backlog)
// On success, zero is returned.  On error, -1 is returned,
//   and errno is set appropriately.
void CDEnet_begin_listen(struct tcb* tcp) { //TODO: or ignore? not captured?!?!?
  vb(2);
  if (CDE_nw_mode) {
    denySyscall(tcp->pid);
  }
}
void CDEnet_end_listen(struct tcb* tcp) { // TODO
  vb(2);
  if (CDE_provenance_mode) {
    print_listen_prov(tcp);
  }
  if (CDE_nw_mode) {
    char *pidkey = db_read_real_pid_key(currdb, tcp->pid);
    ull_t id = db_getListenCounterInc(currdb, pidkey);
    char* prov_pid = getMappedPid(pidkey);	// convert this pid to corresponding prov_pid
    int u_rval = db_getListenResult(netdb, prov_pid, id); // get recorded result
    
    // return recorded result
    struct user_regs_struct regs;
    long pid = tcp->pid;
    EXITIF(ptrace(PTRACE_GETREGS, pid, NULL, &regs)<0);
    SET_RETURN_CODE(&regs, u_rval);
    if (u_rval < 0) {
      // set errno? TODO
    } else {
      db_setListenId(currdb, pidkey, tcp->u_arg[0], id);
      db_setupAcceptCounter(currdb, pidkey, id);
    }
    EXITIF(ptrace(PTRACE_SETREGS, pid, NULL, &regs)<0);
    
    free(prov_pid);
    free(pidkey);
  }
}

void CDEnet_begin_read(struct tcb* tcp) { // TODO
  // ssize_t read(int fd, void *buf, size_t count);
  //~ printf("void CDEnet_read(struct tcb* tcp)\n");
  vb(2);
}
void CDEnet_end_read(struct tcb* tcp) { // TODO
  // ssize_t read(int fd, void *buf, size_t count);
  //~ printf("void CDEnet_read(struct tcb* tcp)\n");
  vb(2);
}

// ssize_t write(int fd, const void *buf, size_t count);
// On  success,  the  number  of  bytes  written is returned 
// (zero indicates nothing was written). 
// On error, -1 is returned.
void CDEnet_begin_write(struct tcb* tcp) { // TODO
  // ssize_t write(int fd, const void *buf, size_t count);
  //~ printf("void CDEnet_write(struct tcb* tcp)\n");
  vb(2);
}
void CDEnet_end_write(struct tcb* tcp) { // TODO
  // ssize_t write(int fd, const void *buf, size_t count);
  //~ printf("void CDEnet_write(struct tcb* tcp)\n");
  vb(2);
}

// int close(int fd);
void CDEnet_close(struct tcb* tcp) {
  int sockfd = tcp->u_arg[0];
  vb(2);
  if (CDE_provenance_mode) {
    print_sock_close(tcp);
  }
  if (CDE_nw_mode && db_isCapturedSock(currdb, sockfd)) {
    db_remove_sock(currdb, tcp->pid, sockfd);
  }
}

/* =============
 * Database info extraction methods
 * =============
 */
void init_nwdb() {
  char path[PATH_MAX];
  char *err = NULL;
  
  sprintf(path, "%s/%s", cde_pseudo_pkg_dir, DB_NAME);
  if (access(path, R_OK)==-1) {
    fprintf(stderr, "Network provenance database does not exist!\n");
    exit(-1);
  }
  
  netdb = malloc(sizeof(lvldb_t));
  netdb->options = leveldb_options_create();
  leveldb_options_set_create_if_missing(netdb->options, 0);
  netdb->db = leveldb_open(netdb->options, path, &err);
  
  if (err != NULL || netdb == NULL) {
    fprintf(stderr, "Leveldb open fail!\n");
    exit(-1);
  }
  
  assert(netdb->db!=NULL);
  netdb->woptions = leveldb_writeoptions_create();
  netdb->roptions = leveldb_readoptions_create();
  
  /* reset error var */
  leveldb_free(err); err = NULL;
  
  /* read in root pid */
  netdb_root = db_readc(netdb, "meta.root");
  assert(netdb_root != NULL);
  
  if (CDE_nw_mode) {
    /* create temp db for current execution graph */
    sprintf(path, "%s/%s.tempXXXXXX", cde_pseudo_pkg_dir, DB_NAME);
    if (mkdtemp(path) == NULL) {
      fprintf(stderr, "Cannot create temp db!\n");
      exit(-1);
    }
    currdb = malloc(sizeof(lvldb_t));
    currdb->options = leveldb_options_create();
    leveldb_options_set_create_if_missing(currdb->options, 1);
    currdb->db = leveldb_open(currdb->options, path, &err);
    assert(currdb->db!=NULL);
    leveldb_free(err); err = NULL;
    currdb->woptions = leveldb_writeoptions_create();
    currdb->roptions = leveldb_readoptions_create();
    
    db_write_root(currdb, getpid());
  }
}


// get the corresponding pidkey from traced execution
// some graph algorithm is needed here
// @input current pidkey
// @return pidkey from netdb
// TODO
char* getMappedPid(char* pidkey) {
  // TODO: need a cache for this operation as well
  char key[KEYLEN], *p;
  const char *read;
  size_t read_len;
  ull_t childid;
  int n=0, i;
  ull_t idlist[100]; // enough?
  
  p = pidkey;
  while (p != NULL) {
    sprintf(key, "prv.pid.%s.childid", p);
    vbp(3, "%s\n", key);
    db_read_ull(currdb, key, &childid);
    idlist[n++] = childid;
    
    sprintf(key, "prv.pid.%s.parent", p);
    if (p != pidkey && p != NULL) free(p);
    p = db_readc(currdb, key);
  }
  vbp(2, "%s -> [%d] [", pidkey, n);
  if (CDE_verbose_mode >= 2) {
    for (i=0; i<n; i++) {
      fprintf(stderr, "%llu, ", idlist[i]);
    }
    fprintf(stderr, "]\n");
  }
  
  if (PIDKEY != NULL) {
    p = strdup(PIDKEY);
    n -= 2;
  } else {
    p = db_readc(netdb, "meta.root");
    sprintf(key, "prv.pid.%s.exec.", p);
    //~ sprintf(key, "prv.pid.%s.actualexec.", p);
    free(p);
    leveldb_iterator_t *it = leveldb_create_iterator(netdb->db, netdb->roptions);
    leveldb_iter_seek(it, key, strlen(key));
      
    read = leveldb_iter_value(it, &read_len);
    p = malloc(read_len + 1);
    memcpy(p, read, read_len);
    p[read_len] = '\0';
    n-=2; // skip the first root -> child that I just did
  }
  while (n>0) {
    sprintf(key, "prv.pid.%s.child.%llu", p, idlist[n-1]);
    n--;
    free(p);
    p = db_readc(netdb, key);
    vbp(3, "%s\n", key);
  }
  vbp(2, "return %s\n", p);
  return p;
  
  //~ char *value;
  //~ const char *read;
  //~ size_t read_len;
  //~ 
  //~ sprintf(key, "prv.pid.%s.parent.", netdb_root);
  //~ leveldb_iterator_t *it = leveldb_create_iterator(netdb->db, netdb->roptions);
  //~ leveldb_iter_seek(it, key, strlen(key));
    //~ 
  //~ read = leveldb_iter_value(it, &read_len);
  //~ value = malloc(read_len + 1);
  //~ memcpy(value, read, read_len);
  //~ value[read_len] = '\0';
  //~ return value;
}

// sample code from systrace
//~ static void
//~ linux_abortsyscall(pid_t pid)
//~ {
	//~ struct user_regs_struct regs;
	//~ int res = ptrace(PTRACE_GETREGS, pid, NULL, &regs);
	//~ if (res == -1) {
		//~ /* We might have killed the process in the mean time */
		//~ if (errno == ESRCH)
			//~ return;
		//~ err(1, "%s: ptrace getregs", __func__);
	//~ }
	//~ SYSCALL_NUM(&regs) = 0xbadca11;
	//~ res = ptrace(PTRACE_SETREGS, pid, NULL, &regs);
	//~ if (res == -1)
		//~ err(1, "%s: ptrace getregs", __func__);
//~ }
