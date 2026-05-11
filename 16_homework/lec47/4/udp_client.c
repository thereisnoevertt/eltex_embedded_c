#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#define HOST "127.0.0.1"
#define UDP_PORT 8081
#define SUCCESS 0
#define FAILURE -1

int main(void) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("socket");
        return FAILURE;
    }

    struct timeval tv = { .tv_sec = 3, .tv_usec = 0 };
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        perror("setsockopt SO_RCVTIMEO");
        close(fd);
        return FAILURE;
    }

    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons(UDP_PORT);

    if (inet_pton(AF_INET, HOST, &srv.sin_addr) <= 0) {
        perror("inet_pton");
        close(fd);
        return FAILURE;
    }

    const char *req = "time";
    if (sendto(fd, req, strlen(req), 0,
               (struct sockaddr*)&srv, sizeof(srv)) == -1) {
        perror("sendto");
        close(fd);
        return FAILURE;
    }

    char buf[256];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    ssize_t n = recvfrom(fd, buf, sizeof(buf) - 1, 0,
                        (struct sockaddr*)&from, &fromlen);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            fprintf(stderr, "Таймаут: сервер не ответил за 3 секунды\n");
        } else {
            perror("recvfrom");
        }
        close(fd);
        return FAILURE;
    }
    buf[n] = '\0';
    fputs(buf, stdout);

    close(fd);
    return SUCCESS;
}