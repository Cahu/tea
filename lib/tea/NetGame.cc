#include "NetGame.hh"
#include "utils/netutils.hh"
#include "utils/splitstr.hh"
#include "cmds.hh"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

#include <vector>



NetGame::NetGame(const char *ip, unsigned short port)
{
	_client_id = -1;
	_playing = false;
	_error = NetGame::ERR_UNDEF;

	sockaddr_in saddr;

	saddr.sin_family = AF_INET;
	saddr.sin_port   = htons(port);
	inet_pton(AF_INET, ip, &saddr.sin_addr.s_addr);


	_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (_udp_sock < 0) {
		_error = NetGame::ERR_CONNECT_UDP;
		throw _error;
	}

	if (connect(_udp_sock, (sockaddr *) &saddr, sizeof saddr) < 0) {
		_error = NetGame::ERR_CONNECT_UDP;
		throw _error;
	}


	_tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (_tcp_sock < 0) {
		_error = NetGame::ERR_CONNECT_TCP;
		throw _error;
	}

	if (connect(_tcp_sock, (sockaddr *) &saddr, sizeof saddr) < 0) {
		_error = NetGame::ERR_CONNECT_TCP;
		throw _error;
	}

	_fds[0].fd     = _tcp_sock;
	_fds[0].events = POLLIN;
	_fds[1].fd     = _udp_sock;
	_fds[1].events = POLLIN;
}


NetGame::~NetGame()
{
	close(_tcp_sock);
	close(_udp_sock);
}


void NetGame::poll_events()
{
	while (poll(_fds, 2, 0) > 0) {

		if (_fds[0].revents & POLLIN) {
			handle_tcp_msg();
		}

		if (_fds[1].revents & POLLIN) {
			handle_udp_msg();
		}
	}
}


void NetGame::enqueue_event(NetGame::Event &e)
{
	if (e.type == Event::PLAYER_ADD && e.pid == _client_id) {
		// server just added us to the game
		_playing = true;
	}

	else if (e.type == Event::PLAYER_LEAVE && e.pid == _client_id) {
		// server just removed us from the game
		_playing = false;
	}

	_events.push_back(e);
}


bool NetGame::pop_event(NetGame::Event &e)
{
	if (_events.empty()) {
		return false;
	}

	e = _events.front();
	_events.pop_front();

	return true;
}


void NetGame::handshake()
{
	ssize_t size;
	char msg[MAX_MSG_LEN];

	// get a cookie
	size = tcp_recv(_tcp_sock, msg, MAX_MSG_LEN-1, 0);
	if (size < 0) {
		_error = NetGame::ERR_COOKIE;
		throw _error;
	}
	msg[size] = '\0';

	// TODO: resend if packet lost
	// send the cookie back (on udp socket!)
	unsigned int cookienum;
	if (1 == sscanf(msg, "COOKIE %u", &cookienum)) {
		size = sprintf(msg, "COOKIE %u", cookienum);
		fprintf(stderr, "got cookie %u\n", cookienum);
		send(_udp_sock, msg, size, 0);
	} else {
		_error = NetGame::ERR_COOKIE;
		throw _error;
	}

	// get an ID
	size = tcp_recv(_tcp_sock, msg, MAX_MSG_LEN-1, 0);
	if (size < 0) {
		_error = NetGame::ERR_ID;
		throw _error;
	}
	msg[size] = '\0';

	unsigned int id;
	if (1 == sscanf(msg, "ID %u", &id)) {
		_client_id = id;
	} else {
		_error = NetGame::ERR_ID;
		throw _error;
	}
}


