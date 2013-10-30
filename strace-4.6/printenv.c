#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h> 
#include <fcntl.h>

#define MAXPATHLEN 255

static void CDE_load_environment_vars() {
  static char cde_full_environment_abspath[MAXPATHLEN];
  strcat(cde_full_environment_abspath, "./cde.full-environment");

  struct stat env_file_stat;
  if (stat(cde_full_environment_abspath, &env_file_stat)) {
    perror(cde_full_environment_abspath);
    exit(1);
  }

  int full_environment_fd = open(cde_full_environment_abspath, O_RDONLY);

  void* environ_start =
    (char*)mmap(0, env_file_stat.st_size, PROT_READ, MAP_PRIVATE, full_environment_fd, 0);

  char* environ_str = (char*)environ_start;
  while (*environ_str) {
    int environ_strlen = strlen(environ_str);

    // format: "name=value"
    // note that 'value' might itself contain '=' characters,
    // so only split on the FIRST '='

    char* cur = strdup(environ_str); // strtok needs to mutate
    char* name = NULL;
    char* val = NULL;

    int count = 0;
    char* p;
    int start_index_of_value = 0;

    // strtok is so dumb!!!  need to munch through the entire string
    // before it restores the string to its original value
    for (p = strtok(cur, "="); p; p = strtok(NULL, "=")) {
      if (count == 0) {
        name = strdup(p);
      }
      else if (count == 1) {
        start_index_of_value = (p - cur);
      }

      count++;
    }

    if (start_index_of_value) {
      val = strdup(environ_str + start_index_of_value);
    }

      if (val) {
        //setenv(name, val, 1);
        printf("%s='%s'\n", name, val);
      }
      else {
        //setenv(name, "", 1);
        printf("%s=''\n", name, val);
      }

    if (name) free(name);
    if (val) free(val);
    free(cur);

    // every string in cde_full_environment_abspath is
    // null-terminated, so this advances to the next string
    environ_str += (environ_strlen + 1);
  }

  munmap(environ_start, env_file_stat.st_size);
  close(full_environment_fd);
}

int main() {
  CDE_load_environment_vars();
  return 0;
}

