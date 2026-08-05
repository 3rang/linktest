#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "linktest.h"

/*
 * Print a one-line throughput report for the interval just ended.
 */
static void print_interval(int num, double mbps)
{
	printf("  [%2d-%2d sec]  %.1f Mbps\n", num, num + 1, mbps);
}

static void print_summary(double secs, long long bytes, double mbps)
{
	printf("\n--- Result ---\n");
	printf("  Time:  %.1f s\n", secs);
	printf("  Data:  %.2f MB\n", bytes / 1e6);
	printf("  Speed: %.1f Mbps\n", mbps);
}

/* ---- Server (receiver) ---- */

int run_server(void)
{
	int lsock = socket(AF_INET, SOCK_STREAM, 0);
	if (lsock < 0) {
		perror("socket");
		return -1;
	}

	int on = 1;
	setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(DATA_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(lsock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		plat_close(lsock);
		return -1;
	}
	listen(lsock, 1);

	printf("Receiver listening on port %d\n", DATA_PORT);

	struct timeval tv = {TIMEOUT_SEC, 0};
	setsockopt(lsock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));

	int csock = accept(lsock, NULL, NULL);
	if (csock < 0) {
		perror("accept (timed out?)");
		plat_close(lsock);
		return -1;
	}
	printf("Peer connected, receiving...\n\n");

	/* shorter timeout during transfer — just to detect dead connections */
	tv.tv_sec = 2;
	setsockopt(csock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));

	char *buf = malloc(BUF_SIZE);
	if (!buf) {
		plat_close(csock);
		plat_close(lsock);
		return -1;
	}

	double t0 = plat_now();
	double t_int = t0;
	long long total = 0, interval = 0;
	int inum = 0;

	while (1) {
		ssize_t n = recv(csock, buf, BUF_SIZE, 0);
		if (n <= 0)
			break;
		total += n;
		interval += n;

		double now = plat_now();
		if (now - t_int >= INTERVAL_SEC) {
			print_interval(inum, (interval * 8.0) / ((now - t_int) * 1e6));
			inum++;
			interval = 0;
			t_int = now;
		}
		if (now - t0 >= DURATION_SEC)
			break;
	}

	double elapsed = plat_now() - t0;
	print_summary(elapsed, total, (total * 8.0) / (elapsed * 1e6));

	free(buf);
	plat_close(csock);
	plat_close(lsock);
	return 0;
}

/* ---- Client (sender) ---- */

int run_client(const char *peer_ip)
{
	printf("Sender → %s:%d\n", peer_ip, DATA_PORT);
	plat_sleep_ms(500); /* let server open the port */

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		return -1;
	}

	int on = 1;
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on));

	struct timeval tv = {TIMEOUT_SEC, 0};
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));

	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(DATA_PORT);
	inet_pton(AF_INET, peer_ip, &addr.sin_addr);

	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		plat_close(sock);
		return -1;
	}
	printf("Connected, sending for %d seconds...\n\n", DURATION_SEC);

	char *buf = malloc(BUF_SIZE);
	if (!buf) {
		plat_close(sock);
		return -1;
	}
	memset(buf, 'A', BUF_SIZE);

	double t0 = plat_now();
	double t_int = t0;
	long long total = 0, interval = 0;
	int inum = 0;

	while (plat_now() - t0 < DURATION_SEC) {
		ssize_t n = send(sock, buf, BUF_SIZE, 0);
		if (n <= 0)
			break;
		total += n;
		interval += n;

		double now = plat_now();
		if (now - t_int >= INTERVAL_SEC) {
			print_interval(inum, (interval * 8.0) / ((now - t_int) * 1e6));
			inum++;
			interval = 0;
			t_int = now;
		}
	}

	double elapsed = plat_now() - t0;
	print_summary(elapsed, total, (total * 8.0) / (elapsed * 1e6));

	free(buf);
	plat_close(sock);
	return 0;
}
