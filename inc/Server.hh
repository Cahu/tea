#include <map>
#include <list>

#include <poll.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <regex.h>
#include <sys/types.h>


#define MAXNCLIENTS 6  // default value


namespace TEA {

	class Server {
		int _tcp_port;
		int _udp_port;
		int _tcp_socket;
		int _udp_socket;
		struct sockaddr_in _tcp_addr;
		struct sockaddr_in _udp_addr;

		unsigned int _nclients;
		unsigned int _maxnclients;

		struct pollfd *_cfds;       // clients
		struct pollfd _sfds[2];     // server sockets
		std::list<int> _free_slots; // free slots indices

		std::map<int, int> _handshakes;

		// regexes
		regex_t _cookie_re;

		Server(Server &);
		Server(const Server &);

		void send_cookie(int sock);
		int  handle_client_msg(int sock, const char *msg);

		public:
		~Server();
		Server(int port, unsigned int maxnclients = MAXNCLIENTS);

		void process_events(unsigned int timeout_ms = 0);

		// accessors
		int tcp_port(void) const;
		int udp_port(void) const;
	};
}

