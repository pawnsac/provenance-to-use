#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <ctype.h>
#include <pwd.h>

#include "defs.h"
#include "provenance.h"
#include "cdenet.h"
#include "../leveldb-1.14.0/include/leveldb/c.h"

/* macros */
#ifndef MAX
#define MAX(a,b)		(((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)		(((a) < (b)) ? (a) : (b))
#endif

extern char* strcpy_from_child(struct tcb* tcp, long addr);
extern void vbprintf(const char *fmt, ...);
void rstrip(char *s);

extern char CDE_exec_mode;
extern char CDE_verbose_mode;
extern char CDE_nw_mode;
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

// provenance leveldb
lvldb_t *provdb;

// current execution db
// import to use in exec capture
extern lvldb_t *currdb;

typedef struct socketdata {
  unsigned short saf;
  unsigned int port;
  union ipdata {
    unsigned long ipv4;
    unsigned char ipv6[16];   /* IPv6 address */
  } ip;
} socketdata_t;

extern int string_quote(const char *instr, char *outstr, int len, int size);
extern char* strcpy_from_child_or_null(struct tcb* tcp, long addr);
extern char* canonicalize_path(char *path, char *relpath_base);

void init_prov();

void print_IO_prov(struct tcb *tcp, char* filename, const char* syscall_name);
void print_spawn_prov(struct tcb *tcp);
void print_sock_prov(struct tcb *tcp, int action, unsigned int port, unsigned long ipv4);
void print_act_prov(struct tcb *tcp, const char* action);
void print_newsock_prov(struct tcb *tcp, int action, \
  unsigned int s_port, unsigned long s_ipv4, \
  unsigned int d_port, unsigned long d_ipv4, int sk);

void rm_pid_prov(pid_t pid);

void db_write(lvldb_t *mydb, const char *key, const char *value);
void db_write_fmt(lvldb_t *mydb, const char *key, const char *fmt, ...);
void db_write_io_prov(lvldb_t *mydb, long pid, int prv, const char *filename_abspath);
void db_write_exec_prov(lvldb_t *mydb, long ppid, long pid, const char *filename_abspath, \
    char *current_dir, char *args);
void db_write_execdone_prov(lvldb_t *mydb, long ppid, long pid);
void db_write_lexit_prov(lvldb_t *mydb, long pid);
void db_write_spawn_prov(lvldb_t *mydb, long ppid, long pid);
void db_write_prov_stat(lvldb_t *mydb, long pid, const char* label, char *stat);
void db_write_sock_action(lvldb_t *mydb, long pid, int sockfd, \
    const char *buf, size_t len_param, int flags, \
    size_t len_result, int action);
void db_write_connect_prov(lvldb_t *mydb, long pid, 
    int sockfd, char* addr, int addr_len, long u_rval);
void db_write_listen_prov(lvldb_t *mydb, int pid, 
    int sock, int backlog, int result);
void db_setupListenCounter(lvldb_t *mydb, char* pidkey);
void db_setupConnectCounter(lvldb_t *mydb, char* pidkey);

void db_write_root(lvldb_t *mydb);
    
ull_t getusec();

void add_pid_prov(pid_t pid);

/*
 * Print string specified by address `addr' and length `len'.
 * If `len' < 0, treat the string as a NUL-terminated string.
 * If string length exceeds `max_strlen', append `...' to the output.
 */
int
get_str_prov(char *dest, struct tcb *tcp, long addr, int len)
{
	static char *str = NULL;
	static char *outstr;
	int size;
  int len_written = 0;

	if (!addr) {
		len_written += sprintf(dest+len_written, "NULL");
		return len_written;
	}
	/* Allocate static buffers if they are not allocated yet. */
	if (!str)
		str = malloc(max_strlen + 1);
	if (!outstr)
		outstr = malloc(4 * max_strlen + sizeof "\"...\"");
	if (!str || !outstr) {
		fprintf(stderr, "out of memory\n");
		len_written += sprintf(dest+len_written, "%#lx", addr);
		return len_written;
	}

	if (len < 0) {
		/*
		 * Treat as a NUL-terminated string: fetch one byte more
		 * because string_quote() quotes one byte less.
		 */
		size = max_strlen + 1;
		str[max_strlen] = '\0';
		if (umovestr(tcp, addr, size, str) < 0) {
			len_written += sprintf(dest+len_written, "%#lx", addr);
			return len_written;
		}
	}
	else {
		size = MIN(len, max_strlen);
		if (umoven(tcp, addr, size, str) < 0) {
			len_written += sprintf(dest+len_written, "%#lx", addr);
			return len_written;
		}
	}

	if (string_quote(str, outstr, len, size) &&
	    (len < 0 || len > max_strlen))
		strcat(outstr, "...");

	len_written += sprintf(dest+len_written, "%s", outstr);
  return len_written;
}

void
print_arg_prov(char *argstr, struct tcb *tcp, long addr)
{
	union {
		unsigned int p32;
		unsigned long p64;
		char data[sizeof(long)];
	} cp;
	const char *sep;
	int n = 0;
  unsigned int len = 0;

  len += sprintf(argstr+len, "[");
	cp.p64 = 1;
	for (sep = ""; !abbrev(tcp) || n < max_strlen / 2; sep = ", ", ++n) {
		if (umoven(tcp, addr, personality_wordsize[current_personality],
			   cp.data) < 0) {
			len += sprintf(argstr+len, "%#lx\n", addr);
			return;
		}
		if (personality_wordsize[current_personality] == 4)
			cp.p64 = cp.p32;
		if (cp.p64 == 0)
			break;
		len += sprintf(argstr+len, "%s", sep);
		len += get_str_prov(argstr+len, tcp, cp.p64, -1);
		addr += personality_wordsize[current_personality];
	}
	if (cp.p64)
		len += sprintf(argstr+len, "%s...", sep);

	len += sprintf(argstr+len, "]");
}


