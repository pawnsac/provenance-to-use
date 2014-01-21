#ifndef _CDENET_H
#define _CDENET_H

//#define SOCK_CONECT (0)
//#define SOCK_SENDTO (1)

enum sock_action{SOCK_CONECT, \
  SOCK_SEND, SOCK_SENDTO, SOCK_SENDMSG,
  SOCK_RECV, SOCK_RECVFROM, SOCK_RECVMSG};

void CDEnet_sin_dict_load();

#endif // _CDENET_H
