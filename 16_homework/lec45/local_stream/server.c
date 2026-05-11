#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SOCK_PATH "/tmp/file"
#define BUF_SIZE 1024

int main() {
    int s = socket(AF_LOCAL, SOCK_STREAM, 0);
    
    if (s == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /*init structure for addres*/
    struct sockaddr_un addr = { .sun_family = AF_LOCAL };
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    unlink(SOCK_PATH);
    
    /*binding sockfd with sockaddr with pointer on structure*/
    if (bind(s, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind");
        close(s);
        exit(EXIT_FAILURE);
    }

    if(listen(s, 1) == -1) {
        perror("listen");
        close(s);
        unlink(SOCK_PATH);
        exit(EXIT_FAILURE);
    }
    
    int client_fd = accept(s, NULL, NULL);
    if (client_fd == -1){
        perror("accept");
        close(s);
        unlink(SOCK_PATH);
        exit(EXIT_FAILURE);
    }
    
    const char *hello = "Hello";

    if(write(client_fd, hello, strlen(hello)) == -1) {
        perror("write");
        close(s);
        close(client_fd);
        unlink(SOCK_PATH);
        exit (EXIT_FAILURE);
    }

    char buf[BUF_SIZE];
    ssize_t r = read(client_fd, buf, sizeof(buf) - 1);
    if (r < 0) {
        perror("read");
        close(client_fd);
        close(s);
        unlink(SOCK_PATH);
        exit(EXIT_FAILURE);
    }
    buf[r] = '\0';

    printf("client said: %s", buf);

    close(s);
    close(client_fd);
    unlink(SOCK_PATH);

    return 0;
}