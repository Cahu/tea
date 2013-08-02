#include "Server.hh"

#include <cstdio>
#include <unistd.h>


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
				int csock;

				csock = accept(_tcp_socket, NULL, 0);

				if (csock < 0) {
					perror("accept");
				}
				
				else {
					fprintf(stderr, "New client connected\n");
				
					if (!_free_slots.empty()) {
						int slot = _free_slots.front();
						_free_slots.pop_front();

						_cfds[slot].fd     = csock;
						_cfds[slot].events = POLLIN;
					}

					else {
						char BYE[] = "No free slots, bye!\n";
						send(csock, BYE, sizeof BYE, 0);
					}
				}
			}

			else if (_sfds[0].revents & POLLHUP) {
				throw "'Impossible' error";
			}

			// Event on udp socket?
		}

#define RECYCLE_SOCK(sock, pfd) \
		close(csock);           \
		pfd.fd = -1;            \

		// poll client fds
		rv = poll(_cfds, _maxnclients, timeout_ms);

		if (rv > 0) {
			for (unsigned int i = 0; i < _maxnclients; i++) {
				int csock = _cfds[i].fd;

				// NOTE: a closed connection may be signaled with POLLIN or
				// POLLHUP depending on the OS. If it's signaled with POLLIN,
				// recv() will read 0 bytes.

				if (_cfds[i].revents & POLLIN) {
					size_t size;
					char *msg = new char[64];
					fprintf(stderr, "Got msg from client\n");

					size = recv(csock, msg, 64, 0);

					if (size <= 0) {
						fprintf(stderr, "Client disconnected\n");
						RECYCLE_SOCK(sock, _cfds[i]);
						_free_slots.push_front(i);
					} else {
						printf("MSG: %s", msg);
					}
				}

				if (_cfds[i].revents & POLLHUP) {
					fprintf(stderr, "Client disconnected\n");
					RECYCLE_SOCK(sock, _cfds[i]);
					_free_slots.push_front(i);
				}
			}
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
}
