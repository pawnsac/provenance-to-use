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

#include "cdenet.h"

extern int CDE_exec_mode;
extern int CDE_block_net_access;

void CDEnet_begin_socket_bind_or_connect(struct tcb* tcp) {

  if (!entering(tcp)) {
  	return;
  }
  
  // only do this redirection in CDE_exec_mode
  if (!CDE_exec_mode) {
    return;
  }
  
  // copied from printsock function of net.c
  long addr = tcp->u_arg[1];
  int addrlen = tcp->u_arg[2];

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
    return;
  }

  if (addrlen < 2 || addrlen > sizeof(addrbuf)) {
    addrlen = sizeof(addrbuf);
  }

  memset(&addrbuf, 0, sizeof(addrbuf));
  if (umoven(tcp, addr, addrlen, addrbuf.pad) < 0) {
    return;
  }
  addrbuf.pad[sizeof(addrbuf.pad) - 1] = '\0';

  switch (addrbuf.sa.sa_family) {
  case AF_UNIX:
  	// already handled in cde
    break;
  case AF_INET:
  	// TODO: replace ip with new ip
    tprintf("sin_port=htons(%u), sin_addr=inet_addr(\"%s\")",
      ntohs(addrbuf.sin.sin_port), inet_ntoa(addrbuf.sin.sin_addr));
    break;
#ifdef HAVE_INET_NTOP
  case AF_INET6:
    inet_ntop(AF_INET6, &addrbuf.sa6.sin6_addr, string_addr, sizeof(string_addr));
    tprintf("sin6_port=htons(%u), inet_pton(AF_INET6, \"%s\", &sin6_addr), sin6_flowinfo=%u",
        ntohs(addrbuf.sa6.sin6_port), string_addr,
        addrbuf.sa6.sin6_flowinfo);
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID
    {
#if defined(HAVE_IF_INDEXTONAME) && defined(IN6_IS_ADDR_LINKLOCAL) && defined(IN6_IS_ADDR_MC_LINKLOCAL)
        int numericscope = 0;
        if (IN6_IS_ADDR_LINKLOCAL (&addrbuf.sa6.sin6_addr)
          || IN6_IS_ADDR_MC_LINKLOCAL (&addrbuf.sa6.sin6_addr)) {
      char scopebuf[IFNAMSIZ + 1];

      if (if_indextoname (addrbuf.sa6.sin6_scope_id, scopebuf) == NULL)
          numericscope++;
      else
          tprintf(", sin6_scope_id=if_nametoindex(\"%s\")", scopebuf);
        } else
      numericscope++;

        if (numericscope)
#endif
      tprintf(", sin6_scope_id=%u", addrbuf.sa6.sin6_scope_id);
    }
#endif
        break;
#endif
#if defined(AF_IPX) && defined(linux)
  case AF_IPX:
    {
      int i;
      tprintf("sipx_port=htons(%u), ",
          ntohs(addrbuf.sipx.sipx_port));
      /* Yes, I know, this does not look too
       * strace-ish, but otherwise the IPX
       * addresses just look monstrous...
       * Anyways, feel free if you don't like
       * this way.. :)
       */
      tprintf("%08lx:", (unsigned long)ntohl(addrbuf.sipx.sipx_network));
      for (i = 0; i<IPX_NODE_LEN; i++)
        tprintf("%02x", addrbuf.sipx.sipx_node[i]);
      tprintf("/[%02x]", addrbuf.sipx.sipx_type);
    }
    break;
#endif /* AF_IPX && linux */
#ifdef AF_PACKET
  case AF_PACKET:
    {
      int i;
      tprintf("proto=%#04x, if%d, pkttype=",
          ntohs(addrbuf.ll.sll_protocol),
          addrbuf.ll.sll_ifindex);
      printxval(af_packet_types, addrbuf.ll.sll_pkttype, "?");
      tprintf(", addr(%d)={%d, ",
          addrbuf.ll.sll_halen,
          addrbuf.ll.sll_hatype);
      for (i=0; i<addrbuf.ll.sll_halen; i++)
        tprintf("%02x", addrbuf.ll.sll_addr[i]);
    }
    break;

#endif /* AF_APACKET */
#ifdef AF_NETLINK
  case AF_NETLINK:
    tprintf("pid=%d, groups=%08x", addrbuf.nl.nl_pid, addrbuf.nl.nl_groups);
    break;
#endif /* AF_NETLINK */
  /* AF_AX25 AF_APPLETALK AF_NETROM AF_BRIDGE AF_AAL5
  AF_X25 AF_ROSE etc. still need to be done */

  default:
    tprintf("sa_data=");
    printstr(tcp, (long) &((struct sockaddr *) addr)->sa_data,
      sizeof addrbuf.sa.sa_data);
    break;
  }

  return;
  // cde stuff not used in here
  /* AF_FILE is also a synonym for AF_UNIX */
  if (addrbuf.sa.sa_family == AF_UNIX) {
    if (addrlen > 2 && addrbuf.sau.sun_path[0]) {
      //tprintf("path=");

      // addr + sizeof(addrbuf.sau.sun_family) is the location of the real path
      char* original_path = strcpy_from_child(tcp, addr + sizeof(addrbuf.sau.sun_family));
      if (original_path) {
        //printf("original_path='%s'\n", original_path);

        char* redirected_path =
          redirect_filename_into_cderoot(original_path, tcp->current_dir, tcp);

        // could be null if path is being ignored by cde.options
        if (redirected_path) {
          //printf("redirected_path: '%s'\n", redirected_path);

          unsigned long new_pathlen = strlen(redirected_path);

          // alter the socket address field to point to redirected path
          memcpy_to_child(tcp->pid, (char*)(addr + sizeof(addrbuf.sau.sun_family)),
                          redirected_path, new_pathlen + 1);

          free(redirected_path);


          // remember the 2 extra bytes for the sun_family field!
          unsigned long new_totallen = new_pathlen + sizeof(addrbuf.sau.sun_family);

          struct user_regs_struct cur_regs;
          EXITIF(ptrace(PTRACE_GETREGS, tcp->pid, NULL, (long)&cur_regs) < 0);

#if defined (I386)
          // on i386, things are tricky tricky!
          // the kernel uses socketcall() as a common entry
          // point for all socket-related system calls
          // http://www.kernel.org/doc/man-pages/online/pages/man2/socketcall.2.html
          //
          // the ecx register contains a pointer to an array of 3 pointers
          // (of size 'unsigned long'), which represents the 3 arguments
          // to the bind/connect syscall.  they are:
          //   arg[0] - socket number
          //   arg[1] - pointer to socket address structure
          //   arg[2] - length of socket address structure

          // we need to alter the length field to new_totallen,
          // which is VERY IMPORTANT or else the path that the
          // kernel sees will be truncated!!!

          // we want to override arg[2], which is located at:
          //   cur_regs.ecx + 2*sizeof(unsigned long)
          memcpy_to_child(tcp->pid, (char*)(cur_regs.ecx + 2*sizeof(unsigned long)),
                          (char*)(&new_totallen), sizeof(unsigned long));
#elif defined(X86_64)
          // on x86-64, things are much simpler.  the length field is
          // stored in %rdx (the third argument), so simply override
          // that register with new_totallen
          cur_regs.rdx = (long)new_totallen;
          ptrace(PTRACE_SETREGS, tcp->pid, NULL, (long)&cur_regs);
#else
          #error "Unknown architecture (not I386 or X86_64)"
#endif
        }

        free(original_path);
      }
    }
  }
  else {
    if (CDE_block_net_access) {
      // blank out the sockaddr argument if you want to block network access
      //
      // I think that blocking 'bind' prevents setting up sockets to accept
      // incoming connections, and blocking 'connect' prevents outgoing
      // connections.
      struct sockaddr s;
      memset(&s, 0, sizeof(s));
      memcpy_to_child(tcp->pid, (char*)addr, (char*)&s, sizeof(s));
    }
  }
}