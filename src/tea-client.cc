#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9999
#define MAX_MSG_LEN 64


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
	int rv;
	sockaddr_in server_addr;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port   = htons(port);
	inet_pton(AF_INET, addr, &server_addr.sin_addr.s_addr);

	udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// yes, connect an udp socket.
	rv = connect(udp_sock, (sockaddr *) &server_addr, sizeof server_addr);
	if (rv < 0) {
		perror("connect");
		close(udp_sock);
		exit(EXIT_FAILURE);
	}

	tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp_sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	rv = connect(tcp_sock, (sockaddr *) &server_addr, sizeof server_addr);
	if (rv < 0) {
		perror("connect");
		close(tcp_sock);
		exit(EXIT_FAILURE);
	}
}
