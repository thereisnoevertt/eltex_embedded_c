#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_SOCK_PATH "/tmp/server_sock"
#define CLIENT_SOCK_PATH "/tmp/client_sock"
#define BUF_SIZE 1024

int main() {
    int s = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (s == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr = {
        .sun_family = AF_LOCAL
    };
    strncpy(addr.sun_path, SERVER_SOCK_PATH, sizeof(addr.sun_path) - 1);

    unlink(SERVER_SOCK_PATH);

    if (bind(s, (struct sockaddr*) &addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind");
        close(s);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un client_addr = {
        .sun_family = AF_LOCAL
    };
    strncpy(client_addr.sun_path, CLIENT_SOCK_PATH, sizeof(client_addr.sun_path) - 1);

    const char* hello = "Hello!";
    if (sendto(s, hello, strlen(hello), 0, 
    (struct sockaddr*) &client_addr, sizeof(client_addr)) == -1){
        perror("sendto");
        close(s);
        unlink(SERVER_SOCK_PATH);
        exit(EXIT_FAILURE);
    }

    char buf[BUF_SIZE];
    ssize_t r = recvfrom(s, buf, sizeof(buf) - 1, 0, NULL, NULL);
    if (r < 0) {
        perror("recvfrom");
        close(s);
        unlink(SERVER_SOCK_PATH);
        exit(EXIT_FAILURE);
    }
    buf[r] = '\0';

    printf("Client said: %s\n", buf);

    close(s);
    unlink(SERVER_SOCK_PATH);
    return 0;
}