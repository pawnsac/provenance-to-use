/*******************************************************************************
 * SYSTEM INCLUDES
 ******************************************************************************/

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
#include <sys/wait.h>
#include <stdio.h>      //printf
#include <string.h>     //memset
#include <errno.h>      //errno
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <unistd.h>

/*******************************************************************************
 * USER INCLUDES
 ******************************************************************************/

#include "defs.h"       // strace module, must go first
#include "provenance.h"
#include "cde.h"
#include "okapi.h"      // canonicalize_path()
#include "cdenet.h"
#include "const.h"
#include "db.h"

/*******************************************************************************
 * EXTERNALLY-DEFINED FUNCTIONS
 ******************************************************************************/

extern void vbprintf (const char* fmt, ...); // cde.c
extern char* strcpy_from_child_or_null (struct tcb* tcp, long addr); // cde.c
extern void print_trace (void);              // strace's util.c
extern int string_quote (const char* instr, char* outstr, int len, int size); // strace's util.c

/*******************************************************************************
 * PUBLIC VARIABLES
 ******************************************************************************/

char* Prov_db_id = NULL;       // leveldb prov_db id
char Prov_prov_mode = 0;       // true if auditing (opposite of Cde_exec_mode)
char Prov_no_app_capture = 0;  // if true, run cde to collect prov but don't capture app

/*******************************************************************************
 * PRIVATE TYPES / CONSTANTS / VARIABLES
 ******************************************************************************/

// private types
typedef struct {
  pid_t pv[1000]; // array of pid's
  int pc;         // current num pid's in array
} PidList;

// private constants
#define ENV_LEN 16384     // max length of str to hold environ vars

// private variables
static FILE* prov_logfile = NULL; // provenance log file
static lvldb_t* prov_db;          // provenance leveldb
static PidList pidlist;
static pthread_mutex_t mut_logfile = PTHREAD_MUTEX_INITIALIZER; // atomically update log file
static pthread_mutex_t mut_pidlist = PTHREAD_MUTEX_INITIALIZER; // atomically update pidlist

/*******************************************************************************
 * PRIVATE MACROS / FUNCTIONS
 ******************************************************************************/

#ifndef MAX
#  define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#  define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

// return string of current environ vars for input PID
static char* get_env_from_pid (int pid, int* length) {
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

// print something (? pidlist ?) to provlog & leveldb
static void print_curr_prov (PidList* pidlist_p) {
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
      fprintf(prov_logfile, "%d %u LEXIT\n", curr_time, pidlist_p->pv[i]); // lost_pid exit
      db_write_lexit_prov(prov_db, pidlist_p->pv[i]);
      pidlist_p->pv[i] = pidlist_p->pv[pidlist_p->pc-1];
      pidlist_p->pc--;
      continue;
    }
    if (fgets(buff, 1024, f) == NULL)
      rss= 0;
    else
      // details of format: http://git.kernel.org/?p=linux/kernel/git/stable/linux-stable.git;a=blob_plain;f=fs/proc/array.c;hb=d1c3ed669a2d452cacfb48c2d171a1f364dae2ed
      sscanf(buff, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u "
          "%*u %*u %*u %*u %*u %*d %*d %*d %*d %*d %*d %*u %lu ", &rss);
    fclose(f);
    fprintf(prov_logfile, "%d %u MEM %lu\n", curr_time, pidlist_p->pv[i], rss);
    db_write_prov_stat(prov_db, pidlist_p->pv[i], "stat", buff);
    sprintf(buff, "/proc/%d/io", pidlist_p->pv[i]);
    f = fopen(buff, "r");
    if (f==NULL) continue; 

    int iostat = fread(buff, 1, 1024, f);
    if (iostat > 0) {
      buff[iostat] = 0;
      db_write_prov_stat(prov_db, pidlist_p->pv[i], "iostat", buff);
    }
    fclose(f);
  }
  pthread_mutex_unlock(&mut_pidlist);
}

// add input pid to end of pidlist array
static void add_pid_prov (const pid_t pid) {
  pthread_mutex_lock(&mut_pidlist);
  pidlist.pv[pidlist.pc] = pid;
  pidlist.pc++;
  pthread_mutex_unlock(&mut_pidlist);
  print_curr_prov(&pidlist);
}

