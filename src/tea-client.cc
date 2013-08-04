#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <poll.h>
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
int  handle_handshake(void);


int main(int argc, char *argv[])
{
	if (argc < 2) {
		init_connect("127.0.0.1", PORT);
	} else {
		init_connect(argv[1], PORT);
	}

	int id = handle_handshake();
	printf("got ID #%d\n", id);

	// init structures for poll()
	pollfd fds[2];
	fds[0].fd     = tcp_sock;
	fds[0].events = POLLIN;
	fds[1].fd     = udp_sock;
	fds[1].events = POLLIN;

	while (1) {

		// handle events on sockets
		if (poll(fds, 2, -1) > 0) {

			if (fds[0].revents & POLLIN) {
				// handle tcp messages here
				;
			}

			if (fds[1].revents & POLLIN) {
				// handle udp messages here
				;
			}
		}

		// update state
		;

		// draw
		;

		// sleep
		;
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


int handle_handshake(void)
{
	int id = -1;

	ssize_t size;
	char msg[MAX_MSG_LEN];

	// get a cookie
	size = recv(tcp_sock, msg, MAX_MSG_LEN-1, 0);
	if (size < 0) {
		perror("recv");
		return -1;
	}
	msg[size] = '\0';

	// send the cookie back (on udp socket!)
	unsigned int cookienum;
	if (1 == sscanf(msg, "COOKIE %u", &cookienum)) {
		size = sprintf(msg, "COOKIE %u", cookienum);
		send(udp_sock, msg, size, 0);
	}

	// get an ID
	size = recv(tcp_sock, msg, MAX_MSG_LEN-1, 0);
	if (size < 0) {
		perror("recv");
		return -1;
	}
	msg[size] = '\0';

	if (1 != sscanf(msg, "ID %d", &id)) {
		return -1;
	}

	return id;
}
