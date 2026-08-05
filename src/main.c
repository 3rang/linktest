#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "platform.h"
#include "linktest.h"

static int pick_role(const char *my_ip, const char *peer_ip)
{
	/* lower IP string becomes the receiver */
	return strcmp(my_ip, peer_ip) < 0;
}

static int read_choice(int min, int max)
{
	char line[64];
	long n;
	char *end;

	while (1) {
		printf("Select [%d-%d]: ", min, max);
		if (!fgets(line, sizeof(line), stdin))
			return -1;
		n = strtol(line, &end, 10);
		if (end == line)
			continue;
		if (n >= min && n <= max)
			return (int)n;
	}
}

static int choose_local_ip(char *out_ip, int out_len, int auto_mode)
{
	char ips[MAX_LOCAL_IPS][INET_ADDRSTRLEN];
	int n = plat_get_all_local_ips(ips, MAX_LOCAL_IPS);

	if (n <= 0)
		return -1;

	printf("Available local IPv4 addresses:\n");
	for (int i = 0; i < n; i++)
		printf("  %d) %s\n", i + 1, ips[i]);

	if (auto_mode || n == 1) {
		snprintf(out_ip, out_len, "%s", ips[0]);
		printf("Using local IP: %s\n", out_ip);
		return 0;
	}

	int choice = read_choice(1, n);
	if (choice < 0)
		return -1;

	snprintf(out_ip, out_len, "%s", ips[choice - 1]);
	printf("Using local IP: %s\n", out_ip);
	return 0;
}

static int choose_peer(const char *local_ip, char *out_peer, int out_len, int auto_mode)
{
	char peers[32][INET_ADDRSTRLEN];
	int n = discover_peers(local_ip, peers, 32);
	if (n <= 0)
		return -1;

	printf("\nDiscovered peers:\n");
	for (int i = 0; i < n; i++)
		printf("  %d) %s\n", i + 1, peers[i]);

	if (auto_mode || n == 1) {
		snprintf(out_peer, out_len, "%s", peers[0]);
		printf("Using peer: %s\n", out_peer);
		return 0;
	}

	int choice = read_choice(1, n);
	if (choice < 0)
		return -1;

	snprintf(out_peer, out_len, "%s", peers[choice - 1]);
	printf("Using peer: %s\n", out_peer);
	return 0;
}

int main(int argc, char *argv[])
{
	char peer[INET_ADDRSTRLEN] = {0};
	char me[INET_ADDRSTRLEN] = {0};
	int force_server = 0, force_client = 0;
	int auto_mode = 0;

	plat_init();
	printf("linktest v%s\n\n", LINKTEST_VERSION);

	/* parse args: optional peer IP and optional -s / -c */
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-s") == 0)
			force_server = 1;
		else if (strcmp(argv[i], "-c") == 0)
			force_client = 1;
		else if (strcmp(argv[i], "--auto") == 0)
			auto_mode = 1;
		else
			snprintf(peer, sizeof(peer), "%s", argv[i]);
	}

	if (choose_local_ip(me, sizeof(me), auto_mode) < 0) {
		fprintf(stderr, "No usable local IPv4 interface found.\n");
		plat_cleanup();
		return 1;
	}

	/* discover peers if no target IP given */
	if (!peer[0] && choose_peer(me, peer, sizeof(peer), auto_mode) < 0) {
		fprintf(stderr, "No peer found.\n");
		fprintf(stderr, "Run linktest on another machine, or: linktest <peer-ip>\n");
		plat_cleanup();
		return 1;
	}

	/* loopback target should bind loopback so local self-tests work */
	if (strncmp(peer, "127.", 4) == 0)
		snprintf(me, sizeof(me), "%s", "127.0.0.1");

	printf("Local: %s\n", me);
	printf("Peer:  %s\n\n", peer);

	int is_server;
	if (force_server)       is_server = 1;
	else if (force_client)  is_server = 0;
	else                    is_server = pick_role(me, peer);

	int ret = is_server ? run_server_on(me) : run_client(peer);

	plat_cleanup();
	return ret;
}
