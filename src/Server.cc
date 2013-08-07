#include "Server.hh"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sstream>

#include "utils.hh"
#include "cmds.hh"

#ifndef NDEBUG
	#include <assert.h>
#endif


#define MAX_MSG_LEN 64


namespace TEA {
	char SERVER_FULL[]  = "FULL";
	char HANDSHAKE_OK[] = "HANDSHAKE OK";

	Server::Server(int port, unsigned int maxnclients)
	{
		int rv;

		_tcp_port = port;
		_udp_port = port;

		// TCP socket
		if (-1 == (_tcp_socket = socket(AF_INET, SOCK_STREAM, 0))) {
			perror("socket");
			throw "Can't create tcp socket";
		}

		int optval = 1;
		setsockopt(
			_tcp_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval
		);

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
		if (-1 == (_udp_socket = socket(AF_INET, SOCK_DGRAM, 0))) {
			perror("socket");
			throw "Can't create udp socket";
		}

		setsockopt(
			_udp_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval
		);

		_udp_addr.sin_family      = AF_INET;
		_udp_addr.sin_port        = htons(port);
		_udp_addr.sin_addr.s_addr = INADDR_ANY;

		rv = bind(
			_udp_socket,
			(struct sockaddr *) &_udp_addr,
			sizeof (struct sockaddr_in)
		);

		if (-1 == rv) {
			perror("bind");
			throw "Can't create tcp socket";
		}

		// server poll stuff
		_sfds[0].fd      = _tcp_socket;
		_sfds[0].events  = POLLIN | POLLPRI;
		_sfds[1].fd      = _udp_socket;
		_sfds[1].events  = POLLIN | POLLPRI;

		// clients poll stuff
		_nclients = 0;
		_maxnclients = maxnclients;
		_players = new Player*[_maxnclients];
		_clients = new Client*[_maxnclients];
		_cfds    = new struct pollfd[_maxnclients];
		for (unsigned int i = 0; i < _maxnclients; i++) {
			_free_slots.push_back(i);
			_players[i]     = NULL;
			_clients[i]     = NULL;
			_cfds[i].fd     = -1; // unset: poll will ignore fds < 0
			_cfds[i].events = POLLIN;
		}
	}


	Server::~Server() {
		close(_tcp_socket);
		close(_udp_socket);

		for (unsigned int i = 0; i < _maxnclients; i++) {
			if (_clients[i] != NULL) { delete _clients[i]; }
			if (_players[i] != NULL) { delete _players[i]; }
		}

		delete[] _cfds;
		delete[] _clients;
		delete[] _players;
	}


	bool Server::full()
	{
		return _free_slots.empty();
	}


	void Server::add_client(int csock, const struct sockaddr_in &caddr)
	{
		//TODO: make sure the ip address in csock is the same as the one in
		//caddr.

		if (full()) {
			throw "Server full";
		}

		int id = _free_slots.front();
		_free_slots.pop_front();

#ifndef NDEBUG
		assert(_cfds[id].fd < 0);
		assert(_clients[id] == NULL);
#endif

		_cfds[id].fd     = csock;
		_cfds[id].events = POLLIN;
		_clients[id] = new Client(csock, caddr);

		// send the client its ID
		char msg[MAX_MSG_LEN];
		sprintf(msg, "ID %d\n", id);
		_clients[id]->send_msg(msg);

		// send player list to new client
		Server::send_player_list(id);

#ifndef NDEBUG
		fprintf(stderr, "New client id: %d\n", id);
#endif
	}


	void Server::remove_client(int cidx)
	{
		int csock = _clients[cidx]->get_sock();

#ifndef NDEBUG
		assert(csock >= 0);
		assert(_clients[cidx] != NULL);
#endif

		close(csock);
		_cfds[cidx].fd = -1;

		delete _clients[cidx];
		_clients[cidx] = NULL;

		// careful: remove player after the client as been removed. This
		// prevents write error (remove_player advertises the quit event)
		// in case the socket as already been closed client-side.
		if (_players[cidx] != NULL) {
			remove_player(cidx);
		}

		_free_slots.push_front(cidx);
	}


	void Server::add_player(int cidx)
	{
		char msg[MAX_MSG_LEN];

		_players[cidx] = new Player();

		// tell everybody about the new player
		sprintf(msg, CMD_JOIN " %u", cidx);
		for (unsigned int i = 0; i < _maxnclients; i++) {
			if (_clients[i] != NULL) {
				_clients[i]->send_msg(msg);
			}
		}
	}


	void Server::remove_player(int cidx)
	{
		delete _players[cidx];
		_players[cidx] = NULL;

		// tell everybody about the player leaving
		char msg[MAX_MSG_LEN];
		sprintf(msg, CMD_LEAVE " %u", cidx);
		for (unsigned int i = 0; i < _maxnclients; i++) {
			if (_clients[i] != NULL) {
				_clients[i]->send_msg(msg);
			}
		}
	}


	void Server::process_events(unsigned int timeout_ms)
	{
		//TODO: use the timeout

		// poll server fds
		while (poll(_sfds, 2, 0)) {
			// Event on tcp socket?
			if (_sfds[0].revents & POLLIN) {
				handle_new_client();
			}

			// Event on udp socket?
			if (_sfds[1].revents & POLLIN) {
				handle_udp_msg();
			}
		}

		// poll client fds
		while (poll(_cfds, _maxnclients, timeout_ms)) {

			for (unsigned int i = 0; i < _maxnclients; i++) {
				if (_cfds[i].revents & POLLIN) {
					if (-1 == handle_client_msg(i)) {
						remove_client(i);
					}
				}

				else if (_cfds[i].revents & POLLHUP) {
					remove_client(i);
				}
			}
		}
	}


