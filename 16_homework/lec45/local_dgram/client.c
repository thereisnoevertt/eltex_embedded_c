#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

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
    strncpy(addr.sun_path, CLIENT_SOCK_PATH, sizeof(addr.sun_path) - 1);

    unlink(CLIENT_SOCK_PATH);

    if (bind(s, (struct sockaddr*) &addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind");
        close(s);
        exit(EXIT_FAILURE);
    }

    char buf[BUF_SIZE];
    struct sockaddr_un server_addr;
    socklen_t server_len = sizeof(server_addr);
    ssize_t r = recvfrom(s, buf, sizeof(buf) - 1, 0, 
        (struct sockaddr*) &server_addr, &server_len);

    if (r < 0) {
        perror("recvfrom");
        close(s);
        unlink(CLIENT_SOCK_PATH);
        exit(EXIT_FAILURE);
    }
    buf[r] = '\0';
    
    printf("Server said: %s\n", buf);

    const char* hi = "Hi!";
    if (sendto(s, hi, strlen(hi), 0, 
    (struct sockaddr*) &server_addr, server_len) == -1){
        perror("sendto");
        close(s);
        unlink(CLIENT_SOCK_PATH);
        exit(EXIT_FAILURE);
    }

    close(s);
    unlink(CLIENT_SOCK_PATH);
    return 0;
}