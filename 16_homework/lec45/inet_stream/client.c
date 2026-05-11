#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define BUF_SIZE 1024

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(8080);

    int result = inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    if (result < 0) {
        perror("inet_pton");
        close(fd);
        exit(EXIT_FAILURE);
    } else if (result == 0) {
        fprintf(stderr, "Неверный формат адреса\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (connect(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(fd);
        exit(EXIT_FAILURE);
    }

    /* receive greeting from server */
    char buf[BUF_SIZE];
    ssize_t received = recv(fd, buf, sizeof(buf) - 1, 0);
    if (received == -1) {
        perror("recv");
        close(fd);
        exit(EXIT_FAILURE);
    }
    buf[received] = '\0';
    printf("Server said: %s", buf);

    /* send response */
    const char *reply = "Hi!\n";
    if (send(fd, reply, strlen(reply), 0) == -1) {
        perror("send");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
    return 0;
}