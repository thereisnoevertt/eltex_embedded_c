#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define BUF_SIZE 1024

int main() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
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

    const char *msg = "Hi!\n";
    if (sendto(fd, msg, strlen(msg), 0,
               (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("sendto");
        close(fd);
        exit(EXIT_FAILURE);
    }

    char buf[BUF_SIZE];
    struct sockaddr_in from_addr;
    socklen_t from_addr_size = sizeof(from_addr);

    ssize_t received = recvfrom(fd, buf, sizeof(buf) - 1, 0,
                                (struct sockaddr*)&from_addr, &from_addr_size);
    if (received == -1) {
        perror("recvfrom");
        close(fd);
        exit(EXIT_FAILURE);
    }
    buf[received] = '\0';
    printf("Server said: %s", buf);

    close(fd);
    return 0;
}