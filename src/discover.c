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
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("socket");
		return -1;
	}

	int on = 1;
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&on, sizeof(on));
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

	struct sockaddr_in local = {0};
	local.sin_family = AF_INET;
	local.sin_port = htons(DISCOVER_PORT);
	if (inet_pton(AF_INET, local_ip, &local.sin_addr) != 1) {
		fprintf(stderr, "Invalid local IP: %s\n", local_ip);
		plat_close(sock);
		return -1;
	}

	if (bind(sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
		perror("bind");
		plat_close(sock);
		return -1;
	}

	/* 1-second recv timeout so we can keep re-broadcasting */
	struct timeval tv = {1, 0};
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));

	struct sockaddr_in bcast = {0};
	bcast.sin_family = AF_INET;
	bcast.sin_port = htons(DISCOVER_PORT);
	bcast.sin_addr.s_addr = INADDR_BROADCAST;

	printf("Looking for peer (port %d)...\n", DISCOVER_PORT);
	printf("Using local IP: %s\n", local_ip);

	double deadline = plat_now() + TIMEOUT_SEC;

	while (plat_now() < deadline) {
		sendto(sock, MAGIC, MAGIC_LEN, 0,
		       (struct sockaddr *)&bcast, sizeof(bcast));

		char buf[64];
		struct sockaddr_in from = {0};
		socklen_t fromlen = sizeof(from);

		ssize_t n = recvfrom(sock, buf, sizeof(buf), 0,
		                     (struct sockaddr *)&from, &fromlen);
		if (n != MAGIC_LEN)
			continue;
		if (memcmp(buf, MAGIC, MAGIC_LEN) != 0)
			continue;

		char who[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &from.sin_addr, who, sizeof(who));

		if (strcmp(who, local_ip) == 0)
			continue;

		int exists = 0;
		for (int i = 0; i < found; i++) {
			if (strcmp(peers[i], who) == 0) {
				exists = 1;
				break;
			}
		}
		if (exists)
			continue;

		if (found < max_peers) {
			snprintf(peers[found], INET_ADDRSTRLEN, "%s", who);
			printf("Found peer: %s\n", peers[found]);
			found++;
		}
	}

	plat_close(sock);
	return found;
}
