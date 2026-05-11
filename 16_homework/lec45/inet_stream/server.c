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

    int opt = 1;
    
    /*set socket's options to set SO_REUSEADDR*/
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == -1) {
        perror("setsockopt");
        close(fd);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);

    /*convert string of addres to in_addr_t to set addr in structure*/
    int result = inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    if (result <0) {
        perror("inet_pton");
        close(fd);
        exit(EXIT_FAILURE);
    }

    else if (result == 0) {
        fprintf(stderr, "Неверный формат адреса\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    /*binding addres with socket*/
    if (bind(fd, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in)) == -1) {
        perror("bind");
        close(fd);
        exit(EXIT_FAILURE);
    }

    /*listen*/
    if(listen(fd, 5) == -1) {
        perror("listen");
        close(fd);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);

    /*accepting client's connection*/
    int client_fd = accept(fd, (struct sockaddr*) &client_addr, &client_addr_size);
    if (client_fd == -1) {
        perror("accept");
        close(fd);
        exit(EXIT_FAILURE);
    }

    const char* hello = "Hello!\n";
    if (send(client_fd, hello, strlen(hello), 0) < 0) {
        perror("send");
        close(fd);
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    char buf[1024];
    ssize_t recieved_bytes = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (recieved_bytes == -1) {
        perror("recv");
        close(fd);
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    buf[recieved_bytes] = '\0';
    printf ("Client said: %s", buf);

    close(client_fd);
    close(fd);
}