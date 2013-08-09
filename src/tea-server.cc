#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <poll.h>
#include <unistd.h>

#include <map>
#include <vector>
#include <sstream>

#include "Player.hh"
#include "Client.hh"
#include "utils.hh"
#include "cmds.hh"

#ifndef NDEBUG
	#include <assert.h>
#endif

#define PORT 9999
#define MAX_MSG_LEN 64

using TEA::Client;
using TEA::Player;

// state variables
static int tcp_socket;
static int udp_socket;

// object collections
static std::vector<Client *> clients;
static std::vector<Player *> players;
static std::map<int, int> handshakes;
static std::vector<unsigned int> free_slots;
// poll structures for clients
static std::vector<struct pollfd> cfds;



// network functions
static int init_network(unsigned short);
static void sync_everyone();
static void handle_new_client();
static void handle_udp_msg();
static int handle_client_msg(int);
static int send_cookie(int);
static int send_player_list(unsigned int);
static int process_client_msg(unsigned int, const char *);
static int process_client_dgram(const sockaddr_in &, const char *);

// state functions
static void add_client(int, const struct sockaddr_in &);
static void remove_client(int);
static void add_player(int);
static void remove_player(int);



int main(void)
{
	// feed everything that needs randomness
	srand(time(NULL));

	init_network(PORT);

	// server poll stuff
	pollfd pfd;
	std::vector<struct pollfd> sfds;
	pfd.fd     = tcp_socket;
	pfd.events = POLLIN;
	sfds.push_back(pfd);
	pfd.fd     = udp_socket;
	pfd.events = POLLIN;
	sfds.push_back(pfd);

	// main loop
	while (1) {
		// poll server fds
		while (poll((pollfd *) &sfds[0], sfds.size(), 0)) {
			// Event on tcp socket?
			if (sfds[0].revents & POLLIN) {
				handle_new_client();
			}

			// Event on udp socket?
			if (sfds[1].revents & POLLIN) {
				handle_udp_msg();
			}
		}

		// poll client fds
		while (poll((pollfd *) &cfds[0], cfds.size(), 0)) {

			for (unsigned int i = 0; i < cfds.size(); i++) {

				if (cfds[i].revents & POLLIN) {
					if (-1 == handle_client_msg(i)) {
						remove_client(i);
					}
				}

				else if (cfds[i].revents & POLLHUP) {
					remove_client(i);
				}
			}
		}

		// update state
		for (unsigned int i = 0; i < players.size(); i++) {
			Player *p = players[i];
			if (p != NULL) {
				p->tick(10);
			}
		}

		// 10ms sleep
		usleep(10000);

		// sync clients
		sync_everyone();
	}

	close(tcp_socket);
	close(udp_socket);

	for (unsigned int i = 0; i < clients.size(); i++)
		if (clients[i] != NULL) delete clients[i];
	for (unsigned int i = 0; i < players.size(); i++)
		if (players[i] != NULL) delete players[i];

	exit(EXIT_SUCCESS);
}



