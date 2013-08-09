#include <sys/socket.h>
#include <arpa/inet.h>

ssize_t tcp_send(int sock, const char *buff, uint16_t len, int flags);
ssize_t tcp_recv(int sock, char *buff, uint16_t len, int flags);
