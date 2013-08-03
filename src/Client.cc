#include "Client.hh"

#include <cstdio>
#include <cstring>


namespace TEA {

	Client::Client(int csock, const struct sockaddr_in &addr)
	{
		_udp_addr = addr;
		_tcp_sock = csock;
#ifndef NDEBUG
		fprintf(stderr, "+ Client created\n");
#endif
	}

	Client::~Client() {
#ifndef NDEBUG
		fprintf(stderr, "- Client removed\n");
#endif
	}


	int Client::send_msg(const char *msg) const
	{
		ssize_t tmp;
		size_t sent = 0;
		size_t len = strlen(msg);

		do {
			tmp = send(_tcp_sock, msg, len-sent, 0);

			if (tmp < 0) {
				perror("send");
				return -1;
			}

			sent += tmp;

		} while (sent < len);

		return 0;
	}


	const struct sockaddr_in &Client::addr(void) const
	{
		return _udp_addr;
	}

	int Client::sock()
	{
		return _tcp_sock;
	}
}
