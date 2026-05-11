#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#define FAILURE 1
#define SUCCESS 0
#define HOST "127.0.0.1"
#define PORT 12345
#define POOL_SIZE 5

static int serv_fd; 

typedef struct {
    int fd;
    struct sockaddr_in peer;
} client_arg_t;

void current_time_str(char *buf, size_t bufsize){
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, bufsize, "%Y-%m-%d %H:%M:%S", tm);
}

static void *handle_client(void *arg) {
    (void)arg;

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t client_size = sizeof(client_addr);

    int client_fd = accept(serv_fd,
                           (struct sockaddr *)&client_addr,
                           &client_size);
    if (client_fd == -1) {
        perror("accept");
        return NULL;
    }

    char addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr,
              addr_str, sizeof(addr_str));
    int port = ntohs(client_addr.sin_port);
    printf("[server] клиент %s:%d подключился\n", addr_str, port);

    char timebuf[64];
    current_time_str(timebuf, sizeof(timebuf));

    char msg[128];
    int msglen = snprintf(msg, sizeof(msg), "TIME: %s\n", timebuf);

    if (send(client_fd, msg, msglen, MSG_NOSIGNAL) < 0)
        perror("[server] send");

    printf("[server] отправлено: %s", msg);
    close(client_fd);
    printf("[server] клиент %s:%d отключён\n", addr_str, port);
    return NULL;
}

int main() {
    signal(SIGPIPE, SIG_IGN);

    serv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_fd == -1) {
        perror("socket");
        return FAILURE;
    }
    int opt = 1;

    if (setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror ("setsockopt");
        return FAILURE;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    /*convert string of addres to in_addr_t to set addr in structure*/
    int result = inet_pton(AF_INET, HOST, &server_addr.sin_addr);
    if (result <0) {
        perror("inet_pton");
        close(serv_fd);
        return FAILURE;
    }

    else if (result == 0) {
        fprintf(stderr, "Неверный формат адреса\n");
        close(serv_fd);
        return FAILURE;
    }
    if (bind(serv_fd, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in)) == -1){
        perror("bind");
        return FAILURE;
    }

    if (listen(serv_fd, 5) == -1) {
        perror("listen");
        close (serv_fd);
        return FAILURE;
    }

    pthread_t pool[POOL_SIZE];
    for (int i = 0; i < POOL_SIZE; i++) {
        if (pthread_create(&pool[i], NULL, handle_client, NULL) != 0) {
            perror("pthread_create");
            close(serv_fd);
            return FAILURE;
        }
    }

    for (int i = 0; i < POOL_SIZE; i++) {
        pthread_join(pool[i], NULL);
    }

    close(serv_fd);
    return SUCCESS;
}