#include <sys/ioctl.h>
#include <net/if.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <linux/limits.h>

int getMAC(char* ret)
{
    struct ifreq ifr;
    struct ifconf ifc;
    char buf[1024];
    int success = 0, i;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1) { /* handle error*/ };

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) { /* handle error */ }

    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

    for (; it != end; ++it) {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    success = 1;
                    break;
                }
            }
        }
        else { /* handle error */ }
    }

    unsigned char mac_address[6];

    if (success) {
        memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
        for (i=0; i<6; i++) {
          sprintf(ret+i+i, mac_address[i]<10 ? "0%x" : "%x", mac_address[i]);
        }
        return 1;
    } else
        return 0;
}

unsigned long
hash(unsigned char *str)
{ // djb2
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

void i62a(char* ret, unsigned long num) {
  const static char* b62 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  const static int BASE = 62;
  int i=0;
  while (num > 0) {
    sprintf(ret+i, "%c", b62[num % BASE]);
    num = num / BASE;
    i++;
  }
}

void getUUID(char* ret) {
  char mac[13];
  FILE *fp;
  char uname[PATH_MAX], huname[PATH_MAX], hmac[PATH_MAX];
  fp = popen("uname -a", "r");
  fgets(uname, PATH_MAX, fp);
  pclose(fp);
  i62a(huname, hash(uname));
  if (getMAC(mac)) {
    i62a(hmac, hash(mac));
    sprintf(ret, "%s_%s", huname, hmac);
  } else {
    sprintf(ret, "%s_nomac", huname);
  }
}

void main() {
  char uuid[PATH_MAX];
  getUUID(uuid);
  printf("%s\n", uuid);
}
 
