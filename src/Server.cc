#include "Server.hh"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

#include <assert.h>


namespace TEA {

	char cookie_regex[] = "^COOKIE[[:space:]]+([[:digit:]]+)";

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

		// compile regexes to match against clients' commands
		if (0 != regcomp(&_cookie_re, cookie_regex, REG_EXTENDED)) {
			fprintf(stderr, "Failed to compile regex\n");
		}
	}


	Server::~Server() {
		close(_tcp_socket);
		//close(_udp_socket);

		delete[] _cfds;

		regfree(&_cookie_re);
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
					handle_client_msg(csock);
					close(csock);
					_cfds[i].fd = -1;
					_free_slots.push_front(i);
				}
			}
		}
	}


	int Server::handle_tcp_msg()
	{
		int csock;

		csock = accept(_tcp_socket, NULL, 0);

		if (csock < 0) {
			perror("accept");
			return -1;
		}

		else {
			fprintf(stderr, "New client connected\n");

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

		return 0;
	}


	int Server::handle_udp_msg()
	{
		int rv = 0;
		return rv;
	}


	int Server::handle_client_msg(int csock)
	{
		char *msg;
		int rv = 0;
		ssize_t size;

		msg = new char[64];
		fprintf(stderr, "Got msg from client\n");

		size = recv(csock, msg, 64-1, 0);

		if (size > 0) {
			msg[size] = '\0';
			rv = parse_client_msg(msg);
		} else {
			fprintf(stderr, "Client disconnected\n");
			rv = -1;
		}

		delete[] msg;

		return rv;
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
		char *msg = new char[32];

		do {
			cookie = rand() % 424242;
		} while (_handshakes.find(cookie) != _handshakes.end());

		_handshakes[cookie] = sock;

		sprintf(msg, "COOKIE %d\n", cookie);
		send(sock, msg, strlen(msg), cookie);

		delete[] msg;
	}


	#define MAX_COOKIE_LEN 32

	int Server::parse_client_msg(const char *msg)
	{
		regmatch_t m[2];

		if (REG_NOMATCH != regexec(&_cookie_re, msg, 2, m, 0)) {
			size_t cookielen = m[1].rm_eo - m[1].rm_so;

			if (cookielen >= MAX_COOKIE_LEN) {
				fprintf(stderr, "Cookie too long!\n");
				return -1;
			}

			// extract cookie
			char *cookiestr = new char[MAX_COOKIE_LEN];
			snprintf(cookiestr, cookielen+1, "%s", msg+m[1].rm_so);
			int cookienum = atoi(cookiestr);
			delete[] cookiestr;

			std::map<int, int>::iterator it = _handshakes.find(cookienum);
			if (it != _handshakes.end()) {
				printf("Found matching handshake\n");
				_handshakes.erase(it);
			} else {
				printf("Cookie #%d unknown or too old\n", cookienum);
			}
		}

		return 0;
	}
}