// remove input pid from pidlist array
static void rm_pid_prov (const pid_t pid) {
  int i=0;
  if (pidlist.pc<=0) return;
  print_curr_prov(&pidlist);
  pthread_mutex_lock(&mut_pidlist);
  while (pidlist.pv[i] != pid && i < pidlist.pc) i++;
  if (i < pidlist.pc) {
    pidlist.pv[i] = pidlist.pv[pidlist.pc-1];
    pidlist.pc--;
  }
  pthread_mutex_unlock(&mut_pidlist);
}

// log file read/write/rw to provlog & leveldb
static void print_io_prov (struct tcb* tcp, const int path_index, const int action) {
  char *filename = strcpy_from_child_or_null(tcp, tcp->u_arg[path_index]);
  char *filename_abspath = canonicalize_path(filename, tcp->current_dir);
  assert(filename_abspath);

  fprintf(prov_logfile, "%d %u %s %s\n", (int)time(0), tcp->pid,
      (action == PRV_RDONLY ? "READ" : (
        action == PRV_WRONLY ? "WRITE" : (
        action == PRV_RDWR ? "READ-WRITE" : "UNKNOWNIO"))),
      filename_abspath);
  db_write_io_prov(prov_db, tcp->pid, action, filename_abspath);

  free(filename);
  free(filename_abspath);
}

// log file read/write/rw + file fd to provlog & leveldb
static void print_iofd_prov (struct tcb* tcp, const int path_index, const int action, const int fd) {
  char *filename = strcpy_from_child_or_null(tcp, tcp->u_arg[path_index]);
  char *filename_abspath = canonicalize_path(filename, tcp->current_dir);
  assert(filename_abspath);

  fprintf(prov_logfile, "%d %u %s %s\n", (int)time(0), tcp->pid,
      (action == PRV_RDONLY ? "READ" : (
        action == PRV_WRONLY ? "WRITE" : (
        action == PRV_RDWR ? "READ-WRITE" : "UNKNOWNIO"))),
      filename_abspath);
  db_write_iofd_prov(prov_db, tcp->pid, action, filename_abspath, fd);

  free(filename);
  free(filename_abspath);
}

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

void print_newsock_prov(struct tcb *tcp, int action, \
    unsigned int s_port, unsigned long s_ipv4, \
    unsigned int d_port, unsigned long d_ipv4, int sk) {
  struct in_addr s_in, d_in;
  char saddr[32], daddr[32];
  if (Prov_prov_mode) {
    s_in.s_addr = s_ipv4;
    strcpy(saddr, inet_ntoa(s_in));
    d_in.s_addr = d_ipv4;
    strcpy(daddr, inet_ntoa(d_in));
    fprintf(prov_logfile, "%d %u %d %u %s %u %s %d\n", (int)time(0), tcp->pid, \
        action, s_port, saddr, d_port, daddr, sk);
    //~ printf("%d %u %d %u %s %u %s %d\n", (int)time(0), tcp->pid, 
        //~ action, s_port, saddr, d_port, daddr, sk);
    //db_write_connect_prov(tcp->pid, action, filename_abspath);
  }
}

