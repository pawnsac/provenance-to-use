#ifndef _CDENET_H
#define _CDENET_H

void CDEnet_sin_dict_load();

void CDEnet_begin_bindconnect(struct tcb* tcp, int isConnect);
void CDEnet_end_bindconnect(struct tcb* tcp, int isConnect);
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
