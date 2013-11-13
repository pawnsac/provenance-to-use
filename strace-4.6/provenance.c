#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/param.h>

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
extern char CDE_verbose_mode;
extern char cde_pseudo_pkg_dir[MAXPATHLEN];

char CDE_provenance_mode = 0;
char CDE_bare_run = 0;
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

void add_pid_prov(pid_t pid);

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
    fprintf(CDE_provenance_logfile, "%d %d EXECVE %u %s %s ", (int)time(0), parentPid, tcp->pid, filename_abspath, tcp->current_dir);
    print_arg_prov(CDE_provenance_logfile, tcp, tcp->u_arg[1]);
    if (CDE_verbose_mode) {
      vbprintf("[%d-prov] BEGIN %s '%s'\n", tcp->pid, "execve", opened_filename);
    }
    free(filename_abspath);
    free(opened_filename);
  }
}

void print_execdone_prov(struct tcb *tcp) {
  if (CDE_provenance_mode) {
    int ppid = -1;
    if (tcp->parent) ppid = tcp->parent->pid;
    fprintf(CDE_provenance_logfile, "%d %u EXECVE2 %d\n", (int)time(0), tcp->pid, ppid);
    add_pid_prov(tcp->pid);
    if (CDE_verbose_mode) {
      vbprintf("[%d-prov] BEGIN %s '%s'\n", tcp->pid, "execve2");
    }
  }
}

void print_IO_prov(struct tcb *tcp, char* filename, const char* syscall_name) {
  if (CDE_provenance_mode) {
    // only track open syscalls
    if ((tcp->u_rval >= 0) &&
        strcmp(syscall_name, "sys_open") == 0) {
      char* filename_abspath = canonicalize_path(filename, tcp->current_dir);
      assert(filename_abspath);

      // Note: tcp->u_arg[1] is only for open(), not openat()
      unsigned char open_mode = (tcp->u_arg[1] & 3);
      if (open_mode == O_RDONLY) {
        fprintf(CDE_provenance_logfile, "%d %u READ %s\n", (int)time(0), tcp->pid, filename_abspath);
      }
      else if (open_mode == O_WRONLY) {
        fprintf(CDE_provenance_logfile, "%d %u WRITE %s\n", (int)time(0), tcp->pid, filename_abspath);
      }
      else if (open_mode == O_RDWR) {
        fprintf(CDE_provenance_logfile, "%d %u READ-WRITE %s\n", (int)time(0), tcp->pid, filename_abspath);
      }
      else {
        fprintf(CDE_provenance_logfile, "%d %u UNKNOWNIO %s\n", (int)time(0), tcp->pid, filename_abspath);
      }

      free(filename_abspath);
    }
  }
}

void print_spawn_prov(struct tcb *tcp) {
  if (CDE_provenance_mode) {
    fprintf(CDE_provenance_logfile, "%d %u SPAWN %u\n", (int)time(0), tcp->parent->pid, tcp->pid);
  }
}

void print_act_prov(struct tcb *tcp, const char* action) {
  if (CDE_provenance_mode) {
    fprintf(CDE_provenance_logfile, "%d %u %s 0\n", (int)time(0), tcp->pid, action);
  }
}

void print_sock_prov(struct tcb *tcp, const char *op, unsigned int port, unsigned long ipv4) {
  print_newsock_prov(tcp, op, 0, 0, port, ipv4, 0);
}
void print_newsock_prov(struct tcb *tcp, const char* op, \
  unsigned int s_port, unsigned long s_ipv4, \
  unsigned int d_port, unsigned long d_ipv4, int sk) {
  struct in_addr s_in, d_in;
  char saddr[32], daddr[32];
  s_in.s_addr = s_ipv4;
  strcpy(saddr, inet_ntoa(s_in));
  d_in.s_addr = d_ipv4;
  strcpy(daddr, inet_ntoa(d_in));
  if (CDE_provenance_mode) {
    fprintf(CDE_provenance_logfile, "%d %u %s %u %s %u %s %d\n", (int)time(0), tcp->pid, \
        op, s_port, saddr, d_port, daddr, sk);
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
    if (f==NULL) { // remove this invalid pid
      fprintf(CDE_provenance_logfile, "%d %u LEXIT\n", curr_time, pidlist_p->pv[i]); // lost_pid exit
      pidlist_p->pv[i] = pidlist_p->pv[pidlist_p->pc-1];
      pidlist_p->pc--;
      continue;
    }
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
  char* env_prov_mode = getenv("IN_CDE_PROVENANCE_MODE");
  char path[255];
  if (env_prov_mode != NULL)
    CDE_provenance_mode = (strcmp(env_prov_mode, "1") == 0) ? 1 : 0;
  else
    CDE_provenance_mode = !CDE_exec_mode;
  if (CDE_provenance_mode) {
    pthread_mutex_init(&mut_logfile, NULL);
    // create NEW provenance log file
    bzero(path, sizeof(path));
    sprintf(path, "%s/provenance.log", cde_pseudo_pkg_dir);
    if (access(path, R_OK)==-1)
      CDE_provenance_logfile = fopen(path, "w");
    else {
      int i=1;
      // check through provenance.$i.log to find a new file name
      do {
        bzero(path, sizeof(path));
        sprintf(path, "%s/provenance.%d.log", cde_pseudo_pkg_dir, i);
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

