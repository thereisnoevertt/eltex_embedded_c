#include <mqueue.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 8192

int main(void) {
    mqd_t server_mq = mq_open("/ser_q", O_CREAT | O_RDONLY, 0666, NULL);
    mqd_t client_mq = mq_open("/cli_q", O_CREAT | O_WRONLY, 0666, NULL);
    
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
    strncpy(buf, "Hi!", BUF_SIZE - 1);

    if (mq_send(client_mq, buf, strlen(buf) + 1, 0) == -1) { 
        perror("mq_send"); 
        return 1; 
    }

    memset(buf, 0, BUF_SIZE);
    if (mq_receive(server_mq, buf, BUF_SIZE, NULL) == -1) { 
        perror("mq_receive"); 
        return 1; 
    }

    write(STDOUT_FILENO, buf, strlen(buf));

    if (mq_unlink("/ser_q") == -1) { 
        perror("mq_unlink"); 
        return 1; 
    }

    if (mq_close(server_mq) == -1){ 
        perror("mq_close");   
        return 1; 
    }

    if (mq_unlink("/cli_q") == -1) { 
        perror("mq_unlink"); 
        return 1; 
    }
    
    if (mq_close(client_mq) == -1){ 
        perror("mq_close");   
        return 1; 
    }

    return 0;
}