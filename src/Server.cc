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
			for (unsigned int i = 0; i < 2; i++) {

				if (_sfds[i].revents & POLLIN) {
					fprintf(stderr, "New client connected\n");
					
					int csock;
					struct sockaddr_in caddr;
					socklen_t slen = sizeof (struct sockaddr_in);

					csock = accept(
						_tcp_socket, (struct sockaddr *) &caddr, &slen
					);

					if (csock < 0) {
						perror("accept");
						continue;
					}

					if (!_free_slots.empty()) {
						char HELLO[] = "Hi there\n";
						send(csock, HELLO, sizeof HELLO, 0);
					} else {
						char BYE[] = "No free slots, bye!\n";
						send(csock, BYE, sizeof BYE, 0);
					}

					close(csock);
				}

				if (_sfds[i].revents & POLLHUP) {
					throw "'Impossible' error";
				}
			}
		}


		// poll client fds
		rv = poll(_cfds, _maxnclients, timeout_ms);

		if (rv > 0) {
			for (unsigned int i = 0; i < _maxnclients; i++) {

				if (_cfds[i].revents & POLLIN) {
					fprintf(stderr, "Got msg from client\n");
					close(_cfds[i].fd);
					_cfds[i].fd = -1;
				}

				if (_cfds[i].revents & POLLHUP) {
					fprintf(stderr, "Client disconnected\n");
					close(_cfds[i].fd);
					_cfds[i].fd = -1;
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
