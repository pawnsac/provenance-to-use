#include <assert.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include "db.h"
#include "const.h"

// extern stuff
extern char CDE_verbose_mode;
extern void vbprintf(const char *fmt, ...);
extern void print_trace (void);


// local function signatures

void printbuf(const char *buf, size_t buflength) {
  int i;
  size_t n = buflength > 200 ? 200 : buflength;
  if (buf == NULL) {
    fprintf(stderr, "\n");
    return;
  }
  fprintf(stderr, "'");
  for (i = 0; i < n; i++) {
    fprintf(stderr, (buf[i] >= 0x20 && buf[i] <= 0x7e) ? "%c" : "\%d", buf[i]);
  }
  fprintf(stderr, buflength > 200 ? "...'\n" : "'\n");
}

uint32_t checksum(const void *buf, size_t buflength) { // Adler-32
  if (buf == NULL) return 0;
  const uint8_t *buffer = (const uint8_t*)buf;

  uint32_t s1 = 1;
  uint32_t s2 = 0;
  size_t n;

  for (n = 0; n < buflength; n++) {
    s1 = (s1 + buffer[n]) % 65521;
    s2 = (s2 + s1) % 65521;
  }     
  return (s2 << 16) | s1;
}

static ull_t getusec() {
  struct timeval tv;
  ull_t usec;
  if (gettimeofday(&tv, NULL) != 0) {
    usec = -1;
  }
  usec = (ull_t) tv.tv_sec * 1000000 + tv.tv_usec;
  return usec;
}

void db_readfailed(const char* key, int readlen) {
  vbp(2, "'%s' %d\n", key, readlen);
  //~ print_trace();
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
    vbprintf("[xxxx-db] DB - Write FAILED: '%s' -> '%s'\n", key, value);
    print_trace();
  }

  leveldb_free(err); err = NULL;
}

char* db_nread(lvldb_t *mydb, const char *key, size_t *plen) {
  char *err = NULL;
  assert(mydb->db!=NULL);

  char* res = leveldb_get(mydb->db, mydb->roptions, key, strlen(key), plen, &err);

  if (err != NULL) db_readfailed(key, *plen);

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
  if (err != NULL) db_readfailed(key, read_len);
  
  if (read == NULL)
    return NULL;
  else {
    read = realloc(read, read_len+1);
    read[read_len] = 0;
    leveldb_free(err); err = NULL;
    return read;
  }
}

int db_read_ull(lvldb_t *mydb, const char *key, ull_t* pvalue) {
  char *err = NULL;
  ull_t *read;
  size_t read_len;

  read = (void*) leveldb_get(mydb->db, mydb->roptions, key, strlen(key), &read_len, &err);
  if (err != NULL) db_readfailed(key, read_len);

  leveldb_free(err); err = NULL;
  if (read == NULL || read_len != sizeof(ull_t)) {
    db_readfailed(key, read_len);
    return 0;
  } else {
    *pvalue = *read;
    freeifnn(read);
    return 1;
  }
}

// the initial PTU pid node
void db_write_root(lvldb_t *mydb, long pid) {
  
  char key[KEYLEN], pidkey[KEYLEN];
  ull_t usec = getusec();

  sprintf(key, "pid.%ld", pid);
  sprintf(pidkey, "%ld.%llu", pid, usec);
  db_write(mydb, key, pidkey);

  sprintf(pidkey, "%ld.%llu", pid, usec);
  db_write(mydb, "meta.root", pidkey);
  
  // initialize child counter
  ull_t zero = 0;
  sprintf(key, "prv.pid.%s.childn", pidkey);
  db_nwrite(mydb, key, (char*) &zero, sizeof(ull_t));
  sprintf(key, "prv.pid.%s.childid", pidkey);
  db_nwrite(mydb, key, (char*) &zero, sizeof(ull_t));
}

