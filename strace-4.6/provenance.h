#ifndef _PROVENANCE_H
#define _PROVENANCE_H

extern char* Prov_db_id;           // leveldb provdb id
extern char Prov_prov_mode;        // true if auditing (opposite of Cde_exec_mode)
extern char Prov_no_app_capture;   // if true, run cde to collect prov but don't capture app

// initialize leveldb prov-db and provlog file
void init_prov ();
// log proc exec call to provlog & leveldb if auditing, to stderr if verbose
void print_begin_execve_prov (struct tcb* tcp, char* dbid, char* ssh_host);
// log ending proc exec call to provlog & leveldb if auditing, to stderr if verbose
void print_end_execve_prov (struct tcb* tcp);
// log file open/openat to provlog & leveldb if auditing, to stderr if verbose
void print_open_prov (struct tcb* tcp, const char* syscall_name);
// log file read to provlog & leveldb if auditing, to stderr if verbose
void print_read_prov (struct tcb* tcp, const char* syscall_name, const int path_index);
// log file write to provlog & leveldb if auditing, to stderr if verbose
void print_write_prov (struct tcb* tcp, const char* syscall_name, const int path_index);
// log file hardlink/symlink to provlog & leveldb if auditing
void print_link_prov (struct tcb* tcp, const char* syscall_name, const int realpath_index, const int linkpath_index);
// log file rename/move to provlog & leveldb if auditing
void print_rename_prov (struct tcb* tcp, const int renameat);

void print_act_prov(struct tcb *tcp, const char *action);
void print_sock_prov(struct tcb *tcp, int action, unsigned int port, unsigned long ipv4);
void print_newsock_prov(struct tcb *tcp, int action, unsigned int s_port, unsigned long s_ipv4, unsigned int d_port, unsigned long d_ipv4, int sk);
void print_sock_action(struct tcb *tcp, int sockfd, char *buf, long len_param, int flags, long len_result, int action, void *msg);
void print_connect_prov(struct tcb *tcp, int sockfd, char* addr, int addr_len, long u_rval, char *ips);
void print_listen_prov(struct tcb *tcp);
int isProvCapturedSock(int sockfd);
void print_accept_prov(struct tcb *tcp);
void print_fd_close(struct tcb *tcp);
void print_getsockname_prov(struct tcb *tcp);

#endif // _PROVENANCE_H

