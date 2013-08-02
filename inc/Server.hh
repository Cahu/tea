#include <list>

#include <poll.h>
#include <arpa/inet.h>
#include <sys/socket.h>

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

		private:
		Server(Server &);
		Server(const Server &);

		public:
		~Server();
		Server(int port, unsigned int maxnclients = MAXNCLIENTS);

		void process_events(unsigned int timeout_ms = 0);

		// accessors
		int tcp_port(void) const;
		int udp_port(void) const;
	};
}

