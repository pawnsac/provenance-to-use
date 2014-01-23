#ifndef _PROVENANCE_H
#define _PROVENANCE_H

enum provenance_type {
  PRV_RDONLY=1, PRV_WRONLY=2, PRV_RDWR = 3, PRV_UNKNOWNIO=4, 
  PRV_EXECVE=17, PRV_SPAWN=18, PRV_LEXIT=19, 
  STAT_MEM=33,
  PRV_ACTION=65,
  PRV_INVALID=127};

void print_syscall_read_prov(struct tcb *tcp, const char *syscall_name, int pos);
void print_syscall_write_prov(struct tcb *tcp, const char *syscall_name, int pos);
void print_syscall_two_prov(struct tcb *tcp, const char *syscall_name, int posread, int poswrite);

void print_open_prov(struct tcb *tcp, const char *syscall_name);
void print_rename_prov(struct tcb *tcp, int renameat);

void print_exec_prov(struct tcb *tcp);
void print_execdone_prov(struct tcb *tcp);

void print_act_prov(struct tcb *tcp, const char *action);
void print_sock_prov(struct tcb *tcp, int action, 
    unsigned int port, unsigned long ipv4);
void print_newsock_prov(struct tcb *tcp, int action,
    unsigned int s_port, unsigned long s_ipv4,
    unsigned int d_port, unsigned long d_ipv4, int sk);
void print_sock_action(struct tcb *tcp, int sockfd,
    const char *buf, size_t len_param, int flags,
    size_t len_result, int action);

#endif // _PROVENANCE_H
