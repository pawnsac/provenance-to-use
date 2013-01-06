#include <assert.h>

#include "defs.h"
#include "provenance.h"

/* macros */
#ifndef MAX
#define MAX(a,b)		(((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)		(((a) < (b)) ? (a) : (b))
#endif

char CDE_provenance_mode = 0;
FILE* CDE_provenance_logfile = NULL;

extern int string_quote(const char *instr, char *outstr, int len, int size);
extern char* strcpy_from_child_or_null(struct tcb* tcp, long addr);


/*
 * Print string specified by address `addr' and length `len'.
 * If `len' < 0, treat the string as a NUL-terminated string.
 * If string length exceeds `max_strlen', append `...' to the output.
 */
void
printstrprov(FILE* logfile, struct tcb *tcp, long addr, int len)
{
	static char *str = NULL;
	static char *outstr;
	int size;

	if (!addr) {
		fprintf(logfile, "NULL");
		return;
	}
	/* Allocate static buffers if they are not allocated yet. */
	if (!str)
		str = malloc(max_strlen + 1);
	if (!outstr)
		outstr = malloc(4 * max_strlen + sizeof "\"...\"");
	if (!str || !outstr) {
		fprintf(stderr, "out of memory\n");
		fprintf(logfile, "%#lx", addr);
		return;
	}

	if (len < 0) {
		/*
		 * Treat as a NUL-terminated string: fetch one byte more
		 * because string_quote() quotes one byte less.
		 */
		size = max_strlen + 1;
		str[max_strlen] = '\0';
		if (umovestr(tcp, addr, size, str) < 0) {
			fprintf(logfile, "%#lx", addr);
			return;
		}
	}
	else {
		size = MIN(len, max_strlen);
		if (umoven(tcp, addr, size, str) < 0) {
			fprintf(logfile, "%#lx", addr);
			return;
		}
	}

	if (string_quote(str, outstr, len, size) &&
	    (len < 0 || len > max_strlen))
		strcat(outstr, "...");

	fprintf(logfile, "%s", outstr);
}


void
printargprov(FILE* logfile, struct tcb *tcp, long addr)
{
	union {
		unsigned int p32;
		unsigned long p64;
		char data[sizeof(long)];
	} cp;
	const char *sep;
	int n = 0;

  fprintf(logfile, "[");
	cp.p64 = 1;
	for (sep = ""; !abbrev(tcp) || n < max_strlen / 2; sep = ", ", ++n) {
		if (umoven(tcp, addr, personality_wordsize[current_personality],
			   cp.data) < 0) {
			fprintf(logfile, "%#lx\n", addr);
			return;
		}
		if (personality_wordsize[current_personality] == 4)
			cp.p64 = cp.p32;
		if (cp.p64 == 0)
			break;
		fprintf(logfile, "%s", sep);
		printstrprov(logfile, tcp, cp.p64, -1);
		addr += personality_wordsize[current_personality];
	}
	if (cp.p64)
		fprintf(logfile, "%s...", sep);

	fprintf(logfile, "]\n");
}


void printexecprov(struct tcb *tcp) {
  if (CDE_provenance_mode) {
    char* opened_filename = strcpy_from_child_or_null(tcp, tcp->u_arg[0]);
    char* filename_abspath = canonicalize_path(opened_filename, tcp->current_dir);
    int parentPid = tcp->parent == NULL ? -1 : tcp->parent->pid;
    assert(filename_abspath);
    fprintf(CDE_provenance_logfile, "%d %d EXECVE %d %s ", (int)time(0), parentPid, tcp->pid, filename_abspath);
    printargprov(CDE_provenance_logfile, tcp, tcp->u_arg[1]);
    free(filename_abspath);
    free(opened_filename);
  }
}

void printIOprov(struct tcb *tcp) {
  if (CDE_provenance_mode) {
    // TODO: move fprintf from cde.c here
  }
}

void printSpawnprov(struct tcb *tcp) {
  if (CDE_provenance_mode) {
    fprintf(CDE_provenance_logfile, "%d %u SPAWN %u\n", (int)time(0), tcp->parent->pid, tcp->pid);
  }
}

