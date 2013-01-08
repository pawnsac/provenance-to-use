#ifndef _PROVENANCE_H
#define _PROVENANCE_H

void initprov();
void printexecprov(struct tcb *tcp);
void printIOprov(struct tcb *tcp);
void printSpawnprov(struct tcb *tcp);

#endif // _PROVENANCE_H
