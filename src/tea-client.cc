#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9999


int main(int argc, char *argv[])
{
	int rv;
	int tcp_sock, udp_sock;
	sockaddr_in server_addr;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port   = htons(PORT);
	if (argc < 2) {
		inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr.s_addr);
	} else {
		inet_pton(AF_INET, argv[1], &server_addr.sin_addr.s_addr);
	}

	udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_sock < 0) {
		perror("socket");
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

	close(tcp_sock);
	close(udp_sock);

	exit(EXIT_SUCCESS);
}
