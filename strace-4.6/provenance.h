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


#endif // _PROVENANCE_H