void print_connect_prov(struct tcb *tcp, 
    int sockfd, char* addrbuf, int addr_len, long u_rval, char *ips) {
  if (Prov_prov_mode) {
    db_setCapturedSock(prov_db, sockfd);
    db_write_connect_prov(prov_db, tcp->pid, sockfd, addrbuf, addr_len, u_rval, ips);
    //~ struct in_addr d_in;
    //~ char daddr[32];
    //~ d_in.s_addr = sock.ip.ipv4;
    //~ strcpy(daddr, inet_ntoa(d_in));
    //~ fprintf(prov_logfile, "%d %u SOCK_CONNECT %u %ld %d\n", (int)time(0), tcp->pid, 
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
  fprintf(prov_logfile, "%d %u SOCK %d %ld %d %ld %d\n", (int)time(0), tcp->pid, \
        sockfd, len_param, flags, len_result, action);
  db_write_sock_action(prov_db, tcp->pid, sockfd, buf, len_param, flags, \
                       len_result, action, msg);
  if (Cde_verbose_mode && (action == SOCK_SEND || action == SOCK_RECVMSG)) {
    #define NPRINT (100)
    if (buf != NULL && strlen(buf)>NPRINT+3) {
      buf[NPRINT] = '.';buf[NPRINT+1] = '.';buf[NPRINT+2] = '.';buf[NPRINT+3] = '\0';
    }
    vbprintf("[%d-prov] print_sock_action action %d [%ld] '%s'\n", tcp->pid, action, len_param, buf);
  }
}

void retrieve_remote_new_dbs(char* remotehost, char* dbid) {
  char rpath[KEYLEN];
  vbp(2, "Retrieve dbs from %s\n", remotehost);
  
  // do the fork
  pid_t pid = fork();
  if (pid == -1) {
    vbp(0, "Error fork\n");
    return;
  }
  
  // child
  if (pid == 0) {
    // sprintf(rpath, "%s:~/cde-package/provenance.cde-root.*", remotehost);
	sprintf(rpath, "%s:/var/tmp/cde-root.%s/provenance.cde-root.1.log.id*",
			remotehost, dbid);
    execlp("scp", "scp", "-r", "-q", rpath, 
        CDE_PACKAGE_DIR, (char *) NULL);
    _exit(127);
  }
  
  // parent
  return; // no need to wait, just do your best here

  int status;
  if (waitpid(pid, &status, 0) == -1) {
    // handle error
    vbp(0, "Error waitpid %d\n", pid);
  } else {
    // child exit code in status
    // use WIFEXITED, WEXITSTATUS, etc. on status
    if (WEXITSTATUS(status) == 127) {
      vbp(0, "Error: scp not found\n");
    } else {
      if (WEXITSTATUS(status) == 0) {
        vbp(1, "Done\n");
      } else {
        vbp(0, "Error\n");
      }
    }
  }
}

void print_iexit_prov(struct tcb *tcp) {
  if (Prov_prov_mode) { // not handle exit by signal yet
		rm_pid_prov(tcp->pid);
		fprintf(prov_logfile, "%d %u EXIT\n", (int)time(0), tcp->pid);
    db_write_iexit_prov(prov_db, tcp->pid);
    char *ssh_host = NULL, *ssh_dbid = NULL;
    if (Cde_follow_ssh_mode && db_get_ssh_host(prov_db, tcp->pid, &ssh_host, &ssh_dbid))
      retrieve_remote_new_dbs(ssh_host, ssh_dbid);
    freeifnn(ssh_host);
    freeifnn(ssh_dbid);
  }
}

void *capture_cont_prov(void* ptr) {
  PidList *pidlist_p = (PidList*) ptr;
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

	if (prov_logfile) {
    vbprintf("=== Close log file and provenance db ===\n");
  	fclose(prov_logfile);
    leveldb_close(prov_db->db);
  }
  pthread_mutex_destroy(&mut_logfile);

  return NULL;
}

char* get_local_ip() {
	FILE *f;
	char line[100] , *p , *c;

	f = fopen("/proc/net/route" , "r");

	while(fgets(line , 100 , f))  {
		p = strtok(line , " \t");
		c = strtok(NULL , " \t");

		if(p!=NULL && c!=NULL) {
			if(strcmp(c , "00000000") == 0) {
				// printf("Default interface is : %s \n" , p);
				break;
			}
		}
	}

	//which family do we require , AF_INET or AF_INET6
	int fm = AF_INET;
	struct ifaddrs *ifaddr, *ifa;
	int family , s;
	char host[NI_MAXHOST];

	if (getifaddrs(&ifaddr) == -1)
	{
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	//Walk through linked list, maintaining head pointer so we can free list later
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL) {
			continue;
		}

		family = ifa->ifa_addr->sa_family;

		if(strcmp( ifa->ifa_name , p) == 0) {
			if (family == fm) {
				s = getnameinfo( ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6) , host , NI_MAXHOST , NULL , 0 , NI_NUMERICHOST);

				if (s != 0) {
					printf("getnameinfo() failed: %s\n", gai_strerror(s));
					exit(EXIT_FAILURE);
				}
				freeifaddrs(ifaddr);
				return strdup(host);
			}
		}
	}

	freeifaddrs(ifaddr);
	return NULL;
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
  if (Prov_prov_mode) {
    db_write_listen_prov(prov_db, tcp->pid, 
        tcp->u_arg[0], tcp->u_arg[1], tcp->u_rval);
  }
}

