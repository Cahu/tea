#include <arpa/inet.h>
#include <sys/socket.h>


namespace TEA {

	class Client {
		int _tcp_sock;
		struct sockaddr_in _udp_addr;

		private:
		// do not allow copy
		Client(Client &);
		Client(const Client &);

		public:
		~Client();
		Client(int, const struct sockaddr_in &);

		int send_msg(const char *) const;

		// accessors
		int sock();
		const struct sockaddr_in &addr(void) const;
	};
}