int init_network(unsigned short port)
{
	int rv;
	int optval = 1;
	struct sockaddr_in addr;

	addr.sin_family      = AF_INET;
	addr.sin_port        = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	// TCP socket
	if (-1 == (tcp_socket = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("socket");
		return -1;
	}

	setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

	rv = bind(tcp_socket, (sockaddr *) &addr, sizeof (sockaddr_in));
	if (-1 == rv) {
		perror("bind");
		return -1;
	}

	rv = listen(tcp_socket, 10);
	if (-1 == rv) {
		perror("listen");
		return -1;
	}

	// udp socket
	if (-1 == (udp_socket = socket(AF_INET, SOCK_DGRAM, 0))) {
		perror("socket");
		return -1;
	}

	setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

	rv = bind(udp_socket, (sockaddr *) &addr, sizeof (sockaddr_in));
	if (-1 == rv) {
		perror("bind");
		return -1;
	}

	return 0;
}


void add_client(int csock, const struct sockaddr_in &caddr)
{
	//TODO: make sure the ip address in csock is the same as the one in
	//caddr.

	int cid;
	
	if (!free_slots.empty()) {
		// recycle slots in our vectors
		cid = free_slots.back();
		free_slots.pop_back();

#ifndef NDEBUG
		assert(cfds[cid].fd < 0);
		assert(clients[cid] == NULL);
		assert(players[cid] == NULL);
#endif

		cfds[cid].fd     = csock;
		cfds[cid].events = POLLIN;
		clients[cid] = new Client(csock, caddr);
	}
	
	else {
		pollfd pfd = {
			.fd = csock,
			.events = POLLIN
		};
		cid = clients.size(); // new id is the same as the size of the vector
		cfds.push_back(pfd);
		clients.push_back(new Client(csock, caddr));
		players.push_back(NULL);
	}

	// send the client its ID
	char msg[MAX_MSG_LEN];
	sprintf(msg, "ID %d", cid);
	clients[cid]->send_msg(msg);

	// send player list to new client
	send_player_list(cid);

#ifndef NDEBUG
	fprintf(stderr, "New client id: %d\n", cid);
#endif
}


void remove_client(int cid)
{
	int csock = clients[cid]->get_sock();

#ifndef NDEBUG
	assert(csock >= 0);
	assert(clients[cid] != NULL);
#endif

	close(csock);
	cfds[cid].fd = -1;

	delete clients[cid];
	clients[cid] = NULL;

	// careful: remove player after the client as been removed. This
	// prevents write error (remove_player advertises the quit event)
	// in case the socket as already been closed client-side.
	if (players[cid] != NULL) {
		remove_player(cid);
	}

	free_slots.push_back(cid);
}


void add_player(int cid)
{
	char msg[MAX_MSG_LEN];

#ifndef NDEBUG
	assert(clients[cid] != NULL);
	assert(players[cid] == NULL);
#endif

	players[cid] = new Player();

	// tell everybody about the new player
	sprintf(msg, CMD_JOIN " %u", cid);
	for (unsigned int i = 0; i < clients.size(); i++) {
		if (clients[i] != NULL) {
			clients[i]->send_msg(msg);
		}
	}
}


void remove_player(int cid)
{
#ifndef NDEBUG
	assert(players[cid] != NULL);
#endif

	delete players[cid];
	players[cid] = NULL;

	// tell everybody about the player leaving
	char msg[MAX_MSG_LEN];
	sprintf(msg, CMD_LEAVE " %u", cid);
	for (unsigned int i = 0; i < clients.size(); i++) {
		if (clients[i] != NULL) {
			clients[i]->send_msg(msg);
		}
	}
}


void sync_everyone()
{
	int nplayers = 0;

#ifndef NDEBUG
	assert(clients.size() >= players.size());
#endif

	std::stringstream ss;
	ss << CMD_SYNC " ";

	for (unsigned int i = 0; i < players.size(); i++) {
		Player *p = players[i];

		if (p != NULL) {
			nplayers++;
			// format: id:x:y;id:x:y;...
			ss << i << ":" << p->get_xpos() << ":" << p->get_ypos() << ";";
		}
	}

	if (nplayers) {
		const char *str = ss.str().c_str();
		size_t len = strlen(str);

		for (unsigned int i = 0; i < clients.size(); i++) {
			Client *c = clients[i];

			if (c != NULL) {
				// send via UDP
				const sockaddr_in &addr = c->get_addr();
				sendto(
					udp_socket, str, len, 0, (sockaddr*) &addr, sizeof addr
				);
			}
		}
	}
}


void handle_new_client()
{
	int csock;

	csock = accept(tcp_socket, NULL, 0);

	if (csock < 0) {
		perror("accept");
		throw "Can't accept new client\n";
	}

	else {
		puts("New client connected");
		send_cookie(csock);
	}
}


void handle_udp_msg()
{
	char msg[MAX_MSG_LEN];

	struct sockaddr_in from;
	socklen_t slen = sizeof (sockaddr_in);

	ssize_t size = recvfrom(
		udp_socket, msg, MAX_MSG_LEN-1, 0,
		(struct sockaddr*) &from, &slen
	);

	if (size > 0) {
		msg[size] = '\0';
		process_client_dgram(from, msg);
	}
}


int handle_client_msg(int cid)
{
#ifndef NDEBUG
	assert(clients[cid] != NULL);
#endif

	char msg[MAX_MSG_LEN];
	int csock = clients[cid]->get_sock();
	ssize_t size = tcp_recv(csock, msg, 64-1, 0);

	if (size > 0) {
		msg[size] = '\0';
		process_client_msg(cid, msg);
	} else {
		return -1;
	}

	return 0;
}


int send_cookie(int sock)
{
	int cookie;
	char msg[MAX_MSG_LEN];

	do {
		cookie = rand() % 424242;
	} while (handshakes.find(cookie) != handshakes.end());

	handshakes[cookie] = sock;

	sprintf(msg, "COOKIE %d", cookie);
	return tcp_send(sock, msg, strlen(msg), 0);
}


int send_player_list(unsigned int cid)
{
#ifndef NDEBUG
	assert(clients.size() >= cid);
	assert(players.size() >= cid);
#endif 

	int nplayers = 0;

	std::stringstream ss;
	ss << CMD_PLIST " ";

	for (unsigned int i = 0; i < players.size(); i++) {
		Player *p = players[i];

		if (p != NULL) {
			nplayers++;
			//format: id:x:y;
			ss << i << ":" << p->get_xpos() << ":" << p->get_ypos() << ";";
		}
	}

	if (nplayers) {
		return clients[cid]->send_msg(ss.str().c_str());
	}

	return 0;
}


int process_client_msg(unsigned int cid, const char *msg)
{
#ifndef NDEBUG
	assert(clients.size() >= cid);
	assert(players.size() >= cid);
#endif
	puts(msg);

	if (NULL != strstr(msg, CMD_JOIN)) {
		fprintf(stderr, CMD_JOIN "\n");

		if (players[cid] != NULL) {
			fprintf(stderr, "Client already in game\n");
		} else {
			add_player(cid);
		}
	}

	else if (NULL != strstr(msg, CMD_LEAVE)) {
		fprintf(stderr, CMD_LEAVE "\n");

		if (players[cid] == NULL) {
			fprintf(stderr, "Client not in game\n");
		} else {
			remove_player(cid);
		}
	}

	else if (NULL != strstr(msg, CMD_QUIT)) {
		fprintf(stderr, CMD_QUIT "\n");
		remove_client(cid);
	}

	else {
		return -1;
	}

	return 0;
}


int process_client_dgram(const sockaddr_in &from, const char *dgram)
{
	unsigned int id;
	unsigned int flags;
	unsigned int cookienum;

	ssize_t size;
	char msg[MAX_MSG_LEN];

	if (sscanf(dgram, "COOKIE %u", &cookienum)) {

		std::map<int, int>::iterator it = handshakes.find(cookienum);

		if (it == handshakes.end()) {
			fprintf(stderr, "Cookie #%u unknown or too old\n", cookienum);
		}

		else {
			puts("Found matching handshake");

			int csock = handshakes[cookienum];
			handshakes.erase(it);

			// promote to client state
			add_client(csock, from);
		}
	}

	else if (2 == sscanf(dgram, CMD_FLAGS "%u:%u", &id, &flags)) {

		if (id > clients.size() || clients[id] == NULL) {
			fprintf(stderr, "Got message with invalid id #%u\n", id);
			return -1;
		}

		// make sure the id is owned by the right guy
		if (!clients[id]->is(from)) {
			fprintf(stderr, "Someone trying to impersonate #%u!\n", id);
			return -1;
		}

		// discard flags from clients that are not playing
		if (players[id] == NULL) {
			fprintf(stderr, "Client #%u not playing sent flags.\n",id);
			return -1;
		}

		players[id]->set_flags(flags);

		size = sprintf(msg, CMD_FLAGS "%u:%u", id, flags);

		// broadcast flag
		for (unsigned int i = 0; i < clients.size(); i++) {
			if (clients[i] != NULL) {
				const sockaddr_in &addr = clients[i]->get_addr();
				sendto(
					udp_socket, msg, size, 0,
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
