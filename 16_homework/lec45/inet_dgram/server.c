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

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == -1) {
        perror("setsockopt");
        close(fd);
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

    if (bind(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(fd);
        exit(EXIT_FAILURE);
    }

    /* UDP: нет listen и accept — сразу ждём датаграмму */
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    char buf[BUF_SIZE];

    /* recvfrom заполняет client_addr — узнаём откуда пришло */
    ssize_t received = recvfrom(fd, buf, sizeof(buf) - 1, 0,
                                (struct sockaddr*)&client_addr, &client_addr_size);
    if (received == -1) {
        perror("recvfrom");
        close(fd);
        exit(EXIT_FAILURE);
    }
    buf[received] = '\0';
    printf("Client said: %s", buf);

    /* отвечаем клиенту — указываем его адрес явно */
    const char *reply = "Hello!\n";
    if (sendto(fd, reply, strlen(reply), 0,
               (struct sockaddr*)&client_addr, client_addr_size) == -1) {
        perror("sendto");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
    return 0;
}