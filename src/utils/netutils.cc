#include "utils/netutils.hh"

#include <cstdio>
#include <cstring>


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
	ssize_t size;
	size_t sent = 0;


	do {
		size = send(sock, buff+sent, len-sent, flags);

		if (size < 0) {
			perror("send");
			return -1;
		} else if (size == 0) {
			break;
		}

		sent += size;

	} while (sent < len);

	return sent;
}


size_t recvall(int sock, char *buff, size_t len, int flags)
{
	ssize_t size;
	size_t got = 0;

	do {
		size = recv(sock, buff+got, len-got, flags);

		if (size < 0) {
			perror("recv");
			return -1;
		} else if (size == 0) {
			break;
		}

		got += size;

	} while (got < len);

	return got;
}


ssize_t tcp_send(int sock, const char *buff, uint16_t len, int flags)
{
	ssize_t size;
	uint16_t sz = htons(len);

	char *tmp = new char[len + sizeof sz];
	memcpy(tmp, (char *) &sz, sizeof sz);
	memcpy(tmp + sizeof sz, buff, len);

	size = sendall(sock, tmp, len + sizeof sz, flags);

	delete[] tmp;
	return size;
}


ssize_t tcp_recv(int sock, char *buff, uint16_t len, int flags)
{
	ssize_t size;
	uint16_t sz;

	// first two bytes are the packet length
	if (1 > (size = recvall(sock, (char *) &sz, sizeof sz, flags))) {
		fprintf(stderr, "Can't get size of message\n");
		return size;
	}

	sz = ntohs(sz);

	if (sz > len) {
		fprintf(
			stderr, "Message size (%hu) > buffer size (%hu)!\n", sz, len
		);
		return -1;
	}

	size = recvall(sock, buff, sz, flags);

	return size;
}
