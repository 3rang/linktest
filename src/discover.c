#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "linktest.h"

/*
 * Broadcast a magic string on UDP, listen for someone else doing the same.
 * First foreign IP we hear back is our peer.
 */
int discover_peer(char *peer_ip, int buflen)
{
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
	local.sin_addr.s_addr = INADDR_ANY;

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

	/* collect all our own IPs so we don't mistake another adapter for a peer */
	char my_ips[MAX_LOCAL_IPS][INET_ADDRSTRLEN];
	int n_ips = plat_get_all_local_ips(my_ips, MAX_LOCAL_IPS);

	printf("Looking for peer (port %d)...\n", DISCOVER_PORT);
	if (n_ips > 0)
		printf("My IP: %s", my_ips[0]);
	for (int i = 1; i < n_ips; i++)
		printf(", %s", my_ips[i]);
	printf("\n");

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

		/* ignore packets from any of our own addresses */
		int is_self = 0;
		for (int i = 0; i < n_ips; i++) {
			if (strcmp(who, my_ips[i]) == 0) {
				is_self = 1;
				break;
			}
		}
		if (is_self)
			continue;

		snprintf(peer_ip, buflen, "%s", who);
		printf("Found: %s\n", peer_ip);
		plat_close(sock);
		return 0;
	}

	plat_close(sock);
	return -1;
}