	void Server::handle_new_client()
	{
		int csock;

		csock = accept(_tcp_socket, NULL, 0);

		if (csock < 0) {
			perror("accept");
			throw "Can't accept new client\n";
		}

		else {
			puts("New client connected");

			if (!full()) {
				send_cookie(csock);
			} else {
				tcp_send(csock, SERVER_FULL, sizeof SERVER_FULL, 0);
			}
		}
	}


	void Server::handle_udp_msg()
	{
		char msg[MAX_MSG_LEN];

		struct sockaddr_in from;
		socklen_t slen = sizeof (struct sockaddr_in);

		ssize_t size = recvfrom(
			_udp_socket, msg, MAX_MSG_LEN-1, 0,
			(struct sockaddr*) &from, &slen
		);

		if (size > 0) {
			msg[size] = '\0';
			process_client_dgram(from, msg);
		}
	}


	int Server::handle_client_msg(int cidx)
	{
		char msg[MAX_MSG_LEN];
		int csock = _clients[cidx]->get_sock();
		ssize_t size = tcp_recv(csock, msg, 64-1, 0);

		if (size > 0) {
			msg[size] = '\0';
			process_client_msg(cidx, msg);
		} else {
			return -1;
		}

		return 0;
	}


	int Server::tcp_port(void) const
	{
		return _tcp_port;
	}


	int Server::udp_port(void) const
	{
		return _udp_port;
	}


	int Server::send_cookie(int sock)
	{
		int cookie;
		char msg[MAX_MSG_LEN];

		do {
			cookie = rand() % 424242;
		} while (_handshakes.find(cookie) != _handshakes.end());

		_handshakes[cookie] = sock;

		sprintf(msg, "COOKIE %d", cookie);
		printf("Sending cookie %d\n", cookie);
		return tcp_send(sock, msg, strlen(msg), 0);
	}


	int Server::send_player_list(int cidx)
	{
		int nplayers = 0;

		std::stringstream ss;
		ss << CMD_PLIST " ";

		for (unsigned int i = 0; i < _maxnclients; i++) {
			if (_players[i] != NULL) {
				const Player *p = _players[i];
				//format: id:x:y;
				ss << i << ":"
				   << p->get_xpos() << ":"
				   << p->get_ypos() << ";";
				nplayers++;
			}
		}

		if (nplayers) {
			return _clients[cidx]->send_msg(ss.str().c_str());
		}

		return 0;
	}


	int Server::process_client_msg(int cidx, const char *msg)
	{
		if (NULL != strstr(msg, CMD_JOIN)) {
			fprintf(stderr, CMD_JOIN "\n");

			if (_players[cidx] != NULL) {
				fprintf(stderr, "Client already in game\n");
			} else {
				add_player(cidx);
			}
		}

		else if (NULL != strstr(msg, CMD_LEAVE)) {
			fprintf(stderr, CMD_LEAVE "\n");

			if (_players[cidx] == NULL) {
				fprintf(stderr, "Client not in game\n");
			} else {
				remove_player(cidx);
			}
		}

		else if (NULL != strstr(msg, CMD_QUIT)) {
			fprintf(stderr, CMD_QUIT "\n");
			remove_client(cidx);
		}

		else {
			return -1;
		}

		return 0;
	}


	int Server::process_client_dgram(
		const sockaddr_in &from, const char *dgram
	) {
		unsigned int id;
		unsigned int flag;
		unsigned int cookienum;

		ssize_t size;
		char msg[MAX_MSG_LEN];

		if (sscanf(dgram, "COOKIE %u", &cookienum)) {

			std::map<int, int>::iterator it = _handshakes.find(cookienum);

			if (it == _handshakes.end()) {
				fprintf(stderr, "Cookie #%u unknown or too old\n", cookienum);
			}

			else {
				puts("Found matching handshake");

				int csock = _handshakes[cookienum];
				_handshakes.erase(it);

				// promote to client state
				if (!full()) {
					add_client(csock, from);
				} else {
					// unfortunately, someone else was faster with the
					// handshake...
					tcp_send(csock, SERVER_FULL, sizeof SERVER_FULL, 0);
				}
			}
		}

		else if (2 == sscanf(dgram, "%u:%u", &id, &flag)) {

			if (id > _maxnclients || _clients[id] == NULL) {
				fprintf(stderr, "Got message with invalid id #%u\n", id);
				return -1;
			}

			// make sure the id is owned by the right guy
			if (!_clients[id]->is(from)) {
				fprintf(stderr, "Someone trying to impersonate #%u!\n", id);
				return -1;
			}

			// discard flags from clients that are not playing
			if (_players[id] == NULL) {
				fprintf(stderr, "Client #%u not playing sent flags.\n",id);
				return -1;
			}

			size = sprintf(msg, "%u:%u", id, flag);

			// broadcast flag
			for (unsigned int i = 0; i < _maxnclients; i++) {
				if (_clients[i] != NULL) {
					const sockaddr_in &addr = _clients[i]->get_addr();
					sendto(
						_udp_socket, msg, size, 0,
						(sockaddr *) &addr, sizeof addr
					);
				}
			}
		}

		else {
			return -1;
		}

		return 0;
	}
}
