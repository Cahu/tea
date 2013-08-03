#include "Client.hh"

#include <cstdio>
#include <cstring>


int cmp_addr(const sockaddr_in &a, const sockaddr_in &b)
{
	if (a.sin_addr.s_addr < b.sin_addr.s_addr) {
		return -1;
	} else if (a.sin_addr.s_addr > b.sin_addr.s_addr) {
		return +1;
	}

	if (a.sin_port < b.sin_port) {
		return -1;
	} else if (a.sin_port > b.sin_port) {
		return +1;
	}

	return 0;
}


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


	bool Client::is(const sockaddr_in &addr) const
	{
		return 0 == cmp_addr(addr, _udp_addr);
	}


	bool Client::is(int sock) const
	{
		return sock == _tcp_sock;
	}


	const struct sockaddr_in &Client::get_addr(void) const
	{
		return _udp_addr;
	}


	int Client::get_sock()
	{
		return _tcp_sock;
	}
}
