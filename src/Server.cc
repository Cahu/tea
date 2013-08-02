#include "Server.hh"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

#include <assert.h>

#define MAX_MSG_LEN 64

namespace TEA {


namespace TEA {

	Server::Server(int port, unsigned int maxnclients) {
		int rv;

		_tcp_port = port;
		_udp_port = port;

		// TCP socket
		if (-1 == (_tcp_socket = socket(AF_INET, SOCK_STREAM, 0))) {
			perror("socket");
			throw "Can't create tcp socket";
		}

		int optval = 1;
		setsockopt(_tcp_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

		_tcp_addr.sin_family      = AF_INET;
		_tcp_addr.sin_port        = htons(port);
		_tcp_addr.sin_addr.s_addr = INADDR_ANY;

		rv = bind(
			_tcp_socket,
			(struct sockaddr *) &_tcp_addr,
			sizeof (struct sockaddr_in)
		);

		if (-1 == rv) {
			perror("bind");
			throw "Can't create tcp socket";
		}
		
		rv = listen(_tcp_socket, 10);

		if (-1 == rv) {
			perror("listen");
			throw "Can't create tcp socket";
		}

		// UDP socket
		// TODO
		_udp_socket = -1;

		// server poll stuff
		_sfds[0].fd      = _tcp_socket;
		_sfds[0].events  = POLLIN | POLLPRI;
		_sfds[1].fd      = _udp_socket;
		_sfds[1].events  = POLLIN | POLLPRI;

		// clients poll stuff
		_nclients = 0;
		_maxnclients = maxnclients;
		_cfds = new struct pollfd[_maxnclients];
		for (unsigned int i = 0; i < _maxnclients; i++) {
			_cfds[i].fd      = -1; // unset: poll will ignore fds < 0
			_cfds[i].events  = POLLIN | POLLPRI;
			_free_slots.push_back(i);
		}
	}


	Server::~Server() {
		close(_tcp_socket);
		//close(_udp_socket);

		delete[] _cfds;
	}


	void Server::process_events(unsigned int timeout_ms)
	{
		int rv;

		// poll server fds
		rv = poll(_sfds, 2, timeout_ms);

		if (rv > 0) {
			// Event on tcp socket?
			if (_sfds[0].revents & POLLIN) {
				handle_tcp_msg();
			}

			// Event on udp socket?
			if (_sfds[1].revents & POLLIN) {
				handle_udp_msg();
			}
		}

		// poll client fds
		rv = poll(_cfds, _maxnclients, timeout_ms);

		if (rv > 0) {
			for (unsigned int i = 0; i < _maxnclients; i++) {
				int csock = _cfds[i].fd;

				if (_cfds[i].revents & POLLIN) {

					try {
						handle_client_msg(csock);
					}

					catch (const char *e) {
						fprintf(stderr, "%s", e);
						close(csock);
						_cfds[i].fd = -1;
						_free_slots.push_front(i);
					}
				}
			}
		}
	}


	void Server::handle_tcp_msg()
	{
		int csock;

		csock = accept(_tcp_socket, NULL, 0);

		if (csock < 0) {
			perror("accept");
			throw "Can't accept new client\n";
		}

		else {
			puts("New client connected");

			if (!_free_slots.empty()) {
				int slot = _free_slots.front();
				_free_slots.pop_front();

				assert(_cfds[slot].fd < 0);
				_cfds[slot].fd     = csock;
				_cfds[slot].events = POLLIN;

				send_cookie(csock);
			}

			else {
				char BYE[] = "No free slots, bye!\n";
				send(csock, BYE, sizeof BYE, 0);
			}
		}
	}


	void Server::handle_udp_msg()
	{
		char msg[MAX_MSG_LEN];
		fprintf(stderr, "Got msg from client\n");

		struct sockaddr_in from;
		socklen_t slen = sizeof (struct sockaddr_in);

		ssize_t size = recvfrom(
			_udp_socket, msg, MAX_MSG_LEN-1, 0,
			(struct sockaddr*) &from, &slen
		);


		if (size > 0) {
			msg[size] = '\0';

			try {
				process_client_dgram(msg);
			}

			catch (const char *e) {
				fprintf(stderr, "%s", e);
			}
		}
		
		else {
			throw "Client disconnected\n";
		}
	}


	void Server::handle_client_msg(int csock)
	{
		char msg[MAX_MSG_LEN];
		fprintf(stderr, "Got msg from client\n");

		ssize_t size = recv(csock, msg, 64-1, 0);

		if (size > 0) {
			msg[size] = '\0';
			process_client_msg(msg);
		}

		else {
			throw "Client disconnected\n";
		}
	}


	int Server::tcp_port(void) const
	{
		return _tcp_port;
	}


	int Server::udp_port(void) const
	{
		return _udp_port;
	}


	void Server::send_cookie(int sock)
	{
		int cookie;
		char msg[MAX_MSG_LEN];

		do {
			cookie = rand() % 424242;
		} while (_handshakes.find(cookie) != _handshakes.end());

		_handshakes[cookie] = sock;

		sprintf(msg, "COOKIE %d\n", cookie);
		send(sock, msg, strlen(msg), cookie);
	}


	void Server::process_client_msg(const char *msg)
	{
		if (NULL != strstr(msg, "JOIN")) {
			fprintf(stderr, "JOIN\n");
		}

		else if (NULL != strstr(msg, "LEAVE")) {
			fprintf(stderr, "LEAVE\n");
		}

		else if (NULL != strstr(msg, "QUIT")) {
			fprintf(stderr, "QUIT\n");
		}

		else {
			throw "Unrecognized message\n";
		}
	}


	void Server::process_client_dgram(const char *dgram)
	{
		int cookienum;

		if (sscanf(dgram, "COOKIE %d", &cookienum)) {

			std::map<int, int>::iterator it = _handshakes.find(cookienum);
			if (it != _handshakes.end()) {
				printf("Found matching handshake\n");
				_handshakes.erase(it);
			} else {
				throw "Cookie #%d unknown or too old\n";
			}
		}
	}
}
