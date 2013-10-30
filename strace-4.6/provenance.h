#ifndef _PROVENANCE_H
#define _PROVENANCE_H

void init_prov();

void print_exec_prov(struct tcb *tcp);
void print_IO_prov(struct tcb *tcp, char* filename, const char* syscall_name);
void print_spawn_prov(struct tcb *tcp);
void print_sock_prov(struct tcb *tcp, const char* op, unsigned int port, unsigned long ipv4);
void print_act_prov(struct tcb *tcp, const char* action);
void print_newsock_prov(struct tcb *tcp, const char* op, \
  unsigned int s_port, unsigned long s_ipv4, \
  unsigned int d_port, unsigned long d_ipv4, int sk);

void rm_pid_prov(pid_t pid);

#endif // _PROVENANCE_H
