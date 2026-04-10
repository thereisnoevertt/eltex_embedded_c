#include <mqueue.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 8192

int main(void) {
    mqd_t server_mq = mq_open("/ser_q", O_WRONLY, 0666, NULL);
    mqd_t client_mq = mq_open("/cli_q", O_RDONLY, 0666, NULL);

    if (server_mq == (mqd_t)-1) { 
        perror("mq_open"); 
        return 1; 
    }

    if (client_mq == (mqd_t)-1) { 
        perror("mq_open"); 
        return 1; 
    }

    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);
    if (mq_receive(client_mq, buf, BUF_SIZE, NULL) == -1) { 
        perror("mq_receive"); 
        return 1; 
    }

    write(STDOUT_FILENO, buf, strlen(buf));
    memset(buf, 0, BUF_SIZE);
    strncpy(buf, "Hello!", BUF_SIZE - 1);

    if (mq_send(server_mq, buf, strlen(buf) + 1, 0) == -1) { 
        perror("mq_send"); 
        return 1; 
    }

    if (mq_close(server_mq) == -1){ 
        perror("mq_close");   
        return 1; 
    }

    if (mq_close(client_mq) == -1){ 
        perror("mq_close");   
        return 1; 
    }

    return 0;
}