void print_exec_prov(struct tcb *tcp) {
  if (CDE_provenance_mode || CDE_nw_mode) {
    char *opened_filename = strcpy_from_child_or_null(tcp, tcp->u_arg[0]);
    char *filename_abspath = canonicalize_path(opened_filename, tcp->current_dir);
    assert(filename_abspath);
    int parentPid = tcp->parent == NULL ? getpid() : tcp->parent->pid;
    char args[KEYLEN*10];
    print_arg_prov(args, tcp, tcp->u_arg[1]);
    if (CDE_provenance_mode) {
      fprintf(CDE_provenance_logfile, "%d %d EXECVE %u %s %s %s\n", (int)time(0),
        parentPid, tcp->pid, filename_abspath, tcp->current_dir, args);
      db_write_exec_prov(provdb, parentPid, tcp->pid, filename_abspath, tcp->current_dir, args);
      if (CDE_verbose_mode) {
        vbprintf("[%d-prov] BEGIN %s '%s'\n", tcp->pid, "execve", opened_filename);
      }
    }
    if (CDE_nw_mode) {
      db_write_exec_prov(currdb, parentPid, tcp->pid, filename_abspath, tcp->current_dir, args);
    }
    free(filename_abspath);
    free(opened_filename);
  }
}

void print_execdone_prov(struct tcb *tcp) {
  if (CDE_provenance_mode || CDE_nw_mode) {
    int ppid = -1;
    if (tcp->parent) ppid = tcp->parent->pid;
    if (CDE_provenance_mode) {
      db_write_execdone_prov(provdb, ppid, tcp->pid);
      fprintf(CDE_provenance_logfile, "%d %u EXECVE2 %d\n", (int)time(0), tcp->pid, ppid);
      add_pid_prov(tcp->pid);
      if (CDE_verbose_mode) {
        vbprintf("[%d-prov] BEGIN execve2\n", tcp->pid);
      }
    }
    if (CDE_nw_mode) {
      db_write_execdone_prov(currdb, ppid, tcp->pid);
    }
  }
}

//~ void print_file_prov(int sec, unsigned int pid, int action, char *path) {
  //~ fprintf(CDE_provenance_logfile, "%d %u %s %s\n", sec, pid,
      //~ (action == PRV_RDONLY ? "READ" : (
        //~ action == PRV_WRONLY ? "WRITE" : (
        //~ action == PRV_RDWR ? "READ-WRITE" : "UNKNOWNIO"))),
      //~ path);
  //~ db_write_io_prov(pid, action, path);
//~ }

void print_io_prov(struct tcb *tcp, int pos, int action) {
  char *filename = strcpy_from_child_or_null(tcp, tcp->u_arg[pos]);
  char *filename_abspath = canonicalize_path(filename, tcp->current_dir);
  assert(filename_abspath);

  fprintf(CDE_provenance_logfile, "%d %u %s %s\n", (int)time(0), tcp->pid,
      (action == PRV_RDONLY ? "READ" : (
        action == PRV_WRONLY ? "WRITE" : (
        action == PRV_RDWR ? "READ-WRITE" : "UNKNOWNIO"))),
      filename_abspath);
  db_write_io_prov(provdb, tcp->pid, action, filename_abspath);

  free(filename);
  free(filename_abspath);
}

void print_syscall_read_prov(struct tcb *tcp, const char *syscall_name, int pos) {
  if (CDE_provenance_mode && tcp->u_rval >= 0) {
    print_io_prov(tcp, pos, PRV_RDONLY);
  }
}

void print_syscall_write_prov(struct tcb *tcp, const char *syscall_name, int pos) {
  if (CDE_provenance_mode && tcp->u_rval >= 0) {
    print_io_prov(tcp, pos, PRV_WRONLY);
  }
}

// assume openAT use the same current_dir with PWD (from CDE code)
void print_open_prov(struct tcb *tcp, const char *syscall_name) {
  if (CDE_provenance_mode) {
    int pos = strcmp(syscall_name, "sys_open") == 0 ? 1 :
        (strcmp(syscall_name, "sys_openat") == 0 ? 2 : 0);

    // track open, rename syscalls
    if (tcp->u_rval >= 0 && pos > 0) {

      // Note: tcp->u_arg[1] is only for open(), tcp->u_arg[2] for openat()
      unsigned char open_mode = (tcp->u_arg[pos] & 3);
      int action;
      switch (open_mode) {
        case O_RDONLY: action = PRV_RDONLY; break;
        case O_WRONLY: action = PRV_WRONLY; break;
        case O_RDWR: action = PRV_RDWR; break;
        default: action = PRV_UNKNOWNIO; break;
      }
      print_io_prov(tcp, pos - 1, action);
      //~ print_file_prov((int)time(0), tcp->pid, PRV_RDONLY, filename_abspath);
      //~ print_file_prov((int)time(0), tcp->pid, PRV_WRONLY, filename_abspath);
      //~ print_file_prov((int)time(0), tcp->pid, PRV_RDWR, filename_abspath);
      //~ print_file_prov((int)time(0), tcp->pid, PRV_UNKNOWNIO, filename_abspath);
    }
  }
}

// TODO: think what to do with
//    int chmod(const char* path, mode_t mod);
//    int chown(const char* path, uid_t owner, gid_t grp); --> write META data of a file
//    int utimes(const char* path, const struct timeval* times);
//    int lutimes(const char* path, const struct timeval* times); --> read META data of a file

