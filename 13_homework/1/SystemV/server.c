// server.c
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#define MSG_SIZE 16
#define FILE_PATH "msgqueuefile"

struct msg_buffer {
    long msg_type;
    char msg_text[MSG_SIZE];
};

int main(void) {
    int fd = creat(FILE_PATH, 0666);
    if (fd == -1) { 
        perror("creat"); 
        return 1; 
    }

    close(fd);

    key_t key = ftok(FILE_PATH, 1);
    if (key == -1) { 
        perror("ftok"); 
        return 1; 
    }

    int msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) { 
        perror("msgget"); 
        return 1; 
    }

    struct msg_buffer message;
    message.msg_type = 1;
    strncpy(message.msg_text, "Hi!", MSG_SIZE - 1);
    message.msg_text[MSG_SIZE - 1] = '\0';

    if (msgsnd(msgid, &message, MSG_SIZE, 0) == -1) {
        perror("msgsnd"); 
        return 1;
    }

    if (msgrcv(msgid, &message, MSG_SIZE, 2, 0) == -1) {
        perror("msgrcv"); 
        return 1;
    }

    message.msg_text[MSG_SIZE - 1] = '\0';

    write(STDOUT_FILENO, message.msg_text, strlen(message.msg_text));
    write(STDOUT_FILENO, "\n", 1);

    if (msgctl(msgid, IPC_RMID, NULL) == -1) { 
        perror("msgctl");
        return 1; 
    }

    unlink(FILE_PATH);
    
    return 0;
}