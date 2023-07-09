#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define SOCK_MyTCP -1
#define T 1

typedef struct table_{
    char entry[10][5001];
    int in,out,count;
    int size[10];
}table;

extern int my_socket(int domain, int type, int protocol);
extern int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern int my_listen(int sockfd, int backlog);
extern int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern int my_send(int sockfd, const void *buf, size_t len, int flags);
extern int my_recv(int sockfd, void *buf, size_t len, int flags);
extern int my_close(int fd);
