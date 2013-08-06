#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <poll.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#ifndef NDEBUG
	#include <assert.h>
#endif

#include "Player.hh"
#include "utils.hh"

#define PORT 9999
#define MAX_MSG_LEN 64

#define CHK(msg, cmd)       \
	if (0 > (cmd)) {        \
		perror(msg);        \
		exit(EXIT_FAILURE); \
	}

using TEA::Player;

static int tcp_sock;
static int udp_sock;
static Player *players;

void init_connect(const char *, unsigned short);
int  handle_handshake(void);
int  handle_tcp_msg();
int  handle_udp_msg();

void add_player(unsigned int);
void remove_player(unsigned int);


int main(int argc, char *argv[])
{
	if (argc < 2) {
		init_connect("127.0.0.1", PORT);
	} else {
		init_connect(argv[1], PORT);
	}

	int id = handle_handshake();
	if (id < 0) {
		fprintf(stderr, "Error during handshake\n");
		close(tcp_sock);
		close(udp_sock);
		exit(EXIT_FAILURE);
	}

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
				if (-1 == handle_tcp_msg()) {
					fprintf(stderr, "Connexion with server lost\n");
					break;
				}
			}

			if (fds[1].revents & POLLIN) {
				if (-1 == handle_udp_msg()) {
					fprintf(stderr, "Connexion with server lost\n");
					break;
				}
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


void add_player(unsigned int pidx)
{
	;
}


void remove_player(unsigned int pidx)
{
	;
}


int handle_handshake(void)
{
	int id = -1;

	ssize_t size;
	char msg[MAX_MSG_LEN];

	// get a cookie
	size = tcp_recv(tcp_sock, msg, MAX_MSG_LEN-1, 0);
	if (size < 0) {
		fprintf(stderr, "Can't get cookie\n");
		return -1;
	}
	msg[size] = '\0';

	// TODO: resend if packet lost
	// send the cookie back (on udp socket!)
	unsigned int cookienum;
	if (1 == sscanf(msg, "COOKIE %u", &cookienum)) {
		size = sprintf(msg, "COOKIE %u", cookienum);
		fprintf(stderr, "got cookie %u\n", cookienum);
		send(udp_sock, msg, size, 0);
	}

	// get an ID
	size = tcp_recv(tcp_sock, msg, MAX_MSG_LEN-1, 0);
	if (size < 0) {
		fprintf(stderr, "Can't get an ID\n");
		return -1;
	}
	msg[size] = '\0';

	if (1 != sscanf(msg, "ID %d", &id)) {
		return -1;
	}

	return id;
}


int handle_tcp_msg(void)
{
	ssize_t size;
	char msg[MAX_MSG_LEN];

	unsigned int id;

	size = tcp_recv(tcp_sock, msg, MAX_MSG_LEN-1, 0);
	if (size <= 0) {
		return -1;
	}
	msg[size] = '\0';

	if (1 == sscanf(msg, "JOIN %u", &id)) {
		add_player(id);
	}

	if (1 == sscanf(msg, "LEAVE %u", &id)) {
		remove_player(id);
	}

	else {
		fprintf(stderr, "Can't make sence of msg from server: %s\n", msg);
	}

	return 0;
}


int handle_udp_msg(void)
{
	ssize_t size;
	char msg[MAX_MSG_LEN];

	unsigned int id;
	unsigned int flags;

	size = tcp_recv(udp_sock, msg, MAX_MSG_LEN-1, 0);
	if (size <= 0) {
		return -1;
	}
	msg[size] = '\0';

	if (2 == sscanf(msg, "%u:%u", &id, &flags)) {
		;
	}

	else {
		fprintf(stderr, "Can't make sence of msg from server: %s\n", msg);
	}

	return 0;
}