void NetGame::handle_tcp_msg()
{
	ssize_t size;
	char msg[MAX_MSG_LEN];

	size = tcp_recv(_tcp_sock, msg, MAX_MSG_LEN-1, 0);
	if (size < 0) {
		_error = NetGame::ERR_RECV_TCP;
		throw _error;
	} else if (size == 0) {
		_error = NetGame::ERR_DISCONNECTED;
		throw _error;
	}
	msg[size] = '\0';


	unsigned int id;
	NetGame::Event e;

	if (1 == sscanf(msg, CMD_JOIN " %u", &id)) {
		e.pid = id;
		e.pxpos = 0;
		e.pypos = 0;
		e.pflags = 0;
		e.type = NetGame::Event::PLAYER_ADD;
		enqueue_event(e);
	}

	else if (1 == sscanf(msg, CMD_LEAVE " %u", &id)) {
		e.pid = id;
		e.type = NetGame::Event::PLAYER_LEAVE;
		enqueue_event(e);
	}

	else if (strstr(msg, CMD_PLIST) == msg) {
		puts(msg);
		std::vector<std::string> items;
		splitstr(msg+sizeof(CMD_PLIST), items, ';');

		std::vector<std::string>::iterator it;
		for (it = items.begin(); it != items.end(); it++) {
			double x, y;
			unsigned int pid;

			if (3 == sscanf(it->c_str(), "%u:%lf:%lf", &pid, &x, &y)) {
				e.pid = pid;
				e.pxpos = x;
				e.pypos = y;
				e.pflags = 0;
				e.type = NetGame::Event::PLAYER_ADD;
				enqueue_event(e);
			}
		}
	}

	else {
		e.type = NetGame::Event::UNKNOWN;
		enqueue_event(e);
	}
}


void NetGame::handle_udp_msg()
{
	ssize_t size;
	char msg[MAX_MSG_LEN];

	size = recv(_udp_sock, msg, MAX_MSG_LEN-1, 0);
	if (size < 0) {
		_error = NetGame::ERR_RECV_UDP;
		throw _error;
	} else if (size == 0) {
		_error = NetGame::ERR_DISCONNECTED;
		throw _error;
	}
	msg[size] = '\0';


	unsigned int pid;
	unsigned int pflags;
	NetGame::Event e;

	if (2 == sscanf(msg, CMD_FLAGS " %u:%u", &pid, &pflags)) {
		if (pid != _client_id) {
			e.pid = pid;
			e.type = NetGame::Event::PLAYER_FLAGS;
			enqueue_event(e);
		} else {
			// TODO: use as ACK
		}
	}

	else if (strstr(msg, CMD_SYNC) == msg) {
		//puts(msg);
		std::vector<std::string> items;
		splitstr(msg+sizeof(CMD_SYNC), items, ';');

		std::vector<std::string>::iterator it;
		for (it = items.begin(); it != items.end(); it++) {
			double x, y;
			unsigned int pid;

			if (3 == sscanf(it->c_str(), "%u:%lf:%lf", &pid, &x, &y)) {
				e.pid = pid;
				e.pxpos = x;
				e.pypos = y;
				e.type = NetGame::Event::PLAYER_SYNC;
				enqueue_event(e);
			}
		}
	}

	else {
		e.type = NetGame::Event::UNKNOWN;
		enqueue_event(e);
	}
}


bool NetGame::playing()
{
	return _playing;
}


unsigned int NetGame::get_id()
{
	return _client_id;
}


void NetGame::send_flags(unsigned int flags)
{
	ssize_t size;
	char msg[MAX_MSG_LEN];

	size = sprintf(msg, CMD_FLAGS " %u:%u", _client_id, flags);
	size = send(_udp_sock, msg, size, 0);

	if (size < 0) {
		_error = ERR_SEND_UDP;
		throw _error;
	}
}


void NetGame::join()
{
	ssize_t size;
	size = tcp_send(_tcp_sock, CMD_JOIN, sizeof CMD_JOIN, 0);

	if (size < 0) {
		_error = ERR_SEND_TCP;
		throw _error;
	} else if (size == 0) {
		_error = ERR_DISCONNECTED;
		throw _error;
	}
}


void NetGame::leave()
{
	ssize_t size;
	size = tcp_send(_tcp_sock, CMD_LEAVE, sizeof CMD_LEAVE, 0);

	if (size < 0) {
		_error = ERR_SEND_TCP;
		throw _error;
	} else if (size == 0) {
		_error = ERR_DISCONNECTED;
		throw _error;
	}
}


void NetGame::quit()
{
	ssize_t size;
	size = tcp_send(_tcp_sock, CMD_QUIT, sizeof CMD_QUIT, 0);

	if (size < 0) {
		_error = ERR_SEND_TCP;
		throw _error;
	} else if (size == 0) {
		_error = ERR_DISCONNECTED;
		throw _error;
	}
}
