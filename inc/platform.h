/*
 * linktest - zero-config throughput tester
 *
 * Platform abstraction for sockets and timing.
 * Keeps the rest of the code free from #ifdef clutter.
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <iphlpapi.h>
  #include <windows.h>
  typedef int socklen_t;
  typedef SSIZE_T ssize_t;
#else
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <sys/socket.h>
  #include <sys/time.h>
  #include <netinet/tcp.h>
  #include <net/if.h>
  #include <ifaddrs.h>
  #include <time.h>
#endif

void    plat_init(void);
void    plat_cleanup(void);
void    plat_close(int fd);
void    plat_sleep_ms(int ms);
double  plat_now(void);
void    plat_get_local_ip(char *buf, int buflen);

#endif