// assume renameAT use the same current_dir with PWD (from CDE code)
void print_rename_prov(struct tcb *tcp, int renameat) {
  if (CDE_provenance_mode) {
    if (tcp->u_rval == 0) {
      int posread = renameat == 0 ? 0 : 1;
      int poswrite = renameat == 0 ? 1 : 3;
      print_io_prov(tcp, posread, PRV_RDWR);
      print_io_prov(tcp, poswrite, PRV_WRONLY);
    }
  }
}

void print_syscall_two_prov(struct tcb *tcp, const char *syscall_name, int posread, int poswrite) {
  //vbprintf("[%d-prov] print_syscall_two_prov: %s %d\n", tcp->pid, syscall_name, tcp->u_rval);
  if (CDE_provenance_mode) {
    if (tcp->u_rval == 0) {
      print_io_prov(tcp, posread, PRV_RDONLY);
      print_io_prov(tcp, poswrite, PRV_WRONLY);
      //~ int pos = -1, file1_action = PRV_RDONLY;
      //~ if (strcmp(syscall_name, "sys_rename") == 0) {
        //~ pos = 0;
        //~ file1_action = PRV_RDWR;
      //~ } else if (strcmp(syscall_name, "sys_link") == 0) {
        //~ pos = 0;
      //~ } else if (strcmp(syscall_name, "sys_symlink") == 0) {
        //~ pos = 0;
      //~ } else if (strcmp(syscall_name, "sys_renameat") == 0) {
        //~ pos = 1;
        //~ file1_action = PRV_RDWR;
      //~ } else if (strcmp(syscall_name, "sys_linkat") == 0) {
        //~ pos = 1;
      //~ } else if (strcmp(syscall_name, "sys_symlinkat") == 0) {
        //~ pos = 1;
      //~ }
      //~ print_io_prov(tcp, pos, file1_action);
      //~ print_io_prov(tcp, pos+pos+1, PRV_WRONLY);
    }
  }
}

void print_spawn_prov(struct tcb *tcp) {
  if (CDE_provenance_mode) {
    fprintf(CDE_provenance_logfile, "%d %u SPAWN %u\n", (int)time(0), tcp->parent->pid, tcp->pid);
    db_write_spawn_prov(provdb, tcp->parent->pid, tcp->pid);
  }
}

void print_act_prov(struct tcb *tcp, const char *action) {
  if (CDE_provenance_mode) {
    fprintf(CDE_provenance_logfile, "%d %u %s 0\n", (int)time(0), tcp->pid, action);
    db_write_io_prov(provdb, tcp->pid, PRV_ACTION, action);
  }
}

void print_sock_prov(struct tcb *tcp, int action, unsigned int port, unsigned long ipv4) {
  print_newsock_prov(tcp, action, 0, 0, port, ipv4, 0);
}
void print_newsock_prov(struct tcb *tcp, int action, \
    unsigned int s_port, unsigned long s_ipv4, \
    unsigned int d_port, unsigned long d_ipv4, int sk) {
  struct in_addr s_in, d_in;
  char saddr[32], daddr[32];
  if (CDE_provenance_mode) {
    s_in.s_addr = s_ipv4;
    strcpy(saddr, inet_ntoa(s_in));
    d_in.s_addr = d_ipv4;
    strcpy(daddr, inet_ntoa(d_in));
    fprintf(CDE_provenance_logfile, "%d %u %d %u %s %u %s %d\n", (int)time(0), tcp->pid, \
        action, s_port, saddr, d_port, daddr, sk);
    printf("%d %u %d %u %s %u %s %d\n", (int)time(0), tcp->pid, \
        action, s_port, saddr, d_port, daddr, sk);
    //db_write_connect_prov(tcp->pid, action, filename_abspath);
  }
}

void print_connect_prov(struct tcb *tcp, 
    int sockfd, char* addr, int addr_len, long u_rval) {
  if (CDE_provenance_mode) {
    db_write_connect_prov(provdb, tcp->pid, sockfd, addr, addr_len, u_rval);
    socketdata_t sock;
    if (getsockinfo(tcp, tcp->u_arg[1], tcp->u_arg[2], &sock)>=0) {
      struct in_addr d_in;
      char daddr[32];
      d_in.s_addr = sock.ip.ipv4;
      strcpy(daddr, inet_ntoa(d_in));
      fprintf(CDE_provenance_logfile, "%d %u SOCK_CONNECT %u %ld %d\n", (int)time(0), tcp->pid, \
          sock.port, (long) daddr, sockfd);
    }
  }
}

void print_sock_action(struct tcb *tcp, int sockfd, \
                       const char *buf, size_t len_param, int flags, \
                       size_t len_result, int action) {
  if (0) {
    int i;
    printf("sock %d action %d size %ld res %ld: '", \
           sockfd, action, len_param, len_result);
    for (i=0; i<len_result; i++) {
      printf("%c", buf[i]);
    }
    printf("'\n");
  }
  fprintf(CDE_provenance_logfile, "%d %u SOCK %d %zu %d %zu %d\n", (int)time(0), tcp->pid, \
        sockfd, len_param, flags, len_result, action);
  db_write_sock_action(provdb, tcp->pid, sockfd, buf, len_param, flags, \
                       len_result, action);
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
      db_write_lexit_prov(provdb, pidlist_p->pv[i]);
      pidlist_p->pv[i] = pidlist_p->pv[pidlist_p->pc-1];
      pidlist_p->pc--;
      continue;
    }
    if (fgets(buff, 1024, f) == NULL)
      rss= 0;
    else
      // details of format: http://git.kernel.org/?p=linux/kernel/git/stable/linux-stable.git;a=blob_plain;f=fs/proc/array.c;hb=d1c3ed669a2d452cacfb48c2d171a1f364dae2ed
      sscanf(buff, "%*d %*s %*c %*d %*d %*d %*d %*d %*lu %*lu \
%*lu %*lu %*lu %*lu %*lu %*ld %*ld %*ld %*ld %*ld %*ld %*lu %lu ", &rss);
    fclose(f);
    fprintf(CDE_provenance_logfile, "%d %u MEM %lu\n", curr_time, pidlist_p->pv[i], rss);
    db_write_prov_stat(provdb, pidlist_p->pv[i], "stat", buff);
    sprintf(buff, "/proc/%d/io", pidlist_p->pv[i]);
    f = fopen(buff, "r");
    if (f==NULL) continue; 
    
    int iostat = fread(buff, 1, 1024, f);
    if (iostat > 0) {
      buff[iostat] = 0;
      db_write_prov_stat(provdb, pidlist_p->pv[i], "iostat", buff);
    }
    fclose(f);
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

	if (CDE_provenance_logfile) {
    vbprintf("=== Close log file and provenance db ===\n");
  	fclose(CDE_provenance_logfile);
    leveldb_close(provdb->db);
  }
  pthread_mutex_destroy(&mut_logfile);

  return NULL;
}

