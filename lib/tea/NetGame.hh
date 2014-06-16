#ifndef _CONNECTION_HH_
#define _CONNECTION_HH_

#include <poll.h>

#include <list>
#include <string>



class NetGame {

	public:
	struct Event;
	enum Error {
		ERR_UNDEF,
		ERR_CONNECT_TCP,
		ERR_CONNECT_UDP,
		ERR_COOKIE,
		ERR_ID,
		ERR_RECV_TCP,
		ERR_RECV_UDP,
		ERR_SEND_TCP,
		ERR_SEND_UDP,
		ERR_DISCONNECTED
	};


	protected:
	int _tcp_sock;
	int _udp_sock;
	pollfd _fds[2];

	bool _playing;
	unsigned int _client_id;

	Error _error;
	std::list<NetGame::Event> _events;


	protected:
	void handle_tcp_msg();
	void handle_udp_msg();

	void enqueue_event(NetGame::Event &e);


	public:
	~NetGame();
	NetGame(const char *, unsigned short);

	void handshake();
	void poll_events();
	bool pop_event(NetGame::Event &);

	bool playing();
	unsigned int get_id();

	void quit();
	void join();
	void leave();
	void send_flags(unsigned int);
};


struct NetGame::Event {
	enum {
		PLAYER_ADD,
		PLAYER_SYNC,
		PLAYER_LEAVE,
		PLAYER_FLAGS,

		SERVER_QUIT,
		SERVER_KICK,
		SERVER_NEWGAME,

		UNKNOWN
	} type;

	union {
		struct {
			unsigned int pid;
			unsigned int pflags;
			double pxpos;
			double pypos;
		};

		struct {
			const char *mapname;
		};
	};

	Event() : type(UNKNOWN) {};
};

#endif
