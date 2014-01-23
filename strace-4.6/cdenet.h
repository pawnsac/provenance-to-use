#ifndef _CDENET_H
#define _CDENET_H

//#define SOCK_CONECT (0)
//#define SOCK_SENDTO (1)

enum sock_action{SOCK_SEND, SOCK_SENDTO, SOCK_SENDMSG,  // 0, 1, 2
  SOCK_RECV, SOCK_RECVFROM, SOCK_RECVMSG,               // 3, 4, 5
  SOCK_BIND, SOCK_CONNECT, SOCK_LISTEN,                 // 6, 7, 8
  SOCK_ACCEPT                                           // 9
  };

void CDEnet_sin_dict_load();

#endif // _CDENET_H
