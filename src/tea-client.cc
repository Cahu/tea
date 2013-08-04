#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9999
#define MAX_MSG_LEN 64

#define CHK(msg, cmd)       \
	if (0 > (cmd)) {        \
		perror(msg);        \
		exit(EXIT_FAILURE); \
	}

static int tcp_sock;
static int udp_sock;

void init_connect(const char *, unsigned short);


int main(int argc, char *argv[])
{
	if (argc < 2) {
		init_connect("127.0.0.1", PORT);
	} else {
		init_connect(argv[1], PORT);
	}

	close(tcp_sock);
	close(udp_sock);

	exit(EXIT_SUCCESS);
}



void init_connect(const char *addr, unsigned short port)
{
	sockaddr_in saddr;

	saddr.sin_family = AF_INET;
	saddr.sin_port   = htons(port);
	inet_pton(AF_INET, addr, &saddr.sin_addr.s_addr);

	CHK("socket", udp_sock = socket(AF_INET, SOCK_DGRAM, 0));
	CHK("connect", connect(udp_sock, (sockaddr *) &saddr, sizeof saddr));

	CHK("socket", tcp_sock = socket(AF_INET, SOCK_STREAM, 0));
	CHK("connect", connect(tcp_sock, (sockaddr *) &saddr, sizeof saddr));
}
