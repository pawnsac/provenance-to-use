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

#include "defs.h"

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

#if defined(HAVE_LINUX_NETLINK_H)
#include <linux/netlink.h>
#endif

#if defined(HAVE_LINUX_IF_PACKET_H)
#include <linux/if_packet.h>
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

// variables from cde.c
extern int CDE_exec_mode;
extern int CDE_block_net_access;

// function from cde.c
extern void memcpy_to_child(int pid, char* dst_child, char* src, int size);

// TODO: read from external file / socket on initialization
int N_SIN = 1;
struct sockaddr_in sin_key[1];
struct sockaddr_in sin_value[1];

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

int getsockinfo(struct tcb *tcp, long addr, int addrlen, socketdata_t *psock)
{
	union {
		char pad[128];
		struct sockaddr sa;
		struct sockaddr_in sin;
		struct sockaddr_un sau;
#ifdef HAVE_INET_NTOP
		struct sockaddr_in6 sa6;
#endif
#if defined(LINUX) && defined(AF_IPX)
		struct sockaddr_ipx sipx;
#endif
#ifdef AF_PACKET
		struct sockaddr_ll ll;
#endif
#ifdef AF_NETLINK
		struct sockaddr_nl nl;
#endif
	} addrbuf;
	char string_addr[100];

	if (addr == 0) {
		return -1;
	}

	if (addrlen < 2 || addrlen > sizeof(addrbuf))
		addrlen = sizeof(addrbuf);

	memset(&addrbuf, 0, sizeof(addrbuf));
	if (umoven(tcp, addr, addrlen, addrbuf.pad) < 0) {
		return -1;
	}
	addrbuf.pad[sizeof(addrbuf.pad) - 1] = '\0';

	psock->saf = addrbuf.sa.sa_family;

	switch (addrbuf.sa.sa_family) {
	case AF_UNIX:
	  // AF_FILE is also a synonym for AF_UNIX
	  // these are file operations
		break;
	case AF_INET:
		//tprintf("sin_port=htons(%u), sin_addr=inet_addr(\"%s\")",
			//ntohs(addrbuf.sin.sin_port), inet_ntoa(addrbuf.sin.sin_addr));
		psock->port = ntohs(addrbuf.sin.sin_port);
		psock->ip.ipv4 = addrbuf.sin.sin_addr.s_addr;
		break;
#ifdef HAVE_INET_NTOP
	case AF_INET6:
		inet_ntop(AF_INET6, &addrbuf.sa6.sin6_addr, string_addr, sizeof(string_addr));
		//tprintf("sin6_port=htons(%u), inet_pton(AF_INET6, \"%s\", &sin6_addr), sin6_flowinfo=%u",
		//		ntohs(addrbuf.sa6.sin6_port), string_addr,
		//		addrbuf.sa6.sin6_flowinfo);
		psock->port = ntohs(addrbuf.sa6.sin6_port);
		memcpy(&addrbuf.sa6.sin6_addr, &psock->ip.ipv6, 16);
		break;
#endif
 
  /* Quan - not handle AF_IPX AF_APACKET AF_NETLINK */
	/* AF_AX25 AF_APPLETALK AF_NETROM AF_BRIDGE AF_AAL5
	AF_X25 AF_ROSE etc. still need to be done */

	default:
		break;
	}
	return 0;
}

void printSockInfo(struct tcb* tcp, const char *op, \
    unsigned int d_port, unsigned long d_ipv4, int sk) {
  struct sockaddr_in localAddr;
  socklen_t len = sizeof(localAddr);;
  if (getsockname(sk, (struct sockaddr*)&localAddr, &len)<0) {
    localAddr.sin_port = 0;
    localAddr.sin_addr.s_addr = 0;
    printf("error: %d on %d\n", errno, sk);
  }
  print_newsock_prov(tcp, op, ntohs(localAddr.sin_port), \
      localAddr.sin_addr.s_addr, d_port, d_ipv4, sk);
}

void CDEnet_bind(struct tcb* tcp) {
  socketdata_t sock;
  int sk = tcp->u_rval;
  if (!entering(tcp)) {
    if (getsockinfo(tcp, tcp->u_arg[1], tcp->u_arg[2], &sock)>=0) {
      printSockInfo(tcp, "BIND", sock.port, sock.ip.ipv4, sk);
    }		  
	}
}
void CDEnet_connect(struct tcb* tcp) {
  socketdata_t sock;
  if (entering(tcp)) {
		if (getsockinfo(tcp, tcp->u_arg[1], tcp->u_arg[2], &sock)>=0) {
		  print_newsock_prov(tcp, "CONNECT", 0, 0, sock.port, sock.ip.ipv4, tcp->u_rval);
		}		  
	}
}
void CDEnet_recvmsg(struct tcb* tcp) { //TODO
  socketdata_t sock;
  if (getsockinfo(tcp, tcp->u_arg[1], tcp->u_arg[2], &sock)>=0) {
    print_sock_prov(tcp, "RECVMSG", sock.port, sock.ip.ipv4);
  }
}
void CDEnet_recvfrom(struct tcb* tcp) { //TODO
  socketdata_t sock;
  if (getsockinfo(tcp, tcp->u_arg[1], tcp->u_arg[2], &sock)>=0) {
    print_sock_prov(tcp, "RECVFROM", sock.port, sock.ip.ipv4);
  }
}
void CDEnet_recv(struct tcb* tcp) { //TODO
  socketdata_t sock;
  
  if (getsockinfo(tcp, tcp->u_arg[1], tcp->u_arg[2], &sock)>=0) {
    print_sock_prov(tcp, "RECV", sock.port, sock.ip.ipv4);
  }
}
void CDEnet_sendmsg(struct tcb* tcp) { //TODO
  socketdata_t sock;
  if (getsockinfo(tcp, tcp->u_arg[1], tcp->u_arg[2], &sock)>=0) {
    print_sock_prov(tcp, "SENDMSG", sock.port, sock.ip.ipv4);
  }
}
void CDEnet_sendto(struct tcb* tcp) { //TODO
  socketdata_t sock;
  if (getsockinfo(tcp, tcp->u_arg[1], tcp->u_arg[2], &sock)>=0) {
    print_sock_prov(tcp, "SENDTO", sock.port, sock.ip.ipv4);
  }
}
void CDEnet_send(struct tcb* tcp) { //TODO
  socketdata_t sock;
  if (getsockinfo(tcp, tcp->u_arg[1], tcp->u_arg[2], &sock)>=0) {
    print_sock_prov(tcp, "SEND", sock.port, sock.ip.ipv4);
  }
}
void CDEnet_accept(struct tcb* tcp) { 
  // done, but should be ignore, since we only care of the return of accept
  // which is handled in accept_exit
  print_act_prov(tcp, "ACCEPT");
}
void CDEnet_accept_exit(struct tcb* tcp) {
  socketdata_t sock;
  int sk = tcp->u_rval;
  if (getsockinfo(tcp, tcp->u_arg[1], tcp->u_arg[2], &sock)>=0) {
    printSockInfo(tcp, "ACCEPT_EXIT", sock.port, sock.ip.ipv4, sk);
  }
}
void CDEnet_listen(struct tcb* tcp) { //TODO: or ignore? not captured?!?!?
  socketdata_t sock;
  if (getsockinfo(tcp, tcp->u_arg[1], tcp->u_arg[2], &sock)>=0) {
    print_sock_prov(tcp, "LISTEN", sock.port, sock.ip.ipv4);
  }
}

