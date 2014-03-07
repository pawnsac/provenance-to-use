#ifndef _PROVENANCE_H
#define _PROVENANCE_H

typedef long long int ull_t;

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
    char *buf, long len_param, int flags,
    long len_result, int action, void *msg);
void print_connect_prov(struct tcb *tcp, 
    int sockfd, char* addr, int addr_len, long u_rval);
void print_listen_prov(struct tcb *tcp);
int isProvCapturedSock(int sockfd);

void print_getsockname_prov(struct tcb *tcp);

#endif // _PROVENANCE_H
