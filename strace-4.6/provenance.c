#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <pwd.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "defs.h"
#include "provenance.h"
#include "const.h"
#include "db.h"

/* macros */
#ifndef MAX
#define MAX(a,b)		(((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)		(((a) < (b)) ? (a) : (b))
#endif

extern char* strcpy_from_child(struct tcb* tcp, long addr);
extern void vbprintf(const char *fmt, ...);
extern void print_trace (void);
void rstrip(char *s);

extern char CDE_exec_mode;
extern char CDE_verbose_mode;
extern char CDE_nw_mode;
extern char cde_pseudo_pkg_dir[MAXPATHLEN];

// local stuff
char *DB_ID = NULL;
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

extern int getsockinfo(struct tcb *tcp, char* addr, socketdata_t *psock);
extern int getPort(union sockaddr_t *addrbuf);
extern void get_ip_info(long pid, int sockfd, char *buf);

void init_prov();

void print_IO_prov(struct tcb *tcp, char* filename, const char* syscall_name);
void print_spawn_prov(struct tcb *tcp);
void print_sock_prov(struct tcb *tcp, int action, unsigned int port, unsigned long ipv4);
void print_act_prov(struct tcb *tcp, const char* action);
void print_newsock_prov(struct tcb *tcp, int action, \
  unsigned int s_port, unsigned long s_ipv4, \
  unsigned int d_port, unsigned long d_ipv4, int sk);

void rm_pid_prov(pid_t pid);

void add_pid_prov(pid_t pid);

char* get_env_from_pid(int pid, int *len);

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


void print_exec_prov(struct tcb *tcp, char *db_id) {
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
      db_write_exec_prov(provdb, parentPid, tcp->pid, filename_abspath, tcp->current_dir, args, db_id);
      if (CDE_verbose_mode) {
        vbprintf("[%d-prov] BEGIN %s '%s'\n", tcp->pid, "execve", opened_filename);
      }
    }
    if (CDE_nw_mode) {
      db_write_exec_prov(currdb, parentPid, tcp->pid, filename_abspath, tcp->current_dir, args, db_id);
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
      int env_len;
      char *env_str = get_env_from_pid(tcp->pid, &env_len);
      db_write_execdone_prov(provdb, ppid, tcp->pid, env_str, env_len);
      fprintf(CDE_provenance_logfile, "%d %u EXECVE2 %d\n", (int)time(0), tcp->pid, ppid);
      add_pid_prov(tcp->pid);
      if (CDE_verbose_mode) {
        vbprintf("[%d-prov] BEGIN execve2\n", tcp->pid);
      }
      freeifnn(env_str);
    }
    if (CDE_nw_mode) {
      db_write_execdone_prov(currdb, ppid, tcp->pid, NULL, 0);
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
  if (CDE_provenance_mode || CDE_nw_mode) {
    if (CDE_provenance_mode) {
      fprintf(CDE_provenance_logfile, "%d %u SPAWN %u\n", (int)time(0), tcp->parent->pid, tcp->pid);
      db_write_spawn_prov(provdb, tcp->parent->pid, tcp->pid);
    }
    if (CDE_nw_mode) {
      db_write_spawn_prov(currdb, tcp->parent->pid, tcp->pid);
    }
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
    //~ printf("%d %u %d %u %s %u %s %d\n", (int)time(0), tcp->pid, 
        //~ action, s_port, saddr, d_port, daddr, sk);
    //db_write_connect_prov(tcp->pid, action, filename_abspath);
  }
}

void print_connect_prov(struct tcb *tcp, 
    int sockfd, char* addrbuf, int addr_len, long u_rval, char *ips) {
  if (CDE_provenance_mode) {
    db_setCapturedSock(provdb, sockfd);
    db_write_connect_prov(provdb, tcp->pid, sockfd, addrbuf, addr_len, u_rval, ips);
    //~ struct in_addr d_in;
    //~ char daddr[32];
    //~ d_in.s_addr = sock.ip.ipv4;
    //~ strcpy(daddr, inet_ntoa(d_in));
    //~ fprintf(CDE_provenance_logfile, "%d %u SOCK_CONNECT %u %ld %d\n", (int)time(0), tcp->pid, 
        //~ sock.port, (long) daddr, sockfd);
  }
}

