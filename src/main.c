#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "linktest.h"

static int pick_role(const char *my_ip, const char *peer_ip)
{
	/* lower IP string becomes the receiver */
	return strcmp(my_ip, peer_ip) < 0;
}

int main(int argc, char *argv[])
{
	char peer[INET_ADDRSTRLEN] = {0};
	char me[INET_ADDRSTRLEN] = {0};
	int force_server = 0, force_client = 0;

	plat_init();
	printf("linktest v%s\n\n", LINKTEST_VERSION);

	/* parse args: optional IP and optional -s / -c */
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-s") == 0)
			force_server = 1;
		else if (strcmp(argv[i], "-c") == 0)
			force_client = 1;
		else
			snprintf(peer, sizeof(peer), "%s", argv[i]);
	}

	/* discover if no IP given */
	if (!peer[0]) {
		if (discover_peer(peer, sizeof(peer)) < 0) {
			fprintf(stderr, "No peer found.\n");
			fprintf(stderr, "Run 'linktest' on another machine, or: linktest <ip>\n");
			plat_cleanup();
			return 1;
		}
	}

	plat_get_local_ip(me, sizeof(me));
	printf("Local: %s\n", me);
	printf("Peer:  %s\n\n", peer);

	int is_server;
	if (force_server)       is_server = 1;
	else if (force_client)  is_server = 0;
	else                    is_server = pick_role(me, peer);

	int ret = is_server ? run_server() : run_client(peer);

	plat_cleanup();
	return ret;
}