ull_t db_getCounterInc(lvldb_t *mydb, char* key) {
  ull_t read;
  db_read_ull(mydb, key, &read);
  vbp(3, "%s -> %llu\n", key, read);
  read++;
  db_nwrite(mydb, key, (char*)&read, sizeof(ull_t));
  return read - 1;
}

char* db_read_pid_key(lvldb_t *mydb, long pid) {
  char key[KEYLEN];
  sprintf(key, "pid.%ld", pid);
  vbp(3, "%ld\n", pid);
  return db_readc(mydb, key);
}

char *db_read_real_pid_key(lvldb_t *mydb, long pid) {
  char key[KEYLEN];
  char *pidkey;

  sprintf(key, "pid.%ld", pid);
  pidkey = db_readc(mydb, key);

  while (pidkey != NULL) {
    sprintf(key, "prv.pid.%s.ok", pidkey);
    char *pidok = db_readc(mydb, key);
    if (pidok != NULL) {
      free(pidok);
      vbp(3, "%ld -> %s\n", pid, pidkey);
      return pidkey; // got the correct pid, now return
    }
    sprintf(key, "prv.pid.%s.parent", pidkey);
    free(pidkey);
    pidkey = db_readc(mydb, key);
  }

  vbp(3, "%ld -> NULL\n", pid);
  return NULL;
}

void db_write_fmt(lvldb_t *mydb, const char *key, const char *fmt, ...) {
  char val[KEYLEN];
  va_list args;
	va_start(args, fmt);
  vsprintf(val, fmt, args);
	va_end(args);
  db_write(mydb, key, val);
}

void db_write_iofd_prov(lvldb_t *mydb, long pid, int action, const char *filename_abspath, int fd) {
  ull_t usec = db_write_io_prov(mydb, pid, action, filename_abspath);
  char key[KEYLEN], value[KEYLEN];
  char *pidkey=db_read_pid_key(mydb, pid);
  if (pidkey == NULL) return;
  
  sprintf(key, "prv.iopid.%s.%d.%llu.fd", pidkey, action, usec);
  db_nwrite(mydb, key, (char*) &fd, sizeof(int));

  sprintf(key, "prv.file.%s.%llu.%d.path", pidkey, usec, fd);
  db_write(mydb, key, filename_abspath);
  
  sprintf(value, "prv.file.%s.%llu.%d", pidkey, usec, fd);
  sprintf(key, "file.%s.%d", pidkey, fd); // temporary lookup
  db_write(mydb, key, value);

  free(pidkey);
}

int db_markFileClosed(lvldb_t *mydb, long pid, int fd) {
  char key[KEYLEN];
  char *pidkey=db_read_pid_key(mydb, pid);
  if (pidkey == NULL) return 0;
  ull_t usec = getusec();
  
  sprintf(key, "file.%s.%d", pidkey, fd);
  char *filekey = db_readc(mydb, key);
  
  sprintf(key, "%s.close", filekey);
  db_nwrite(mydb, key, (char*) &usec, sizeof(ull_t));
  
  free(pidkey);
  if (filekey != NULL) {
    // todo: delete temp key from db
    free(filekey);
    return 1;
  } else
    return 0;
}