void init_prov() {
  pthread_t ptid;
  char *env_prov_mode = getenv("IN_CDE_PROVENANCE_MODE");
  char path[PATH_MAX];
  int subns=1;
  char *err = NULL;

  if (env_prov_mode != NULL)
    CDE_provenance_mode = (strcmp(env_prov_mode, "1") == 0) ? 1 : 0;
  else
    CDE_provenance_mode = !CDE_exec_mode;
  if (CDE_provenance_mode) {
    setenv("IN_CDE_PROVENANCE_MODE", "1", 1);
    pthread_mutex_init(&mut_logfile, NULL);
    // create NEW provenance log file
    bzero(path, sizeof(path));
    sprintf(path, "%s/provenance.%s.1.log", cde_pseudo_pkg_dir, CDE_ROOT_NAME);
    if (access(path, R_OK)==-1)
      CDE_provenance_logfile = fopen(path, "w");
    else {
      // check through provenance.$subns.log to find a new file name
      do {
        subns++;
        bzero(path, sizeof(path));
        sprintf(path, "%s/provenance.%s.%d.log", cde_pseudo_pkg_dir, CDE_ROOT_NAME, subns);
      } while (access(path, R_OK)==0);
      fprintf(stderr, "Provenance log file: %s\n", path);
      CDE_provenance_logfile = fopen(path, "w");
    }

    // leveldb initialization
    sprintf(path+strlen(path), "_db");
    fprintf(stderr, "Provenance db: %s\n", path);
    provdb = malloc(sizeof(lvldb_t));
    provdb->options = leveldb_options_create();
    leveldb_options_set_create_if_missing(provdb->options, 1);
    provdb->db = leveldb_open(provdb->options, path, &err);
    if (err != NULL || provdb->db == NULL) {
      fprintf(stderr, "Leveldb open fail!\n");
      exit(-1);
    }
    assert(provdb->db!=NULL);
    provdb->woptions = leveldb_writeoptions_create();
    provdb->roptions = leveldb_readoptions_create();
    /* reset error var */
    leveldb_free(err); err = NULL;

    struct passwd *pw = getpwuid(getuid()); // don't free this pointer
    FILE *fp;
    char uname[PATH_MAX];
    fp = popen("uname -a", "r");
    if (fgets(uname, PATH_MAX, fp) == NULL)
      sprintf(uname, "(unknown architecture)");
    pclose(fp);
    rstrip(uname);
    char fullns[PATH_MAX];
    sprintf(fullns, "%s.%d", CDE_ROOT_NAME, subns);
    fprintf(CDE_provenance_logfile, "# @agent: %s\n", pw == NULL ? "(noone)" : pw->pw_name);
    fprintf(CDE_provenance_logfile, "# @machine: %s\n", uname);
    fprintf(CDE_provenance_logfile, "# @namespace: %s\n", CDE_ROOT_NAME);
    fprintf(CDE_provenance_logfile, "# @subns: %d\n", subns);
    fprintf(CDE_provenance_logfile, "# @fullns: %s\n", fullns);
    fprintf(CDE_provenance_logfile, "# @parentns: %s\n", getenv("CDE_PROV_NAMESPACE"));

    // provenance meta data in lvdb
    db_write(provdb, "meta.agent", pw == NULL ? "(noone)" : pw->pw_name);
    db_write(provdb, "meta.machine", uname);
    db_write(provdb, "meta.namespace", CDE_ROOT_NAME);
    db_write_fmt(provdb, "meta.subns", "%d", subns);
    db_write(provdb, "meta.fullns", fullns);
    db_write(provdb, "meta.parentns", getenv("CDE_PROV_NAMESPACE"));

    db_write_root(provdb);
    setenv("CDE_PROV_NAMESPACE", fullns, 1);

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

void rstrip(char *s) {
  size_t size;
  char *end;

  size = strlen(s);

  if (!size)
    return;

  end = s + size - 1;
  while (end >= s && isspace(*end))
    end--;
  *(end + 1) = '\0';

}

void db_nwrite(lvldb_t *mydb, const char *key, const char *value, int len) {
  char *err = NULL;
  assert(mydb->db!=NULL);

  if (value == NULL)
    len = 0;
  else if (len < 0)
    len = strlen(value);
  leveldb_put(mydb->db, mydb->woptions, key, strlen(key), value, len, &err);

  if (err != NULL) {
    vbprintf("DB - Write FAILED: '%s' -> '%s'\n", key, value);
    print_trace();
  }

  leveldb_free(err); err = NULL;
}

char* db_nread(lvldb_t *mydb, const char *key, size_t *plen) {
  char *err = NULL;
  assert(mydb->db!=NULL);

  char* res = leveldb_get(mydb->db, mydb->roptions, key, strlen(key), plen, &err);

  if (err != NULL) {
    vbprintf("DB - Read FAILED: '%s'\n", key);
    print_trace();
  }

  leveldb_free(err); err = NULL;
  return res;
}

void db_write(lvldb_t *mydb, const char *key, const char *value) {
  db_nwrite(mydb, key, value, -1);
}

//~ void mydb_write_fmt(leveldb_t *mydb, leveldb_writeoptions_t *mywoptions, 
    //~ const char *key, const char *fmt, ...) {
  //~ char val[KEYLEN];
  //~ va_list args;
	//~ va_start(args, fmt);
  //~ vsprintf(val, fmt, args);
	//~ va_end(args);
  //~ db_write(mydb, mywoptions, key, val);
//~ }

char* db_readc(lvldb_t *mydb, const char *key) {
  char *err = NULL;
  char *read;
  size_t read_len;

  read = leveldb_get(mydb->db, mydb->roptions, key, strlen(key), &read_len, &err);

  if (err != NULL) {
    vbprintf("DB - Read FAILED: '%s'\n", key);
    print_trace();
    return NULL;
  }
  read = realloc(read, read_len+1);
  read[read_len] = 0;

  leveldb_free(err); err = NULL;

  return read;
}

void db_read_ull(lvldb_t *mydb, const char *key, ull_t* pvalue) {
  char *err = NULL;
  ull_t *read;
  size_t read_len;

  read = (ull_t*) leveldb_get(mydb->db, mydb->roptions, key, strlen(key), &read_len, &err);

  if (err != NULL || read == NULL) {
    vbprintf("DB - Read FAILED: '%s'\n", key);
    print_trace();
    return;
  }

  leveldb_free(err); err = NULL;

  *pvalue = *read;
  free(read);
}

// the initial PTU pid node
void db_write_root(lvldb_t *mydb) {
  
  long pid = getpid();
  char key[KEYLEN], pidkey[KEYLEN];
  ull_t usec = getusec();

  sprintf(key, "pid.%ld", pid);
  sprintf(pidkey, "%ld.%llu", pid, usec);
  db_write(mydb, key, pidkey);

  sprintf(pidkey, "%ld.%llu", pid, usec);
  db_write(mydb, "meta.root", pidkey);
}

ull_t db_getCounterInc(lvldb_t *mydb, char* key) {
  ull_t read;
  db_read_ull(mydb, key, &read);
  if (CDE_verbose_mode) {
    vbprintf("[xxxx-prov] db_getCounterInc %s -> %llu\n", key, read);
  }
  read++;
  db_nwrite(mydb, key, (char*)&read, sizeof(ull_t));
  return read - 1;
}

char* db_read_pid_key(lvldb_t *mydb, long pid) {
  char key[KEYLEN];
  sprintf(key, "pid.%ld", pid);
  return db_readc(mydb, key);
}

void db_write_fmt(lvldb_t *mydb, const char *key, const char *fmt, ...) {
  char val[KEYLEN];
  va_list args;
	va_start(args, fmt);
  vsprintf(val, fmt, args);
	va_end(args);
  db_write(mydb, key, val);
}

ull_t getusec() {
  struct timeval tv;
  ull_t usec;
  if (gettimeofday(&tv, NULL) != 0) {
    usec = -1;
  }
  usec = (ull_t) tv.tv_sec * 1000000 + tv.tv_usec;
  return usec;
}

void db_write_io_prov(lvldb_t *mydb, long pid, int action, const char *filename_abspath) {
  char key[KEYLEN];
  char *pidkey=db_read_pid_key(mydb, pid);
  if (pidkey == NULL) return;
  ull_t usec = getusec();

  sprintf(key, "prv.iopid.%s.%d.%llu", pidkey, action, usec);
  db_write(mydb, key, filename_abspath);

  sprintf(key, "prv.iofile.%s.%s.%llu", filename_abspath, pidkey, usec);
  db_write_fmt(mydb, key, "%d", action);

  free(pidkey);
}

char* db_create_pid(lvldb_t *mydb, long pid, ull_t usec, char* ppidkey) {
  char key[KEYLEN];
  char *pidkey = malloc(KEYLEN);

  sprintf(key, "pid.%ld", pid);
  sprintf(pidkey, "%ld.%llu", pid, usec);
  db_write(mydb, key, pidkey);

  sprintf(key, "prv.pid.%s.parent", pidkey);
  db_write(mydb, key, ppidkey);

  return pidkey;
}

void db_write_exec_prov(lvldb_t *mydb, long ppid, long pid, const char *filename_abspath, char *current_dir, char *args) {
  char key[KEYLEN];
  char *ppidkey=db_read_pid_key(mydb, ppid);
  if (ppidkey == NULL) return;
  ull_t usec = getusec();
  char *pidkey;

  pidkey = db_create_pid(mydb, pid, usec, ppidkey);

  // new execve item
  sprintf(key, "prv.pid.%s.exec.%llu", ppidkey, usec);
  db_write(mydb, key, pidkey);

  // info on new pidkey
  sprintf(key, "prv.pid.%s.path", pidkey);
  db_write(mydb, key, filename_abspath);
  sprintf(key, "prv.pid.%s.pwd", pidkey);
  db_write(mydb, key, current_dir);
  sprintf(key, "prv.pid.%s.args", pidkey);
  db_write(mydb, key, args);
  sprintf(key, "prv.pid.%s.start", pidkey);
  db_write_fmt(mydb, key, "%llu", usec);

  free(pidkey);
  free(ppidkey);
}

void db_write_execdone_prov(lvldb_t *mydb, long ppid, long pid) {
  char key[KEYLEN], value[KEYLEN];
  char *pidkey=db_read_pid_key(mydb, pid);
  if (pidkey == NULL) return;
  ull_t usec = getusec();

  // create (successful) exec relation
  sprintf(key, "prv.pid.%s.ok", pidkey);
  sprintf(value, "%llu", usec);
  db_write(mydb, key, value);
  
  db_setupConnectCounter(mydb, pidkey);
  db_setupListenCounter(mydb, pidkey);

  free(pidkey);
}

void db_write_lexit_prov(lvldb_t *mydb, long pid) {
  char key[KEYLEN], value[KEYLEN];
  char *pidkey=db_read_pid_key(mydb, pid);
  if (pidkey == NULL) return;
  ull_t usec = getusec();

  // create (successful) exec relation
  sprintf(key, "prv.pid.%s.lexit", pidkey);
  sprintf(value, "%llu", usec);
  db_write(mydb, key, value);

  free(pidkey);
}

void db_write_spawn_prov(lvldb_t *mydb, long ppid, long pid) {
  char key[KEYLEN];
  char *ppidkey=db_read_pid_key(mydb, ppid);
  if (ppidkey == NULL) return;
  ull_t usec = getusec();

  char *pidkey = db_create_pid(mydb, pid, usec, ppidkey);

  sprintf(key, "prv.pid.%s.spawn.%llu", ppidkey, usec);
  db_write(mydb, key, pidkey);

  free(pidkey);
  free(ppidkey);
}

void db_write_prov_stat(lvldb_t *mydb, long pid,  const char* label,char *stat) {
  char key[KEYLEN];
  char *pidkey = db_read_pid_key(mydb, pid);
  if (pidkey == NULL) return;
  ull_t usec = getusec();

  sprintf(key, "prv.pid.%s.%s.%llu", pidkey, label, usec);
  db_write(mydb, key, stat);

  free(pidkey);
}

/* =====
 * read/write socket
 */
// setup

void _db_setupSockCounter(lvldb_t *mydb, char *pidkey, int sockfd, char* ch_sockid) {
  char key[KEYLEN];
  ull_t zero = 0;
  // prv.pid.$(pid.usec).skid.$sockid.act.$action -> $n
  sprintf(key, "prv.pid.%s.skid.%s.act.%d", pidkey, ch_sockid, SOCK_SEND);
  db_nwrite(mydb, key, (char*) &zero, sizeof(ull_t));
  sprintf(key, "prv.pid.%s.skid.%s.act.%d", pidkey, ch_sockid, SOCK_RECV);
  db_nwrite(mydb, key, (char*) &zero, sizeof(ull_t));
}
void db_setupSockAcceptCounter(lvldb_t *mydb, char *pidkey, int sockfd, ull_t listenid, ull_t acceptid) {
  char ch_sockid[KEYLEN];
  sprintf(ch_sockid, "%llu_%llu", listenid, acceptid);
  _db_setupSockCounter(mydb, pidkey, sockfd, ch_sockid);
}
void db_setupSockConnectCounter(lvldb_t *mydb, char *pidkey, int sockfd, ull_t sockid) {
  char ch_sockid[KEYLEN];
  sprintf(ch_sockid, "%llu", sockid);
  _db_setupSockCounter(mydb, pidkey, sockfd, ch_sockid);
}

void _db_setSockId(lvldb_t *mydb, char* pidkey, int sock, char* ch_sockid) {
  char key[KEYLEN];
  sprintf(key, "pid.%s.sk2id.%d", pidkey, sock);
  if (CDE_verbose_mode) {
    vbprintf("[xxxx] setSockId pidkey %s, sock %d -> sockid %s\n", 
        pidkey, sock, ch_sockid);
  }
  db_nwrite(mydb, key, ch_sockid, sizeof(ch_sockid));
}
void db_setSockConnectId(lvldb_t *mydb, char* pidkey, int sock, ull_t sockid) {
  char ch_sockid[KEYLEN];
  sprintf(ch_sockid, "%llu", sockid);
  _db_setSockId(mydb, pidkey, sock, ch_sockid);
}
void db_setSockAcceptId(lvldb_t *mydb, char* pidkey, int sock, ull_t listenid, ull_t acceptid) {
  char ch_sockid[KEYLEN];
  sprintf(ch_sockid, "%llu_%llu", listenid, acceptid);
  _db_setSockId(mydb, pidkey, sock, ch_sockid);
}

// action

char* db_getSockId(lvldb_t *mydb, char* pidkey, int sock) {
  char key[KEYLEN];
  sprintf(key, "pid.%s.sk2id.%d", pidkey, sock);
  char* sockid = db_readc(mydb, key);
  if (CDE_verbose_mode) {
    vbprintf("[xxx] getSockId pidkey %s, sock %d -> sockid %llu\n", 
        pidkey, sock, sockid);
  }
  return sockid;
}

ull_t db_getPkgCounterInc(lvldb_t *mydb, char* pidkey, char* sockid, int action) {
  char key[KEYLEN];
  sprintf(key, "prv.pid.%s.skid.%s.act.%d", pidkey, sockid, action);
  return db_getCounterInc(mydb, key);
}

void db_write_sock_action(lvldb_t *mydb, long pid, int sockfd, \
                       const char *buf, size_t len_param, int flags, \
                       size_t len_result, int action) {
  char key[KEYLEN];
  char* pidkey = db_read_pid_key(mydb, pid);
  char* sockid = db_getSockId(mydb, pidkey, sockfd);
  ull_t pkgid = db_getPkgCounterInc(mydb, pidkey, sockid, action);
  if (pidkey == NULL) return;
  ull_t usec = getusec();

  sprintf(key, "prv.pid.%s.sock.%llu.%d.%d.%ld.%d.%ld", \
          pidkey, usec, action, sockfd, len_param, flags, len_result);
  db_nwrite(mydb, key, buf, len_result);
  sprintf(key, "prv.sock.%s.action.%llu.%d.%d.%ld.%d.%ld", \
          pidkey, usec, action, sockfd, len_param, flags, len_result);
  db_nwrite(mydb, key, buf, len_result);
  
  // prv.pid.$(pid.usec).skid.$sockid.act.$action.n.$pkgid -> $syscall_result
  sprintf(key, "prv.pid.%s.skid.%s.act.%d.n.%llu", \
          pidkey, sockid, action, pkgid);
  ull_t result = len_result;
  db_nwrite(mydb, key, (char*) &result, sizeof(ull_t));
  
  // prv.pid.$(pid.usec).skid.$sockid.act.$action.n.$pkgid.buff -> $buff
  sprintf(key, "prv.pid.%s.skid.%s.act.%d.n.%llu.buff", \
          pidkey, sockid, action, pkgid);
  db_nwrite(mydb, key, buf, len_result);
  
  free(sockid);
  free(pidkey);
}

/* =====
 * connect socket
 */
void db_setupConnectCounter(lvldb_t *mydb, char* pidkey) {
  char key[KEYLEN];
  ull_t zero = 0;
  sprintf(key, "prv.pid.%s.sockn", pidkey);
  db_nwrite(mydb, key, (char*) &zero, sizeof(ull_t));
}
ull_t db_getConnectCounterInc(lvldb_t *mydb, char* pidkey) {
  char key[KEYLEN];
  sprintf(key, "prv.pid.%s.sockn", pidkey);
  return db_getCounterInc(mydb, key);
}
//~ void db_setConnectId(lvldb_t *mydb, char *pidkey, int sockfd, ull_t sockid) {
  //~ char key[KEYLEN];
  //~ // prv.pid.$(pid.usec).sk2id.$sockfd -> $n
  //~ sprintf(key, "prv.pid.%s.sk2id.%d", pidkey, sockfd);
  //~ db_nwrite(mydb, key, (char*) &sockid, sizeof(ull_t));
//~ }

void db_write_connect_prov(lvldb_t *mydb, long pid, 
    int sockfd, char* addr, int addr_len, long u_rval) {
  char key[KEYLEN], idkey[KEYLEN];
  char *pidkey = db_read_pid_key(mydb, pid);
  if (pidkey == NULL) return;
  ull_t usec = getusec();
  
  // prv.sock.$(pid.usec).newfd.$usec.$sockfd.$addr_len.$u_rval -> $(addr) [1]
  sprintf(key, "prv.sock.%s.newfd.%llu.%d.%d.%lu", \
          pidkey, usec, sockfd, addr_len, u_rval);
  db_nwrite(mydb, key, addr, addr_len);
  
  // prv.pid.$(pid.usec).sockid.$n -> $key[1]
  ull_t sockn = db_getConnectCounterInc(mydb, pidkey);
  sprintf(idkey, "prv.pid.%s.sockid.%llu", pidkey, sockn);
  db_write(mydb, idkey, key);
  
  db_setSockConnectId(mydb, pidkey, sockfd, sockn);
  db_setupSockConnectCounter(mydb, pidkey, sockfd, sockn);
  
  free(pidkey);
}

/* =====
 * listen socket
 * int listen(int sockfd, int backlog);
 */
void db_setupListenCounter(lvldb_t *mydb, char* pidkey) {
  char key[KEYLEN];
  sprintf(key, "prv.pid.%s.listenn", pidkey);
  ull_t zero = 0;
  db_nwrite(mydb, key, (char*) &zero, sizeof(ull_t));
}
ull_t db_getListenCounterInc(lvldb_t *mydb, char* pidkey) {
  char key[KEYLEN];
  sprintf(key, "prv.pid.%s.listenn", pidkey);
  ull_t res = db_getCounterInc(mydb, key);
  printf("%llu\n", res);
  return res;
}
void db_write_listen_prov(lvldb_t *mydb, int pid, int sock, int backlog, int result) {
  char key[KEYLEN],value[KEYLEN];
  char *pidkey = db_read_pid_key(mydb, pid);
  ull_t listenn = db_getListenCounterInc(mydb, pidkey);
  sprintf(key, "prv.pid.%s.listenid.%llu", pidkey, listenn);
  sprintf(value, "%d.%d.%d", result, sock, backlog);
  db_write(mydb, key, value);
}
void print_listen_prov(struct tcb *tcp) {
  if (CDE_provenance_mode) {
    db_write_listen_prov(provdb, tcp->pid, 
        tcp->u_arg[0], tcp->u_arg[1], tcp->u_rval);
  }
}
int db_getListenResult(lvldb_t *mydb, char* pidkey, ull_t id) {
  char key[KEYLEN], *value;
  size_t len;
  int result;
  sprintf(key, "prv.pid.%s.listenid.%llu", pidkey, id);
  value = db_nread(mydb, key, &len);
  sscanf(value, "%d", &result);
  return result;
}
void db_setListenId(lvldb_t *mydb, char* pidkey, int sock, ull_t sockid) {
  char key[KEYLEN];
  sprintf(key, "pid.%s.lssk2id.%d", pidkey, sock);
  if (CDE_verbose_mode) {
    vbprintf("[xxxx] db_setListenId pidkey %s, sock %d -> lssk2id %d\n", 
        pidkey, sock, sockid);
  }
  db_nwrite(mydb, key, (char*) &sockid, sizeof(ull_t));
}
ull_t db_getListenId(lvldb_t *mydb, char* pidkey, int sock) {
  char key[KEYLEN];
  ull_t sockid;
  sprintf(key, "pid.%s.lssk2id.%d", pidkey, sock);
  db_read_ull(mydb, key, &sockid);
  if (CDE_verbose_mode) {
    vbprintf("[xxx] db_getListenId pidkey %s, sock %d -> lssk2id %llu\n", 
        pidkey, sock, sockid);
  }
  return sockid;
}

/* =====
 * accept socket
 * int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
 */
ull_t db_getAcceptCounterInc(lvldb_t *mydb, char* pidkey, int lssock) {
  char key[KEYLEN];
  sprintf(key, "prv.pid.%s.listenid.%d.acceptn", pidkey, lssock);
  return db_getCounterInc(mydb, key);
}
void db_setAcceptId(lvldb_t *mydb, char* pidkey, int client_sock, ull_t listenid, ull_t acceptid) {
  char key[KEYLEN], value[KEYLEN];
  sprintf(key, "pid.%s.ac2id.%d", pidkey, client_sock);
  sprintf(value, "%llu.%llu", listenid, acceptid);
  db_write(mydb, key, value);
  if (CDE_verbose_mode) {
    vbprintf("[xxxx] db_setAcceptId pidkey %s, client_sock %d -> listenid %llu, acceptid %d\n", 
        pidkey, listenid, client_sock, acceptid);
  }
}
void db_write_accept_prov(lvldb_t *mydb, int pid, int lssock, char* addrbuf, int len, ull_t client_sock) {
  char key[KEYLEN];
  char *pidkey = db_read_pid_key(mydb, pid);
  ull_t listenid = db_getListenId(mydb, pidkey, lssock);
  ull_t acceptid = db_getAcceptCounterInc(mydb, pidkey, lssock);
  
  sprintf(key, "prv.pid.%s.listenid.%llu.accept.%llu.addr", pidkey, listenid, acceptid);
  db_nwrite(mydb, key, addrbuf, len);
  sprintf(key, "prv.pid.%s.listenid.%llu.accept.%llu", pidkey, listenid, acceptid);
  db_nwrite(mydb, key, (char*) &client_sock, sizeof(ull_t));
  
  db_setAcceptId(mydb, pidkey, client_sock, listenid, acceptid);
  db_setupSockAcceptCounter(mydb, pidkey, client_sock, listenid, acceptid);
}
void print_accept_prov(struct tcb *tcp) {
  if (CDE_provenance_mode) {
    char addrbuf[128];
    if (umoven(tcp, tcp->u_arg[1], tcp->u_arg[2], addrbuf) < 0)
      return;
    db_write_accept_prov(provdb, tcp->pid, 
        tcp->u_arg[0], addrbuf, tcp->u_arg[2], tcp->u_rval);
  }
}

/*
 * primary key of processes:
 * pid.$pid -> $(pid.usec)
 * with prv.pid.$(pid.usec).parent -> $(ppid.usec)
 *
 ******** 
 * IO provenance: (note: should change to prv.pid.$(pid.usec).io.$action.$usec)
 * prv.iopid.$(pid.usec).$action.$usec -> $filepath // tuple (pid, action, time, filepath)
 * prv.iofile.$filepath.$(pid.usec).$usec -> $action
 * 
 ******** 
 * Network provenance:
 * prv.pid.$(pid.usec).sockn -> #n // current sock counter
 * 
 * Per send/receive request:
 * prv.pid.$(pid.usec).sock.$usec.$action.$sockfd.$len_param.$flags.$len_result -> $memoryblock
 * prv.pid.$(pid.usec).skid.$sockid.act.$action -> $n
 * prv.pid.$(pid.usec).skid.$sockid.act.$action.n.$counter -> $syscall_result
 * 
 * Per socket:
 * prv.sock.$(pid.usec).newfd.$usec.$sockfd.$addr_len.$u_rval -> $(addr) [1]
 * prv.pid.$(pid.usec).sockid.$n -> $key[1]
 * prv.pid.$(pid.usec).sk2id.$sockfd -> $n // temporary of "active" sockfd
 * 
 * Per listen request:
 * prv.pid.$(pid.usec).listenn -> $n
 * prv.pid.$(pid.usec).listenid.$n -> $result.$sockfd.$backlog
 *
 ******** 
 * Exec provenance:
 * prv.pid.$(ppid.usec).exec.$usec -> $(pid.usec)
 * prv.pid.$(pid.usec).[path, pwd, args, start] -> corresponding value of EXECVE
 * prv.pid.$(pid.usec).ok -> success (>0, = usec) or not exists
 * prv.pid.$(pid.usec).lexit -> $usec
 * prv.pid.$(pid.usec).stat.$usec -> $fullstring // every 1 sec?
 *
 * prv.pid.$(ppid.usec).spawn.$usec -> $(pid.usec)
 *
 ******** 
 * Exec statistic:
 * prv.pid.$(pid.usec).stat.$usec -> $(content of /proc/$pid/stat)
 * prv.pid.$(pid.usec).iostat.$usec -> $(content of /proc/$pid/io)
 *
 * === summary graph ===
 * prv.pid.$(pid.usec).actualpid -> $(pid.usec)       // if a "real" process
 * prv.pid.$(pid.usec).actualpid -> $(actualpid.usec) // if spawn
 * prv.pid.$(pid.usec).actualparent -> $(actualppid.usec)
 * prv.pid.$(actualppid.usec).actualexec.$usec -> $(actualpid.usec)
 * prv.iopid.$(actualppid.usec).actual.$action.$usec -> $filepath
 */

