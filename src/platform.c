#include <stdio.h>
#include <string.h>
#include "platform.h"

void plat_init(void)
{
#ifdef _WIN32
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
}

void plat_cleanup(void)
{
#ifdef _WIN32
	WSACleanup();
#endif
}

void plat_close(int fd)
{
#ifdef _WIN32
	closesocket(fd);
#else
	close(fd);
#endif
}

void plat_sleep_ms(int ms)
{
#ifdef _WIN32
	Sleep(ms);
#else
	usleep(ms * 1000);
#endif
}

double plat_now(void)
{
#ifdef _WIN32
	LARGE_INTEGER freq, count;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);
	return (double)count.QuadPart / (double)freq.QuadPart;
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e9;
#endif
}

void plat_get_local_ip(char *buf, int buflen)
{
	buf[0] = '\0';
#ifdef _WIN32
	char hostname[256];
	struct addrinfo hints = {0};
	struct addrinfo *res = NULL;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	if (gethostname(hostname, sizeof(hostname)) == 0 &&
	    getaddrinfo(hostname, NULL, &hints, &res) == 0 && res) {
		struct sockaddr_in *sa = (struct sockaddr_in *)res->ai_addr;
		inet_ntop(AF_INET, &sa->sin_addr, buf, buflen);
		freeaddrinfo(res);
	}
#else
	struct ifaddrs *list, *p;
	if (getifaddrs(&list) == -1)
		return;
	for (p = list; p; p = p->ifa_next) {
		if (!p->ifa_addr || p->ifa_addr->sa_family != AF_INET)
			continue;
		if (p->ifa_flags & IFF_LOOPBACK)
			continue;
		struct sockaddr_in *sa = (struct sockaddr_in *)p->ifa_addr;
		inet_ntop(AF_INET, &sa->sin_addr, buf, buflen);
		break;
	}
	freeifaddrs(list);
#endif
}

int plat_get_all_local_ips(char ips[][INET_ADDRSTRLEN], int max)
{
	int count = 0;
#ifdef _WIN32
	char hostname[256];
	struct addrinfo hints = {0};
	struct addrinfo *res = NULL, *p;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	if (gethostname(hostname, sizeof(hostname)) == 0 &&
	    getaddrinfo(hostname, NULL, &hints, &res) == 0) {
		for (p = res; p && count < max; p = p->ai_next) {
			struct sockaddr_in *sa = (struct sockaddr_in *)p->ai_addr;
			inet_ntop(AF_INET, &sa->sin_addr, ips[count], INET_ADDRSTRLEN);
			count++;
		}
		freeaddrinfo(res);
	}
#else
	struct ifaddrs *list, *p;
	if (getifaddrs(&list) == -1)
		return 0;
	for (p = list; p && count < max; p = p->ifa_next) {
		if (!p->ifa_addr || p->ifa_addr->sa_family != AF_INET)
			continue;
		if (p->ifa_flags & IFF_LOOPBACK)
			continue;
		struct sockaddr_in *sa = (struct sockaddr_in *)p->ifa_addr;
		inet_ntop(AF_INET, &sa->sin_addr, ips[count], INET_ADDRSTRLEN);
		count++;
	}
	freeifaddrs(list);
#endif
	return count;
}