ull_t db_write_io_prov(lvldb_t *mydb, long pid, int action, const char *filename_abspath) {
  char key[KEYLEN];
  char *pidkey=db_read_pid_key(mydb, pid);
  if (pidkey == NULL) return 0;
  ull_t usec = getusec();

  sprintf(key, "prv.iopid.%s.%d.%llu", pidkey, action, usec);
  db_write(mydb, key, filename_abspath);

  sprintf(key, "prv.iofile.%s.%s.%llu", filename_abspath, pidkey, usec);
  db_write_fmt(mydb, key, "%d", action);

  free(pidkey);
  return usec;
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

void db_setupChildCounter(lvldb_t *mydb, char* ppidkey, char* pidkey) {
  ull_t zero = 0, childn = 0;
  char key[KEYLEN];
  
  if (ppidkey != NULL) {
    // read child counter of parent
    sprintf(key, "prv.pid.%s.childn", ppidkey);
    db_read_ull(mydb, key, &childn);
    
    // rewrite it (using the same key)
    childn++;
    db_nwrite(mydb, key, (char*) &childn, sizeof(ull_t));
    childn--;
    
    sprintf(key, "prv.pid.%s.child.%llu", ppidkey, childn);
    db_write(mydb, key, pidkey);
  }
  
  // set current counter to this pid
  sprintf(key, "prv.pid.%s.childid", pidkey);
  db_nwrite(mydb, key, (char*) &childn, sizeof(ull_t));
  
  // initialize child counter
  sprintf(key, "prv.pid.%s.childn", pidkey);
  db_nwrite(mydb, key, (char*) &zero, sizeof(ull_t));
  
  vbp(3, "%s %s -> %llu\n", ppidkey, pidkey, childn);
}

void db_write_exec_prov(lvldb_t *mydb, long ppid, long pid, const char *filename_abspath, \
    char *current_dir, char *args, char *dbid, char *ssh_host) {
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
  
  if (dbid != NULL) {
    sprintf(key, "prv.pid.%s.dbid", pidkey);
    db_write(mydb, key, dbid);
  }
  if (ssh_host != NULL) {
    sprintf(key, "prv.pid.%s.sshhost", pidkey);
    db_write(mydb, key, ssh_host);
  }

  free(pidkey);
  free(ppidkey);
}

char* db_get_ssh_host(lvldb_t *mydb, long pid) {
  char *pidkey=db_read_pid_key(mydb, pid);
  char key[KEYLEN];
  sprintf(key, "prv.pid.%s.sshhost", pidkey);
  return db_readc(mydb, key);
}

char* db_getEnvVars(lvldb_t *mydb, char* pidkey) {
  char key[KEYLEN];
  sprintf(key, "prv.pid.%s.env", pidkey);
  return db_readc(mydb, key);
}

void db_write_execdone_prov(lvldb_t *mydb, long ppid, long pid,
    char* env_str, int env_len) {
  char key[KEYLEN], value[KEYLEN];
  char *pidkey=db_read_pid_key(mydb, pid);
  char *ppidkey=db_read_pid_key(mydb, ppid);
  if (pidkey == NULL) return;
  ull_t usec = getusec();

  // create (successful) exec relation
  sprintf(key, "prv.pid.%s.ok", pidkey);
  sprintf(value, "%llu", usec);
  db_write(mydb, key, value);
  vbp(3, "%s -> %s\n", key, value);
  
  sprintf(key, "prv.pid.%s.env", pidkey);
  db_nwrite(mydb, key, env_str, env_len);
  
  db_setupChildCounter(mydb, ppidkey, pidkey);

  db_setupConnectCounter(mydb, pidkey);
  db_setupListenCounter(mydb, pidkey);

  free(pidkey);
  free(ppidkey);
}

void db_write_exit_prov(lvldb_t *mydb, const char* keystr, long pid) {
  char key[KEYLEN], value[KEYLEN];
  char *pidkey=db_read_pid_key(mydb, pid);
  if (pidkey == NULL) return;
  ull_t usec = getusec();

  // create (successful) exec relation
  sprintf(key, keystr, pidkey);
  sprintf(value, "%llu", usec);
  db_write(mydb, key, value);

  free(pidkey);
}

void db_write_lexit_prov(lvldb_t *mydb, long pid) {
  db_write_exit_prov(mydb, "prv.pid.%s.lexit", pid);
}

void db_write_iexit_prov(lvldb_t *mydb, long pid) {
  db_write_exit_prov(mydb, "prv.pid.%s.iexit", pid);
}

void db_write_spawn_prov(lvldb_t *mydb, long ppid, long pid) {
  char key[KEYLEN];
  char *ppidkey=db_read_pid_key(mydb, ppid);
  if (ppidkey == NULL) return;
  ull_t usec = getusec();

  char *pidkey = db_create_pid(mydb, pid, usec, ppidkey);

  sprintf(key, "prv.pid.%s.spawn.%llu", ppidkey, usec);
  db_write(mydb, key, pidkey);
  vbp(3, "%s -> %s\n", key, pidkey);
  
  db_setupChildCounter(mydb, ppidkey, pidkey);

  db_setupConnectCounter(mydb, pidkey);
  db_setupListenCounter(mydb, pidkey);

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
  vbp(3, "pidkey %s, sock %d -> sockid %s\n", pidkey, sock, ch_sockid);
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

void db_setCapturedSock(lvldb_t *mydb, int sockfd) {
  char key[KEYLEN];
  sprintf(key, "sock.caputured.%d", sockfd);
  db_write(mydb, key, "1");
  vbp(3, "sock %d\n", sockfd);
}
void db_removeCapturedSock(lvldb_t *mydb, int sockfd) {
  char key[KEYLEN];
  char *err = NULL;
  sprintf(key, "sock.caputured.%d", sockfd);
  leveldb_delete(mydb->db, mydb->woptions, key, strlen(key), &err);
  if (CDE_verbose_mode>=3) {
    vbprintf("[xxxx-db] db_removeCapturedSock sock %d err '%s'\n", sockfd, err == NULL ? "null" : err);
  }
  leveldb_free(err); err = NULL;
}
int db_isCapturedSock(lvldb_t *mydb, int sockfd) {
  char key[KEYLEN], *value;
  sprintf(key, "sock.caputured.%d", sockfd);
  value = db_readc(mydb, key);
  vbp(3, "sock %d value %s\n", sockfd, value == NULL ? "null" : value);
  if (value != NULL) {
    free(value);
    return 1;
  } else
    return 0;
}
// action

char* db_getSockId(lvldb_t *mydb, char* pidkey, int sock) {
  char key[KEYLEN];
  sprintf(key, "pid.%s.sk2id.%d", pidkey, sock);
  char* sockid = db_readc(mydb, key);
  if (sockid == NULL) {
    free(sockid);
    sprintf(key, "pid.%s.ac2id.%d", pidkey, sock);
    sockid = db_readc(mydb, key);
  }
  if (sockid != NULL)
    vbp(3, "pidkey %s, sock %d -> sockid %s [%zu]\n", 
        pidkey, sock, sockid, strlen(sockid));
  return sockid;
}

void db_removeSockId(lvldb_t *mydb, char* pidkey, int sock) {
  char key[KEYLEN];
  char *err = NULL, deleted = 0;
  
  sprintf(key, "pid.%s.sk2id.%d", pidkey, sock);
  leveldb_delete(mydb->db, mydb->woptions, key, strlen(key), &err);
  deleted += err == NULL ? 1 : 0;
  leveldb_free(err); err = NULL;
  
  sprintf(key, "pid.%s.ac2id.%d", pidkey, sock);
  leveldb_delete(mydb->db, mydb->woptions, key, strlen(key), &err);
  deleted += err == NULL ? 1 : 0;
  leveldb_free(err); err = NULL;
  
  vbp(3, "pidkey %s, sock %d deleted %d\n", pidkey, sock, deleted);
}

ull_t db_getPkgCounterInc(lvldb_t *mydb, char* pidkey, char* sockid, int action) {
  char key[KEYLEN];
  sprintf(key, "prv.pid.%s.skid.%s.act.%d", pidkey, sockid, action);
  return db_getCounterInc(mydb, key);
}

void db_get_pid_sock(lvldb_t *mydb, long pid, int sockfd, char **pidkey, char **sockid) {
  *pidkey = NULL;
  *sockid = NULL;
  *pidkey = db_read_pid_key(mydb, pid);
  if (*pidkey == NULL) return;
  *sockid = db_getSockId(mydb, *pidkey, sockfd);
  if (*sockid == NULL) {
    free(*pidkey);
    *pidkey = db_read_real_pid_key(mydb, pid);
    if (*pidkey == NULL) return;
    *sockid = db_getSockId(mydb, *pidkey, sockfd);
    if (*sockid == NULL) {
      free(*pidkey);
      return;
    }
  }
}

void db_write_sock_action(lvldb_t *mydb, long pid, int sockfd, \
                       const char *buf, long len_param, int flags, \
                       long len_result, int action, void* msg) {
  char key[KEYLEN];
  int old_action = action;
  action = action == SOCK_RECVMSG ? SOCK_RECV : action;
  char *pidkey, *sockid;
  db_get_pid_sock(mydb, pid, sockfd, &pidkey, &sockid);
  if (pidkey == NULL || sockid == NULL) return;
  ull_t pkgid = db_getPkgCounterInc(mydb, pidkey, sockid, action);
  ull_t usec = getusec();
  
  if (old_action == SOCK_RECVMSG) {
    struct msghdr *mh = msg;
    sprintf(key, "prv.pid.%s.skid.%s.act.%d.n.%llu.msg.msg_flags", \
            pidkey, sockid, action, pkgid);
    db_nwrite(mydb, key, (char*) &mh->msg_flags, sizeof(int));
  }

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
  //~ vbp(3, "%s -> %zd, checksum %u ", key, len_result, checksum(buf, len_result));
  vbp(3, "checksum %u ", checksum(buf, len_result));
  if (CDE_verbose_mode >= 3) printbuf(buf, len_result);
  
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
    int sockfd, char* addr, int addr_len, long u_rval, char *ips) {
  char key[KEYLEN], idkey[KEYLEN];
  char *pidkey = db_read_pid_key(mydb, pid);
  if (pidkey == NULL) return;
  ull_t usec = getusec();
  
  // prv.sock.$(pid.usec).newfd.$usec.$sockfd.$addr_len.$u_rval -> $(addr) [1]
  sprintf(key, "prv.sock.%s.newfd.%llu.%d.%d.%ld", \
          pidkey, usec, sockfd, addr_len, u_rval);
  db_nwrite(mydb, key, addr, addr_len);
  
  // prv.pid.$(pid.usec).sockid.$n -> $key[1]
  ull_t sockn = db_getConnectCounterInc(mydb, pidkey);
  db_setSockConnectId(mydb, pidkey, sockfd, sockn);
  sprintf(idkey, "prv.pid.%s.sockid.%llu", pidkey, sockn);
  db_write(mydb, idkey, key);
  
  vbp(3, "%s -> %s\n", idkey, key);
  
  sprintf(key, "prv.sock.%s.newfdips.%llu.%d.%d.%ld", \
          pidkey, usec, sockfd, addr_len, u_rval);
  db_write(mydb, key, ips);
  
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
  vbp(3, "pidkey %s\n", pidkey);
}
ull_t db_getListenCounterInc(lvldb_t *mydb, char* pidkey) {
  char key[KEYLEN];
  sprintf(key, "prv.pid.%s.listenn", pidkey);
  return db_getCounterInc(mydb, key);
}
void db_setListenId(lvldb_t *mydb, char* pidkey, int sock, ull_t sockid) {
  char key[KEYLEN];
  sprintf(key, "pid.%s.lssk2id.%d", pidkey, sock);
  vbp(3, "pidkey %s, sock %d -> listenid %lld\n", pidkey, sock, sockid);
  db_nwrite(mydb, key, (char*) &sockid, sizeof(ull_t));
}
ull_t db_getListenId(lvldb_t *mydb, char* pidkey, int sock) {
  char key[KEYLEN];
  ull_t sockid;
  sprintf(key, "pid.%s.lssk2id.%d", pidkey, sock);
  db_read_ull(mydb, key, &sockid);
  vbp(3, "pidkey %s, sock %d -> lssk2id %llu\n", pidkey, sock, sockid);
  return sockid;
}
void db_write_listen_prov(lvldb_t *mydb, int pid, int sock, int backlog, int result) {
  char key[KEYLEN],value[KEYLEN];
  char *pidkey = db_read_real_pid_key(mydb, pid);
  ull_t listenn = db_getListenCounterInc(mydb, pidkey);
  db_setListenId(mydb, pidkey, sock, listenn);
  sprintf(key, "prv.pid.%s.listenid.%llu", pidkey, listenn);
  sprintf(value, "%d.%d.%d", result, sock, backlog);
  db_write(mydb, key, value);
  db_setupAcceptCounter(mydb, pidkey, listenn);
  free(pidkey);
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

/* =====
 * accept socket
 * int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
 */
ull_t db_getAcceptCounterInc(lvldb_t *mydb, char* pidkey, ull_t listenid) {
  char key[KEYLEN];
  sprintf(key, "prv.pid.%s.listenid.%llu.acceptn", pidkey, listenid);
  return db_getCounterInc(mydb, key);
}
void db_setAcceptId(lvldb_t *mydb, char* pidkey, int client_sock, ull_t listenid, ull_t acceptid) {
  char key[KEYLEN], value[KEYLEN];
  sprintf(key, "pid.%s.ac2id.%d", pidkey, client_sock);
  sprintf(value, "%llu_%llu", listenid, acceptid);
  db_write(mydb, key, value);
  vbp(1, "pidkey %s, client_sock %d -> listenid %llu, acceptid %lld\n", 
        pidkey, client_sock, listenid, acceptid);
}
void db_write_accept_prov(lvldb_t *mydb, int pid, int lssock, 
      char* addrbuf, int len, ull_t client_sock, char* ips) {
  char key[KEYLEN];
  char *pidkey = db_read_real_pid_key(mydb, pid);
  ull_t listenid = db_getListenId(mydb, pidkey, lssock);
  ull_t acceptid = db_getAcceptCounterInc(mydb, pidkey, listenid);
  sprintf(key, "prv.pid.%s.listenid.%llu.accept.%llu.addr", pidkey, listenid, acceptid);
  db_nwrite(mydb, key, addrbuf, len);
  ull_t usec = getusec();
  sprintf(key, "prv.pid.%s.skac.%llu.listenid.%llu.accept.%llu.ips", pidkey, usec, listenid, acceptid);
  db_write(mydb, key, ips);
  
  db_setAcceptId(mydb, pidkey, client_sock, listenid, acceptid);
  db_setupSockAcceptCounter(mydb, pidkey, client_sock, listenid, acceptid);
  vbp(1, "pidkey %s, listenid %llu, acceptid %llu, listen sock %d, client_sock %lld\n", 
        pidkey, listenid, acceptid, lssock, client_sock);
  free(pidkey);
}

void db_setupAcceptCounter(lvldb_t *mydb, char* pidkey, ull_t listenid) {
  char key[KEYLEN];
  sprintf(key, "prv.pid.%s.listenid.%llu.acceptn", pidkey, listenid);
  ull_t zero = 0;
  db_nwrite(mydb, key, (char*) &zero, sizeof(ull_t));
  vbp(3, "pidkey %s, listenid %llu\n", pidkey, listenid);
}

void db_write_getsockname_prov(lvldb_t *mydb, int pid, int sock, char* addrbuf, int len, ull_t res) {
  char key[KEYLEN];
  char *pidkey = db_read_real_pid_key(mydb, pid);
  ull_t listenid = db_getListenId(mydb, pidkey, sock);
  sprintf(key, "prv.pid.%s.listenid.%llu.gsn.addr", pidkey, listenid);
  db_nwrite(mydb, key, addrbuf, len);
  vbp(0, "sock %d key %s [%d]: ", sock, key, len);
  printbuf(addrbuf, len);
  
  sprintf(key, "prv.pid.%s.listenid.%llu.gsn", pidkey, listenid);
  db_nwrite(mydb, key, (char*) &res, sizeof(ull_t));
  
  free(pidkey);
}

/* =====
 * sock gets closed
 */
void db_remove_sock(lvldb_t *mydb, long pid, int sockfd) {
  db_removeCapturedSock(mydb, sockfd);
  // remove the corresponding (if set) sockid in currdb
  // to allow detection of socket or file fd
  char *pidkey = db_read_pid_key(mydb, pid);
  db_removeSockId(mydb, pidkey, sockfd);
  free(pidkey);
}

char* db_getSendRecvResult(lvldb_t *mydb, int action, 
    char* pidkey, char* sockid, ull_t pkgid, ull_t *presult, void *msg) {
  char key[KEYLEN];
  size_t len;
  
  if (action == SOCK_RECVMSG) {
    sprintf(key, "prv.pid.%s.skid.%s.act.%d.n.%llu.msg.msg_flags", pidkey, sockid, action, pkgid);
    int *v = (void*) db_nread(mydb, key, &len);
    if (v != NULL) {
      ((struct msghdr*) msg)->msg_flags = *v;
      free(v);
    }
    action = SOCK_RECV;
  }
  
  // prv.pid.$(pid.usec).skid.$sockid.act.$action.n.$counter -> $syscall_result
  sprintf(key, "prv.pid.%s.skid.%s.act.%d.n.%llu", pidkey, sockid, action, pkgid);
  if (db_read_ull(mydb, key, presult) == 0) {
    vbp(3, "%s\n", key);
    return NULL; // error getting the result
  }

  if (action == SOCK_RECV) {
    sprintf(key, "prv.pid.%s.skid.%s.act.%d.n.%llu.buff", \
          pidkey, sockid, action, pkgid);
    char *res = db_nread(mydb, key, &len); // len might be != *presult in the case "-1"
    vbp(3, "%s -> %lld checksum: %u ", key, *presult, checksum(res, len));
    if (CDE_verbose_mode >= 3) printbuf(res, len);
    return res;
  } else
    vbp(3, "%s -> %lld\n", key, *presult);
  return NULL; // SOCK_SEND and "other?" cases
}

int db_getSockResult(lvldb_t *mydb, char* pidkey, int sockid) {
  // prv.pid.$(pid.usec).sockid.$n
  // prv.sock.$(pid.usec).newfd.$usec.$sockfd.$addr_len.$u_rval
  char *err = NULL, key[KEYLEN];
  char *read;
  size_t read_len;
  int u_rval;

  sprintf(key, "prv.pid.%s.sockid.%d", pidkey, sockid);
  read = leveldb_get(mydb->db, mydb->roptions, key, strlen(key), &read_len, &err);
  if (read == NULL) {
    fprintf(stderr, "Cannot find key '%s'\n", key);
    print_trace();
  }
  vbp(3, "%s -> %s\n", key, read);
  //~ sscanf(read, "prv.sock.%*d.%*llu.newfd.%*llu.%*d.%*d.%d", &u_rval);
  sscanf(read, "prv.sock.%*d.%*u.newfd.%*u.%*d.%*d.%d", &u_rval);
  return u_rval;
}

// remote host properties
int db_hasPTUonRemoteHost(lvldb_t *mydb, char* remotehost) {
  char key[KEYLEN];
  ull_t value = FALSE;
  sprintf(key, "host.%s.hasptu", remotehost);
  if (db_read_ull(mydb, key, &value)==0) {
    vbp(2, "%s nokey\n", remotehost);
    return FALSE;
  }
  vbp(2, "%s %lld\n", remotehost, value);
  return value > 0 ? TRUE : FALSE;
}

void db_setPTUonRemoteHost(lvldb_t *mydb, char* remotehost) {
  char key[KEYLEN];
  ull_t value = TRUE;
  sprintf(key, "host.%s.hasptu", remotehost);
  db_nwrite(mydb, key, (char*) &value, sizeof(ull_t));
  vbp(2, "%s %lld\n", remotehost, value);
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


