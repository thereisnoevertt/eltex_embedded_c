#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define HOST "127.0.0.1"
#define TCP_PORT 8080
#define SUCCESS 0
#define FAILURE -1

int main(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return FAILURE;
    }

    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons(TCP_PORT);

    if (inet_pton(AF_INET, HOST, &srv.sin_addr) <= 0) {
        perror("inet_pton");
        close(fd);
        return FAILURE;
    }

    if (connect(fd, (struct sockaddr*)&srv, sizeof(srv)) == -1) {
        perror("connect");
        close(fd);
        return FAILURE;
    }

    char buf[256];
    while (1) {
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        if (n == 0) break;
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("read");
            close(fd);
            return FAILURE;
        }
        buf[n] = '\0';
        fputs(buf, stdout);
    }

    close(fd);
    return SUCCESS;
}