void print_sock_action(struct tcb *tcp, int sockfd, \
                       char *buf, long len_param, int flags, \
                       long len_result, int action, void *msg) {
  if (0) {
    int i;
    printf("sock %d action %d size %ld res %ld: '", \
           sockfd, action, len_param, len_result);
    for (i=0; i<len_result; i++) {
      printf("%c", buf[i]);
    }
    printf("'\n");
  }
  fprintf(CDE_provenance_logfile, "%d %u SOCK %d %ld %d %ld %d\n", (int)time(0), tcp->pid, \
        sockfd, len_param, flags, len_result, action);
  db_write_sock_action(provdb, tcp->pid, sockfd, buf, len_param, flags, \
                       len_result, action, msg);
  if (CDE_verbose_mode && (action == SOCK_SEND || action == SOCK_RECVMSG)) {
    #define NPRINT (100)
    if (buf != NULL && strlen(buf)>NPRINT+3) {
      buf[NPRINT] = '.';buf[NPRINT+1] = '.';buf[NPRINT+2] = '.';buf[NPRINT+3] = '\0';
    }
    vbprintf("[%d-prov] print_sock_action action %d [%ld] '%s'\n", tcp->pid, action, len_param, buf);
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
      db_write_lexit_prov(provdb, pidlist_p->pv[i]);
      pidlist_p->pv[i] = pidlist_p->pv[pidlist_p->pc-1];
      pidlist_p->pc--;
      continue;
    }
    if (fgets(buff, 1024, f) == NULL)
      rss= 0;
    else
      // details of format: http://git.kernel.org/?p=linux/kernel/git/stable/linux-stable.git;a=blob_plain;f=fs/proc/array.c;hb=d1c3ed669a2d452cacfb48c2d171a1f364dae2ed
      //~ sscanf(buff, "%*d %*s %*c %*d %*d %*d %*d %*d %*lu %*lu "
          //~ "%*lu %*lu %*lu %*lu %*lu %*ld %*ld %*ld %*ld %*ld %*ld %*lu %lu ", &rss);
      sscanf(buff, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u "
          "%*u %*u %*u %*u %*u %*d %*d %*d %*d %*d %*d %*u %lu ", &rss);
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
    if (DB_ID == NULL)
      sprintf(path, "%s/provenance.%s.1.log", cde_pseudo_pkg_dir, CDE_ROOT_NAME);
    else
      sprintf(path, "%s/provenance.%s.1.log.id%s", cde_pseudo_pkg_dir, CDE_ROOT_NAME, DB_ID);
    if (access(path, R_OK)==-1)
      CDE_provenance_logfile = fopen(path, "w");
    else {
      // check through provenance.$subns.log to find a new file name
      do {
        subns++;
        bzero(path, sizeof(path));
        if (DB_ID == NULL)
          sprintf(path, "%s/provenance.%s.%d.log", cde_pseudo_pkg_dir, CDE_ROOT_NAME, subns);
        else
          sprintf(path, "%s/provenance.%s.%d.log.id%s", cde_pseudo_pkg_dir, CDE_ROOT_NAME, subns, DB_ID);
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
    char *parentns = getenv("CDE_PROV_NAMESPACE");
    db_write(provdb, "meta.parentns", parentns == NULL ? "(null)" : parentns);
    db_write(provdb, "meta.dbid", DB_ID == NULL ? "(null)" : DB_ID);

    db_write_root(provdb, getpid());
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

void print_listen_prov(struct tcb *tcp) {
  if (CDE_provenance_mode) {
    db_write_listen_prov(provdb, tcp->pid, 
        tcp->u_arg[0], tcp->u_arg[1], tcp->u_rval);
  }
}

void print_accept_prov(struct tcb *tcp) {
  if (CDE_provenance_mode) {
    char addrbuf[KEYLEN], buf[KEYLEN];
    bzero(buf, sizeof(buf));
    socklen_t len = 0;
    int newsockfd = tcp->u_rval;
    if (tcp->u_arg[1] != 0) {
      if (umoven(tcp, tcp->u_arg[2], sizeof(len), (char*) &len) < 0) {
        vbprintf("[xxxx-prov] Error accept - umoven\n");
        return;
      }
    }
    if (umoven(tcp, tcp->u_arg[1], len, addrbuf) < 0) {
      vbprintf("[xxxx-prov] Error accept - umoven\n");
      return;
    }
    if (tcp->u_rval >= 0) {
      get_ip_info(tcp->pid, newsockfd, buf);
    }
    db_write_accept_prov(provdb, tcp->pid, 
        tcp->u_arg[0], addrbuf, len, newsockfd, buf);
    db_setCapturedSock(provdb, newsockfd);
  }
}

void print_getsockname_prov(struct tcb *tcp) {
  if (CDE_provenance_mode) {
    char addrbuf[KEYLEN];
    socklen_t len = 0;
    if (tcp->u_arg[1] != 0) {
      if (umoven(tcp, tcp->u_arg[2], sizeof(len), (char*) &len) < 0) {
        vbprintf("[xxxx-prov] Error getsockname - umoven\n");
        return;
      }
    }
    if (umoven(tcp, tcp->u_arg[1], len, addrbuf) < 0) {
      vbprintf("[xxxx-prov] Error getsockname - umoven\n");
      return;
    }
    db_write_getsockname_prov(provdb, tcp->pid, 
        tcp->u_arg[0], addrbuf, len, tcp->u_rval);
  }
}

void print_sock_close(struct tcb *tcp) {
  int sockfd = tcp->u_arg[0];
  if (CDE_provenance_mode && db_isCapturedSock(provdb, sockfd)) {
    db_remove_sock(provdb, tcp->pid, sockfd);
  }
}

int isProvCapturedSock(int sockfd) {
  return db_isCapturedSock(provdb, sockfd);
}

#define ENV_LEN 16384
char* get_env_from_pid(int pid, int *length) {
  char fullenviron_fn[KEYLEN];
  char *environment = malloc(ENV_LEN);
  if (environment == NULL) return NULL;
  
  sprintf(fullenviron_fn, "/proc/%d/environ", pid);
  int full_environment_fd = open(fullenviron_fn, O_RDONLY);
  *length = read (full_environment_fd, environment, ENV_LEN) + 1;
  environment[*length - 1] = '\0'; // NULL terminate the string
  close(full_environment_fd);
  return environment;
}
