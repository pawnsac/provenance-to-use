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

#endif // _CDENET_H
