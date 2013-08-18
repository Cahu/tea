#ifndef _CONNECTION_HH_
#define _CONNECTION_HH_

#include <poll.h>

#include <list>
#include <string>



class NetGame {

	public:
	struct Event;
	enum Error {
		ERR_UNDEF         = 0,
		ERR_CONNECT_TCP   = 1,
		ERR_CONNECT_UDP   = 2,
		ERR_COOKIE        = 3,
		ERR_ID            = 4,
		ERR_RECV_TCP      = 5,
		ERR_RECV_UDP      = 6,
		ERR_SEND_TCP      = 7,
		ERR_SEND_UDP      = 8,
		ERR_DISCONNECTED  = 9
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
	void handshake();
	void handle_tcp_msg();
	void handle_udp_msg();

	void enqueue_event(NetGame::Event &e);


	public:
	~NetGame();
	NetGame(const char *, unsigned short);

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
