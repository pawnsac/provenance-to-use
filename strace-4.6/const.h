#ifndef _HEADER_CONST_H
#define _HEADER_CONST_H

enum provenance_type {
  PRV_RDONLY=1, PRV_WRONLY=2, PRV_RDWR = 3, PRV_UNKNOWNIO=4, 
  PRV_EXECVE=17, PRV_SPAWN=18, PRV_LEXIT=19, 
  STAT_MEM=33,
  PRV_ACTION=65,
  PRV_INVALID=127};

enum sock_action{SOCK_SEND, SOCK_SENDTO, SOCK_SENDMSG,  // 0, 1, 2
  SOCK_RECV, SOCK_RECVFROM, SOCK_RECVMSG,               // 3, 4, 5
  SOCK_BIND, SOCK_CONNECT, SOCK_LISTEN,                 // 6, 7, 8
  SOCK_ACCEPT                                           // 9
  };

#ifndef KEYLEN
#define KEYLEN (1024)
#endif

#endif // _HEADER_CONST_H
