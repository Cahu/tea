#include "utils.hh"

#include <arpa/inet.h>


/* These functions are used to take care of annoying properties of TCP packets
 * which can be split or assembled together. In other words, one send() does
 * not imply one recv().
 *
 * + tcp_send will first prepend the packet with the size so the receiver
 *   knows the size passed to a single send().
 * + tcp_recv will first extract the length of the packet with a call to
 *   recv() and will then read the whole packet with a second call to recv
 *   using the size that was just read.
 * + sendall will loop until everything is sent
 * + recvall will loop until all is received
 *
 * This way, there is always one tcp_recv for one tcp_send.
 */

ssize_t sendall(int sock, const char *buff, size_t len, int flags)
{
	size_t size, sent = 0;

	do {
		size = send(sock, buff+sent, len-sent, flags);

		if (size < 1) {
			return sent;
		}

		sent += size;

	} while (sent < len);

	return sent;
}


size_t recvall(int sock, char *buff, size_t len, int flags)
{
	size_t size, got = 0;

	do {
		size = recv(sock, buff+got, len-got, flags);

		if (size < 1) {
			return got;
		}

		got += size;

	} while (got < len);

	return got;
}


ssize_t tcp_send(int sock, const char *buff, unsigned short len, int flags)
{
	size_t size;
	unsigned short sz = htons(len);

	if (1 > (size = sendall(sock, (char *) &sz, sizeof sz, flags))) {
		return size;
	}

	size = sendall(sock, buff, len, flags);

	return size;
}


ssize_t tcp_recv(int sock, char *buff, unsigned short len, int flags)
{
	ssize_t size;
	unsigned short pktsz;

	// first two bytes are the packet length
	if (1 > (size = recvall(sock, (char *) &pktsz, sizeof pktsz, flags))) {
		return size;
	}

	pktsz = ntohs(pktsz);

	if (pktsz > len) {
		return -1;
	}

	size = recv(sock, buff, pktsz, flags);

	return size;
}
