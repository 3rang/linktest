#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "linktest.h"

/*
 * Broadcast a magic string on one selected local interface and collect peers.
 */
int discover_peers(const char *local_ip, char peers[][INET_ADDRSTRLEN], int max_peers)
{
	int found = 0;
	int rsock = socket(AF_INET, SOCK_DGRAM, 0);
	if (rsock < 0) {
		perror("socket");
		return -1;
	}
	int ssock = socket(AF_INET, SOCK_DGRAM, 0);
	if (ssock < 0) {
		perror("socket");
		plat_close(rsock);
		return -1;
	}

	int on = 1;
	setsockopt(rsock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	setsockopt(ssock, SOL_SOCKET, SO_BROADCAST, (char *)&on, sizeof(on));

	struct sockaddr_in raddr = {0};
	raddr.sin_family = AF_INET;
	raddr.sin_port = htons(DISCOVER_PORT);
	raddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(rsock, (struct sockaddr *)&raddr, sizeof(raddr)) < 0) {
		perror("bind recv");
		plat_close(ssock);
		plat_close(rsock);
		return -1;
	}

	struct sockaddr_in saddr = {0};
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(0);
	if (inet_pton(AF_INET, local_ip, &saddr.sin_addr) != 1) {
		fprintf(stderr, "Invalid local IP: %s\n", local_ip);
		plat_close(ssock);
		plat_close(rsock);
		return -1;
	}
	if (bind(ssock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		perror("bind send");
		plat_close(ssock);
		plat_close(rsock);
		return -1;
	}

	/* 1-second recv timeout so we can keep re-broadcasting */
	struct timeval tv = {1, 0};
	setsockopt(rsock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));

	struct sockaddr_in bcast = {0};
	bcast.sin_family = AF_INET;
	bcast.sin_port = htons(DISCOVER_PORT);
	bcast.sin_addr.s_addr = INADDR_BROADCAST;

	char beacon[64];
	snprintf(beacon, sizeof(beacon), "%s|%s", MAGIC, local_ip);

	printf("Looking for peer (port %d)...\n", DISCOVER_PORT);
	printf("Using local IP: %s\n", local_ip);

	double deadline = plat_now() + TIMEOUT_SEC;

	while (plat_now() < deadline) {
		sendto(ssock, beacon, (int)strlen(beacon), 0,
		       (struct sockaddr *)&bcast, sizeof(bcast));

		char buf[64];
		struct sockaddr_in from = {0};
		socklen_t fromlen = sizeof(from);

		ssize_t n = recvfrom(rsock, buf, sizeof(buf) - 1, 0,
		                     (struct sockaddr *)&from, &fromlen);
		if (n <= 0)
			continue;
		buf[n] = '\0';
		if (strncmp(buf, MAGIC "|", MAGIC_LEN + 1) != 0)
			continue;

		char who[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &from.sin_addr, who, sizeof(who));

		char *msg_ip = strchr(buf, '|');
		if (!msg_ip)
			continue;
		msg_ip++;

		if (strcmp(msg_ip, local_ip) == 0)
			continue;

		int exists = 0;
		for (int i = 0; i < found; i++) {
			if (strcmp(peers[i], msg_ip) == 0) {
				exists = 1;
				break;
			}
		}
		if (exists)
			continue;

		if (found < max_peers) {
			snprintf(peers[found], INET_ADDRSTRLEN, "%s", msg_ip);
			printf("Found peer: %s (via %s)\n", peers[found], who);
			found++;
		}
	}

	plat_close(ssock);
	plat_close(rsock);
	return found;
}
