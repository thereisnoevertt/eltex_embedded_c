#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
 
#define HOST "127.0.0.1"
#define PORT 12345
 
int main(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { 
        perror("socket"); 
        return 1; 
    }
 
    struct sockaddr_in srv = {0};
    srv.sin_family      = AF_INET;
    srv.sin_port        = htons(PORT);
    inet_pton(AF_INET, HOST, &srv.sin_addr);
 
    if (connect(fd, (struct sockaddr *)&srv, sizeof(srv)) < 0) {
        perror("connect"); 
        close(fd); 
        return 1;
    }

    printf("[client] подключились к серверу\n");
 
    char buf[256];
    ssize_t n;
    while ((n = recv(fd, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        printf("[client] получено: %s", buf);
    }
    if (n < 0) perror("recv");
 
    close(fd);
    printf("[client] соединение закрыто\n");
    return 0;
}