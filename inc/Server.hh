#include <map>
#include <list>

#include <poll.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "Client.hh"
#include "Player.hh"


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

		std::map<int, int> _handshakes;  // cookie to client socket map

		/* NOTE: in _clients, _playerds and _cfds, the index in the array
		 * equals the client's id. */
		Player **_players;          // players array
		Client **_clients;          // clients array
		struct pollfd *_cfds;       // clients poll structures

		/* list of free slots in _clients or _cfds */
		std::list<int> _free_slots; // free slots indices

		/* TCP socket in _sfds[0], UDP socket in _sfds[1] */
		struct pollfd _sfds[2];     // server poll structures


		private:
		// do not allow copy
		Server(Server &);
		Server(const Server &);

		void handle_udp_msg();
		void handle_new_client();
		int handle_client_msg(int cidx);
		int process_client_msg(int cidx, const char *msg);
		int process_client_dgram(const struct sockaddr_in &, const char *);


		void send_cookie(int sock);

		void add_client(int csock, const struct sockaddr_in &caddr);
		void remove_client(int cidx);
		void add_player(int cidx);
		void remove_player(int cidx);


		public:
		~Server();
		Server(int port, unsigned int maxnclients = MAXNCLIENTS);

		bool full();

		void process_events(unsigned int timeout_ms = 0);

		// accessors
		int tcp_port(void) const;
		int udp_port(void) const;
	};
}

