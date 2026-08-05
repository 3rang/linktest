/*
 * linktest - zero-config throughput tester
 *
 * Common defines used across all source files.
 */

#ifndef LINKTEST_H
#define LINKTEST_H

#define LINKTEST_VERSION  "0.1"

#define DISCOVER_PORT  5199
#define DATA_PORT      5200
#define MAGIC          "LINKTEST1"
#define MAGIC_LEN      9

#define BUF_SIZE       (128 * 1024)   /* 128 KB send/recv chunk */
#define DURATION_SEC   5
#define TIMEOUT_SEC    5
#define INTERVAL_SEC   1

/* discover.c */
int discover_peers(const char *local_ip,
			  char peers[][INET_ADDRSTRLEN],
			  int max_peers);

/* tput.c */
int run_server_on(const char *local_ip);
int run_client(const char *peer_ip);

#endif
