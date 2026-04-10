#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include "message.h"

typedef struct {
    mqd_t client_queue;
    pid_t client_pid;
    char  queue_name[64];
    char  name[NAME_LEN];
} ClientEntry;


static ClientEntry clients[MAX_CLIENTS];
static int         client_count = 0;

static Message hist_buf[HISTORY_SIZE];
static int     hist_head  = 0;
static int     hist_count = 0; 

static mqd_t server_queue = (mqd_t)-1;
static volatile sig_atomic_t running = 1;

static void sig_handler(int sig)
{
    (void)sig;
    running = 0;
}


static void history_add(const Message *msg)
{
    hist_buf[hist_head] = *msg;
    hist_head = (hist_head + 1) % HISTORY_SIZE;
    if (hist_count < HISTORY_SIZE)
        hist_count++;
}

static void send_history(mqd_t cq)
{
    int start = (hist_head - hist_count + HISTORY_SIZE) % HISTORY_SIZE;
    for (int i = 0; i < hist_count; i++) {
        Message h = hist_buf[(start + i) % HISTORY_SIZE];
        h.type = MSG_HIST;
        if (mq_send(cq, (const char *)&h, sizeof(h), 0) == -1)
            perror("mq_send history");
    }
}

static void broadcast(const Message *msg, pid_t skip_pid)
{
    for (int i = 0; i < client_count; i++) {
        if (clients[i].client_pid == skip_pid)
            continue;
        if (mq_send(clients[i].client_queue,
                    (const char *)msg, sizeof(*msg), 0) == -1) {
            fprintf(stderr, "[Server] mq_send to %s failed: %s\n",
                    clients[i].name, strerror(errno));
        }
    }
}


static void remove_client(int idx)
{
    printf("[Server] %s (pid=%d) left\n",
           clients[idx].name, clients[idx].client_pid);

    Message lv;
    memset(&lv, 0, sizeof(lv));
    lv.type = MSG_LEAVE_NOTIFY;
    lv.sender_pid = clients[idx].client_pid;
    strncpy(lv.name, clients[idx].name, NAME_LEN - 1);
    snprintf(lv.text, TEXT_LEN, "%s left the chat", clients[idx].name);
    broadcast(&lv, clients[idx].client_pid);

    mq_close(clients[idx].client_queue);

    for (int i = idx; i < client_count - 1; i++)
        clients[i] = clients[i + 1];
    client_count--;
}


static void cleanup(void)
{
    for (int i = 0; i < client_count; i++)
        if (clients[i].client_queue != (mqd_t)-1)
            mq_close(clients[i].client_queue);

    if (server_queue != (mqd_t)-1) {
        mq_close(server_queue);
        server_queue = (mqd_t)-1;
    }
    mq_unlink(MAIN_QUEUE_NAME);
}

int main(void)
{
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);

    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg  = 10;
    attr.mq_msgsize = sizeof(Message);

    mq_unlink(MAIN_QUEUE_NAME);
server_queue = mq_open(MAIN_QUEUE_NAME,
                           O_CREAT | O_RDONLY, 0666, &attr);
    if (server_queue == (mqd_t)-1) {
        perror("mq_open server");
        return 1;
    }

    printf("[Server] Started on queue %s  (pid=%d)\n",
           MAIN_QUEUE_NAME, (int)getpid());
    printf("[Server] Press Ctrl+C to stop\n");

    while (running) {
        Message msg;
        memset(&msg, 0, sizeof(msg));

        ssize_t bytes = mq_receive(server_queue,
                                   (char *)&msg, sizeof(msg), NULL);
        if (bytes == -1) {
            if (errno == EINTR) continue;
            perror("mq_receive");
            break;
        }

        switch (msg.type) {

        case MSG_JOIN_REQ: {
            int dup = 0;
            for (int i = 0; i < client_count; i++)
                if (clients[i].client_pid == msg.sender_pid) { dup = 1; break; }
            if (dup) {
                printf("[Server] Duplicate join from pid=%d, ignored\n",
                       msg.sender_pid);
                break;
            }
            if (client_count >= MAX_CLIENTS) {
                fprintf(stderr, "[Server] Max clients (%d) reached\n",
                        MAX_CLIENTS);
                break;
            }

            mqd_t cq = mq_open(msg.queue_name, O_WRONLY);
            if (cq == (mqd_t)-1) {
                fprintf(stderr, "[Server] Cannot open client queue %s: %s\n",
                        msg.queue_name, strerror(errno));
                break;
            }

            send_history(cq);

            {
                Message ok;
                memset(&ok, 0, sizeof(ok));
                ok.type = MSG_JOIN_OK;
                ok.sender_pid = getpid();
                strncpy(ok.name, msg.name, NAME_LEN - 1);
                snprintf(ok.text, TEXT_LEN,
                         "Welcome to the chat, %s!", msg.name);
                if (mq_send(cq, (const char *)&ok, sizeof(ok), 0) == -1)
                    perror("mq_send JOIN_OK");
            }

            clients[client_count].client_queue = cq;
            clients[client_count].client_pid   = msg.sender_pid;
            strncpy(clients[client_count].queue_name, msg.queue_name, 63);
            strncpy(clients[client_count].name, msg.name, NAME_LEN - 1);
            client_count++;

            {
                Message notify;
                memset(&notify, 0, sizeof(notify));
                notify.type = MSG_JOIN_NOTIFY;
                notify.sender_pid = msg.sender_pid;
                strncpy(notify.name, msg.name, NAME_LEN - 1);
                snprintf(notify.text, TEXT_LEN,
                         "%s joined the chat", msg.name);
                broadcast(&notify, msg.sender_pid);
            }

            printf("[Server] %s (pid=%d) joined  [%d/%d clients]\n",
                   msg.name, msg.sender_pid, client_count, MAX_CLIENTS);
            break;
        }

        case MSG_TEXT: {
            printf("[Server] %s: %s\n", msg.name, msg.text);
            history_add(&msg);
            broadcast(&msg, -1);
            break;
        }

        case MSG_LEAVE: {
            for (int i = 0; i < client_count; i++) {
                if (clients[i].client_pid == msg.sender_pid) {
                    remove_client(i);
                    break;
                }
            }
            break;
        }

        default:
            fprintf(stderr, "[Server] Unknown message type %d\n", msg.type);
        }
    }

    cleanup();
    printf("[Server] Stopped\n");
    return 0;
}