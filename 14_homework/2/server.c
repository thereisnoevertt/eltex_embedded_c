#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/types.h>

#define HISTORY_SIZE 10
#define MAX_CLIENTS 10
#define NAME_LEN 32
#define TEXT_LEN 256
#define SHM_NAME "/chat_shm"

typedef enum {
    MSG_JOIN_REQ = 1,
    MSG_JOIN_OK = 2,
    MSG_TEXT = 3,
    MSG_JOIN_NOTIFY = 4,
    MSG_LEAVE = 5,
    MSG_LEAVE_NOTIFY = 6,
} MsgType;

typedef struct {
    pid_t client_pid;
    char name[NAME_LEN];
} ClientEntry;

typedef struct {
    ClientEntry owner;
    MsgType type;
    pid_t sender_pid;
    char to[NAME_LEN];
    char text[TEXT_LEN];
} Message;

typedef struct {
    Message msgs[HISTORY_SIZE];
    int msg_count;
    int write_index;
} shared_chat;

static ClientEntry clients[MAX_CLIENTS];
static int client_count = 0;
static volatile sig_atomic_t running = 1;

int check_unique_name(char *name) {
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].name, name) == 0 ||
            strcmp(clients[i].name, "Server") == 0) {
            return 0;
        }
    }
    return 1;
}

static void sig_handler(int sig) {
    (void)sig;
    running = 0;
}

void push_message(shared_chat* chat, Message* msg) {
    chat->msgs[chat->write_index] = *msg;
    chat->msg_count++;
    chat->write_index = (chat->write_index + 1) % HISTORY_SIZE;
}

static void broadcast(shared_chat* chat, Message* src, int type) {
    Message msg;
    memset(&msg, 0, sizeof(msg));

    msg.type = type;
    msg.sender_pid = getpid();
    strncpy(msg.owner.name, "Server", NAME_LEN - 1);
    msg.owner.name[NAME_LEN - 1] = '\0';
    strncpy(msg.to, "*", NAME_LEN - 1);
    msg.to[NAME_LEN - 1] = '\0';

    if (type == MSG_JOIN_NOTIFY) {
        snprintf(msg.text, TEXT_LEN, "%s joined", src->owner.name);
    } else if (type == MSG_LEAVE_NOTIFY) {
        snprintf(msg.text, TEXT_LEN, "%s left", src->owner.name);
    }

    push_message(chat, &msg);
}

static void remove_client(Message msg) {
    int idx = -1;

    for (int i = 0; i < client_count; i++) {
        if (clients[i].client_pid == msg.sender_pid) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        printf("[Server] Unknown client leave (pid=%d)\n", msg.sender_pid);
        return;
    }

    char name[NAME_LEN];
    strncpy(name, clients[idx].name, NAME_LEN - 1);
    name[NAME_LEN - 1] = '\0';

    for (int i = idx; i < client_count - 1; i++) {
        clients[i] = clients[i + 1];
    }
    client_count--;

    printf("[Server] %s left (pid=%d)\n", name, msg.sender_pid);
}


static void cleanup(shared_chat* chat, sem_t* event_sem, sem_t* mutex_sem) {
    sem_close(event_sem);
    sem_close(mutex_sem);
    sem_unlink("/event_sem");
    sem_unlink("/mutex_sem");

    munmap(chat, sizeof(shared_chat));
    shm_unlink(SHM_NAME);
}

int main() {
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    int last_processed = 0;

    sem_t* event_sem = sem_open("/event_sem", O_CREAT, 0666, 0);
    sem_t* mutex_sem = sem_open("/mutex_sem", O_CREAT, 0666, 1);

    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return (EXIT_FAILURE);
    }
    if (ftruncate(fd, sizeof(shared_chat)) == -1) {
        perror("ftruncate");
        return (EXIT_FAILURE);
    }

    shared_chat* chat = mmap(NULL, sizeof(shared_chat),
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED, fd, 0);
    if (chat == MAP_FAILED) {
        perror("mmap");
        return (EXIT_FAILURE);
    }

    close(fd);

    chat->msg_count = 0;
    chat->write_index = 0;

    printf("[Server] started (pid=%d)\n", getpid());

    while (running) {

        sem_wait(event_sem);
        sem_wait(mutex_sem);

        int wrote = 0;

        if (chat->msg_count - last_processed > HISTORY_SIZE) {
            printf("[Server] overflow\n");
            last_processed = chat->msg_count - HISTORY_SIZE;
        }

        while (last_processed < chat->msg_count) {

            Message msg = chat->msgs[last_processed % HISTORY_SIZE];
            switch (msg.type) {

                case MSG_JOIN_REQ: {

                    if (!check_unique_name(msg.owner.name)) {
                        printf("[Server] name taken: %s\n", msg.owner.name);
                        break;
                    }

                    if (client_count >= MAX_CLIENTS) {
                        printf("[Server] max clients reached\n");
                        break;
                    }

                    clients[client_count++] = msg.owner;

                    Message ok;
                    memset(&ok, 0, sizeof(ok));

                    ok.type = MSG_JOIN_OK;
                    ok.sender_pid = getpid();
                    strncpy(ok.owner.name, "Server", NAME_LEN);
                    ok.owner.name[NAME_LEN - 1] = '\0';
                    strncpy(ok.to, msg.owner.name, NAME_LEN);
                    ok.to[NAME_LEN - 1] = '\0';
                    strncpy(ok.text, "Welcome!", TEXT_LEN);
                    ok.text[TEXT_LEN - 1] = '\0';

                    push_message(chat, &ok);
                    wrote++;

                    broadcast(chat, &msg, MSG_JOIN_NOTIFY);
                    wrote++;

                    printf("[Server] %s joined\n", msg.owner.name);

                    break;
                }

                case MSG_TEXT: {
                    printf("[Server] %s: %s\n", msg.owner.name, msg.text);
                    break;
                }

                case MSG_LEAVE: {

                    remove_client(msg);

                    broadcast(chat, &msg, MSG_LEAVE_NOTIFY);
                    wrote++;

                    break;
                }

                default:
                    printf("[Server] unknown msg\n");
                    break;
            }

            last_processed++;
        }

        sem_post(mutex_sem);

        if (wrote > 0) {
            sem_post(event_sem);
        }
    }

    cleanup(chat, event_sem, mutex_sem);

    printf("[Server] stopped\n");
    return 0;
}