void print_accept_prov(struct tcb *tcp) {
  if (Prov_prov_mode) {
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
    db_write_accept_prov(prov_db, tcp->pid, 
        tcp->u_arg[0], addrbuf, len, newsockfd, buf);
    db_setCapturedSock(prov_db, newsockfd);
  }
}

void print_getsockname_prov(struct tcb *tcp) {
  if (Prov_prov_mode) {
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
    db_write_getsockname_prov(prov_db, tcp->pid, 
        tcp->u_arg[0], addrbuf, len, tcp->u_rval);
  }
}

void print_fd_close(struct tcb *tcp) {
  int sockfd = tcp->u_arg[0];
  if (Prov_prov_mode) {
    if (!db_markFileClosed(prov_db, tcp->pid, sockfd)) {
      if (db_isCapturedSock(prov_db, sockfd)) {
        db_remove_sock(prov_db, tcp->pid, sockfd);
      }
    }
  }
}

int isProvCapturedSock(int sockfd) {
  return db_isCapturedSock(prov_db, sockfd);
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/

// initialize leveldb prov-db and provlog file
void init_prov () {
  pthread_t ptid;
  char *env_prov_mode = getenv("IN_CDE_PROVENANCE_MODE");
  char path[PATH_MAX];
  int subns=1;
  char *err = NULL;

  if (env_prov_mode != NULL)
    Prov_prov_mode = (strcmp(env_prov_mode, "1") == 0) ? 1 : 0;
  else
    Prov_prov_mode = !Cde_exec_mode;
  if (Prov_prov_mode) {
    setenv("IN_CDE_PROVENANCE_MODE", "1", 1);
    pthread_mutex_init(&mut_logfile, NULL);
    // create NEW provenance log file
    bzero(path, sizeof(path));
    if (Prov_db_id == NULL)
      sprintf(path, "%s/provenance.%s.1.log", Cde_app_dir, CDE_ROOT_NAME);
    else
      sprintf(path, "%s/provenance.%s.1.log.id%s", Cde_app_dir, CDE_ROOT_NAME, Prov_db_id);
    if (access(path, R_OK)==-1)
      prov_logfile = fopen(path, "w");
    else {
      // check through provenance.$subns.log to find a new file name
      do {
        subns++;
        bzero(path, sizeof(path));
        if (Prov_db_id == NULL)
          sprintf(path, "%s/provenance.%s.%d.log", Cde_app_dir, CDE_ROOT_NAME, subns);
        else
          sprintf(path, "%s/provenance.%s.%d.log.id%s", Cde_app_dir, CDE_ROOT_NAME, subns, Prov_db_id);
      } while (access(path, R_OK)==0);
      fprintf(stderr, "Provenance log file: %s\n", path);
      prov_logfile = fopen(path, "w");
    }

    // leveldb initialization
    sprintf(path+strlen(path), "_db");
    prov_db = malloc(sizeof(lvldb_t));
    prov_db->options = leveldb_options_create();
    leveldb_options_set_create_if_missing(prov_db->options, 1);
    prov_db->db = leveldb_open(prov_db->options, path, &err);
    if (err != NULL || prov_db->db == NULL) {
      fprintf(stderr, "Leveldb open fail!\n");
      exit(-1);
    }
    assert(prov_db->db!=NULL);
    prov_db->woptions = leveldb_writeoptions_create();
    prov_db->roptions = leveldb_readoptions_create();
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
    fprintf(prov_logfile, "# @agent: %s\n", pw == NULL ? "(noone)" : pw->pw_name);
    fprintf(prov_logfile, "# @machine: %s\n", uname);
    fprintf(prov_logfile, "# @namespace: %s\n", CDE_ROOT_NAME);
    fprintf(prov_logfile, "# @subns: %d\n", subns);
    fprintf(prov_logfile, "# @fullns: %s\n", fullns);
    fprintf(prov_logfile, "# @parentns: %s\n", getenv("CDE_PROV_NAMESPACE"));

    // provenance meta data in lvdb
    db_write(prov_db, "meta.agent", pw == NULL ? "(noone)" : pw->pw_name);
    db_write(prov_db, "meta.machine", uname);
    db_write(prov_db, "meta.namespace", CDE_ROOT_NAME);
    db_write_fmt(prov_db, "meta.subns", "%d", subns);
    db_write(prov_db, "meta.fullns", fullns);
    char *parentns = getenv("CDE_PROV_NAMESPACE");
    db_write(prov_db, "meta.parentns", parentns == NULL ? "(null)" : parentns);
    db_write(prov_db, "meta.dbid", Prov_db_id == NULL ? "(null)" : Prov_db_id);

    char* host = get_local_ip();
    if (host != NULL) {
    	db_write(prov_db, "meta.ip", host);
    	free(host);
    } else {
    	db_write(prov_db, "meta.ip", "(NULL)");
    }

    db_write_root(prov_db, getpid());
    setenv("CDE_PROV_NAMESPACE", fullns, 1);

    pthread_mutex_init(&mut_pidlist, NULL);
    pidlist.pc = 0;
    pthread_create( &ptid, NULL, capture_cont_prov, &pidlist);
  }
}

// log beginning proc exec call to provlog & leveldb if auditing, to stderr if verbose
void print_begin_execve_prov (struct tcb* tcp, char* db_id, char* ssh_host) {
  if (Prov_prov_mode || Cdenet_network_mode) {
    char *opened_filename = strcpy_from_child_or_null(tcp, tcp->u_arg[0]);
    char *filename_abspath = canonicalize_path(opened_filename, tcp->current_dir);
    assert(filename_abspath);
    int parentPid = tcp->parent == NULL ? getpid() : tcp->parent->pid;
    char args[KEYLEN*10];
    print_arg_prov(args, tcp, tcp->u_arg[1]);
    if (Prov_prov_mode) {
      fprintf(prov_logfile, "%d %d EXECVE %u %s %s %s\n", (int)time(0),
        parentPid, tcp->pid, filename_abspath, tcp->current_dir, args);
      db_write_exec_prov(prov_db, parentPid, tcp->pid, filename_abspath, 
          tcp->current_dir, args, db_id, ssh_host);
      if (Cde_verbose_mode) {
        vbprintf("[%d-prov] BEGIN %s '%s'\n", tcp->pid, "execve", opened_filename);
      }
    }
    if (Cdenet_network_mode) {
      db_write_exec_prov(Cdenet_exec_provdb, parentPid, tcp->pid, filename_abspath, 
          tcp->current_dir, args, db_id, ssh_host);
    }
    free(filename_abspath);
    free(opened_filename);
  }
}

// log ending proc exec call to provlog if auditing, to stderr if verbose
// log ending proc exec call, proc environ vars, and [something in add_pid_prov] to leveldb
void print_end_execve_prov (struct tcb* tcp) {
  if (Prov_prov_mode || Cdenet_network_mode) {
    int ppid = -1;
    if (tcp->parent) ppid = tcp->parent->pid;
    if (Prov_prov_mode) {
      int env_len;
      char *env_str = get_env_from_pid(tcp->pid, &env_len);
      db_write_execdone_prov(prov_db, ppid, tcp->pid, env_str, env_len);
      fprintf(prov_logfile, "%d %u EXECVE2 %d\n", (int)time(0), tcp->pid, ppid);
      add_pid_prov(tcp->pid);
      if (Cde_verbose_mode) {
        vbprintf("[%d-prov] BEGIN execve2\n", tcp->pid);
      }
      freeifnn(env_str);
    }
    if (Cdenet_network_mode) {
      db_write_execdone_prov(Cdenet_exec_provdb, ppid, tcp->pid, NULL, 0);
    }
  }
}

// log proc creation of new proc to provlog & leveldb if auditing
void print_spawn_prov(struct tcb *tcp) {
  if (Prov_prov_mode || Cdenet_network_mode) {
    if (Prov_prov_mode) {
      fprintf(prov_logfile, "%d %u SPAWN %u\n", (int)time(0), tcp->parent->pid, tcp->pid);
      db_write_spawn_prov(prov_db, tcp->parent->pid, tcp->pid);
    }
    if (Cdenet_network_mode) {
      db_write_spawn_prov(Cdenet_exec_provdb, tcp->parent->pid, tcp->pid);
    }
  }
}

// log file open/openat to provlog & leveldb if auditing, to stderr if verbose
void print_open_prov (struct tcb* tcp, const char* syscall_name) {

  // assume openAT uses the same current_dir with PWD (from CDE code)
  // open u_args:   [abs path] [r/w/rw] [permissions]
  //      u_rval:   fd if ok, -1 if error
  // openat u_args: [rel path] [fd for abs dir] [r/w/rw] [permissions]
  //      u_rval:   fd if ok, -1 if error

  // set path_index to 1 if open, 2 if openat, or return if bad syscall_name
  int path_index;
  if (strcmp(syscall_name, "sys_open") == 0) {
    path_index = 1;
  } else if (strcmp(syscall_name, "sys_openat") == 0) {
    path_index = 2;
  } else {
    return;
  }

  // get abs path used to open file
  char* filename = strcpy_from_child_or_null(tcp, tcp->u_arg[path_index-1]);
  char* filename_abspath = canonicalize_path(filename, tcp->current_dir);

  // log prov if in prov mode and successful open call (on valid file)
  if (Prov_prov_mode && (tcp->u_rval >= 0)) {

    // get r/w/rw mode bits
    unsigned char open_mode = (tcp->u_arg[path_index] & 3);

    // translate mode bits into mode enum
    int action;
    switch (open_mode) {
      case O_RDONLY: action = PRV_RDONLY; break;
      case O_WRONLY: action = PRV_WRONLY; break;
      case O_RDWR: action = PRV_RDWR; break;
      default: action = PRV_UNKNOWNIO; break;
    }

    // log the open call to prov log and prov db
    print_iofd_prov(tcp, path_index - 1, action, tcp->u_rval);

    // store exact abs path used to open the file
    freeifnn(tcp->opened_file_paths[tcp->u_rval]);
    tcp->opened_file_paths[tcp->u_rval] = strdup(filename_abspath);
  }

  // log to stderr if verbose
  if (Cde_verbose_mode >= 1) {
    vbp(1, "%s: fd= %ld\n", filename_abspath, tcp->u_rval);
  }

  freeifnn(filename);
  freeifnn(filename_abspath);
}

// log file read to provlog & leveldb if auditing, to stderr if verbose
void print_read_prov (struct tcb* tcp, const char* syscall_name, const int path_index) {
  if (Prov_prov_mode && tcp->u_rval >= 0) {
    print_io_prov(tcp, path_index, PRV_RDONLY);
  }
}

// log file write to provlog & leveldb if auditing, to stderr if verbose
void print_write_prov (struct tcb* tcp, const char* syscall_name, const int path_index) {
  if (Prov_prov_mode && tcp->u_rval >= 0) {
    print_io_prov(tcp, path_index, PRV_WRONLY);
  }
}

// log file hardlink/symlink to provlog & leveldb if auditing
void print_link_prov (struct tcb* tcp, const char* syscall_name,
                      const int realpath_index, const int linkpath_index) {
  if ( (Prov_prov_mode) && (tcp->u_rval == 0) ) {
      print_io_prov(tcp, realpath_index, PRV_RDONLY);
      print_io_prov(tcp, linkpath_index, PRV_WRONLY);
  }
}

// log file rename/move to provlog & leveldb if auditing
void print_rename_prov (struct tcb* tcp, const int renameat) {
  if (Prov_prov_mode) {
    if (tcp->u_rval == 0) {
      // assume renameat uses the same current_dir with PWD (from CDE code)
      int origpath_index = renameat == 0 ? 0 : 1;
      int newpath_index = renameat == 0 ? 1 : 3;
      print_io_prov(tcp, origpath_index, PRV_RDWR);
      print_io_prov(tcp, newpath_index, PRV_WRONLY);
    }
  }
}

// log file close to provlog if auditing, to stderr if verbose
void print_close_prov (struct tcb* tcp) {
  // exit early if not in prov mode (i.e. not auditing)
  if (!Prov_prov_mode) {
    return;
  }

  // get close sys call's close fd from sys call's pcb
  const int closefd = tcp->u_arg[0];

  // will hold stored exact abs path used to open the file
  const char* openpath;

  // exit early if closing an fd this proc did NOT open
  //  --> there is not prov value in a close without a matching open
  if (tcp->opened_file_paths[closefd] == NULL) {
    return;

  // close sys call WAS called on an fd that this proc opened
  } else {
    openpath = tcp->opened_file_paths[closefd];
  }

  // log to provlog
  fprintf(prov_logfile, "%d %u %s %s\n", (int)time(0), tcp->pid, "CLOSE", openpath);

  // log to stderr if verbose
  if (Cde_verbose_mode) {
    vbprintf("[%d-prov] CLOSE %s\n", tcp->pid, openpath);
  }
}

