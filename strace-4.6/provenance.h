#ifndef _PROVENANCE_H
#define _PROVENANCE_H

void init_prov();
void print_exec_prov(struct tcb *tcp);
void print_IO_prov(struct tcb *tcp, char* filename, const char* syscall_name);
void print_spawn_prov(struct tcb *tcp);
void rm_pid_prov(pid_t pid);

#endif // _PROVENANCE_H
