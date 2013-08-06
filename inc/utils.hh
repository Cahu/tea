#include <sys/socket.h>

ssize_t tcp_send(int sock, const char *buff, unsigned short len, int flags);
ssize_t tcp_recv(int sock, char *buff, unsigned short len, int flags);
