#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#include "defs.h"
#include "provenance.h"

/* macros */
#ifndef MAX
#define MAX(a,b)		(((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)		(((a) < (b)) ? (a) : (b))
#endif

extern char CDE_exec_mode;

char CDE_provenance_mode = 0;
FILE* CDE_provenance_logfile = NULL;
pthread_mutex_t mut_logfile = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_pidlist = PTHREAD_MUTEX_INITIALIZER;
typedef struct {
  pid_t pv[1000]; // the list
  int pc; // total count
  
} pidlist_t;
static pidlist_t pidlist;

extern int string_quote(const char *instr, char *outstr, int len, int size);
extern char* strcpy_from_child_or_null(struct tcb* tcp, long addr);
extern char* canonicalize_path(char* path, char* relpath_base);


/*
 * Print string specified by address `addr' and length `len'.
 * If `len' < 0, treat the string as a NUL-terminated string.
 * If string length exceeds `max_strlen', append `...' to the output.
 */
void
print_str_prov(FILE* logfile, struct tcb *tcp, long addr, int len)
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
print_arg_prov(FILE* logfile, struct tcb *tcp, long addr)
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
		print_str_prov(logfile, tcp, cp.p64, -1);
		addr += personality_wordsize[current_personality];
	}
	if (cp.p64)
		fprintf(logfile, "%s...", sep);

	fprintf(logfile, "]\n");
}


void print_exec_prov(struct tcb *tcp) {
  if (CDE_provenance_mode) {
    char* opened_filename = strcpy_from_child_or_null(tcp, tcp->u_arg[0]);
    char* filename_abspath = canonicalize_path(opened_filename, tcp->current_dir);
    int parentPid = tcp->parent == NULL ? -1 : tcp->parent->pid;
    assert(filename_abspath);
    fprintf(CDE_provenance_logfile, "%d %d EXECVE %u %s ", (int)time(0), parentPid, tcp->pid, filename_abspath);
    print_arg_prov(CDE_provenance_logfile, tcp, tcp->u_arg[1]);
    free(filename_abspath);
    free(opened_filename);
  }
}

void print_IO_prov(struct tcb *tcp) {
  if (CDE_provenance_mode) {
    // TODO: move fprintf from cde.c here
  }
}

void print_spawn_prov(struct tcb *tcp) {
  if (CDE_provenance_mode) {
    fprintf(CDE_provenance_logfile, "%d %u SPAWN %u\n", (int)time(0), tcp->parent->pid, tcp->pid);
  }
}

void print_curr_prov(pidlist_t *pidlist_p) {
  int i, curr_time;
  FILE *f;
  char buff[1024];
  long unsigned int rss;
  
  pthread_mutex_lock(&mut_pidlist);
  curr_time = (int)time(0);
  for (i = 0; i < pidlist_p->pc; i++) {
    sprintf(buff, "/proc/%d/stat", pidlist_p->pv[i]);
    f = fopen(buff, "r");
    if (f==NULL) continue;
    fgets(buff, 1024, f);
    // details of format: http://git.kernel.org/?p=linux/kernel/git/stable/linux-stable.git;a=blob_plain;f=fs/proc/array.c;hb=d1c3ed669a2d452cacfb48c2d171a1f364dae2ed
    sscanf(buff, "%*d %*s %*c %*d %*d %*d %*d %*d %*lu %*lu \
  %*lu %*lu %*lu %*lu %*lu %*ld %*ld %*ld %*ld %*ld %*ld %*lu %lu ", &rss);
    fclose(f);
    fprintf(CDE_provenance_logfile, "%d %u MEM %lu\n", curr_time, pidlist_p->pv[i], rss);
  }
  pthread_mutex_unlock(&mut_pidlist);
}

void *capture_cont_prov(void* ptr) {
  pidlist_t *pidlist_p = (pidlist_t*) ptr;
  // Wait till we have the first pid, which should be the traced process.
  // Start recording memory footprint.
  // Ok to stop when there is no more pid, since by then,
  // the original tracded process should have stopped.
  while (pidlist_p->pc == 0) usleep(100000);
  while (pidlist_p->pc > 0) { // recording
    print_curr_prov(pidlist_p);
    sleep(1); // TODO: configurable
  } // done recording: pidlist.pc == 0
  pthread_mutex_destroy(&mut_pidlist);
  
	if (CDE_provenance_logfile)
  	fclose(CDE_provenance_logfile);
  pthread_mutex_destroy(&mut_logfile);

  return NULL;
}

void init_prov() {
  pthread_t ptid;
  CDE_provenance_mode = !CDE_exec_mode;
  if (CDE_provenance_mode) {
    pthread_mutex_init(&mut_logfile, NULL);
    // create NEW provenance log file
    if (access("provenance.log", R_OK)==-1)
      CDE_provenance_logfile = fopen("provenance.log", "w");
    else {
      int i=1;
      char path[100];
      // check through provenance.$i.log to find a new file name
      do {
        bzero(path, sizeof(path));
        sprintf(path, "provenance.%d.log", i);
        i++;
      } while (access(path, R_OK)==0);
      fprintf(stderr, "Provenance log file: %s\n", path);
      CDE_provenance_logfile = fopen(path, "w");
    }

    pthread_mutex_init(&mut_pidlist, NULL);
    pidlist.pc = 0;
    pthread_create( &ptid, NULL, capture_cont_prov, &pidlist);
  }
}

void add_pid_prov(pid_t pid) {
  pthread_mutex_lock(&mut_pidlist);
  pidlist.pv[pidlist.pc] = pid;
  pidlist.pc++;
  pthread_mutex_unlock(&mut_pidlist);
  print_curr_prov(&pidlist);
}

void rm_pid_prov(pid_t pid) {
  int i=0;
  assert(pidlist.pc>0);
  print_curr_prov(&pidlist);
  pthread_mutex_lock(&mut_pidlist);
  while (pidlist.pv[i] != pid && i < pidlist.pc) i++;
  if (i < pidlist.pc) {
    pidlist.pv[i] = pidlist.pv[pidlist.pc-1];
    pidlist.pc--;
  }
  pthread_mutex_unlock(&mut_pidlist);
}

