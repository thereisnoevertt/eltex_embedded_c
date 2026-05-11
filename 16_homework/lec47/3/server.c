#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define POOL_SIZE 10
#define QUEUE_CAP 256
#define FAILURE 1
#define SUCCESS 0
#define HOST "127.0.0.1"
#define PORT 12345

static volatile sig_atomic_t running = 1;

static void on_signal(int sig) {
    (void)sig;
    running = 0;
}

static void setup_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

typedef struct {
    int fd;
    struct sockaddr_in peer;
} task_t;

typedef struct {
    task_t      items[QUEUE_CAP];
    int      head, tail, size;
    pthread_mutex_t mu;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;
} fd_queue_t;

static fd_queue_t queue;

void current_time_str(char *buf, size_t bufsize) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, bufsize, "%Y-%m-%d %H:%M:%S", tm);
}

void queue_init(fd_queue_t *q) {
    q->head = q->tail = q->size = 0;
    pthread_mutex_init(&q->mu, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
}

void queue_push(fd_queue_t *q, task_t t) {
    pthread_mutex_lock(&q->mu);
    while (q->size == QUEUE_CAP) {
        pthread_cond_wait(&q->not_full, &q->mu);
    }
    q->items[q->tail] = t;
    q->tail = (q->tail + 1) % QUEUE_CAP;
    q->size++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mu);
}

void queue_pop(fd_queue_t *q, task_t* t) {
    pthread_mutex_lock(&q->mu);
    while (q->size == 0) {
        pthread_cond_wait(&q->not_empty, &q->mu);
    }
    *t = q->items[q->head];
    q->head = (q->head + 1) % QUEUE_CAP;
    q->size--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mu);
}

void* worker(void* arg) {
    (void)arg;
    while (1) {
        task_t t;
        queue_pop(&queue, &t);
        if (t.fd == -1) {
            break; // poison pill
        }
        char addr_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &t.peer.sin_addr,
                    addr_str, sizeof(addr_str));
        int port = ntohs(t.peer.sin_port);
        printf("[server] клиент %s:%d подключился\n", addr_str, port);

        char timebuf[64];
        current_time_str(timebuf, sizeof(timebuf));

        char msg[128];
        int msglen = snprintf(msg, sizeof(msg), "TIME: %s\n", timebuf);

        if (send(t.fd, msg, msglen, MSG_NOSIGNAL) < 0) {
            perror("[server] send");
        }
        printf("[server] отправлено: %s", msg);
        close(t.fd);
        printf("[server] клиент %s:%d отключён\n", addr_str, port);
    }
    return NULL;
}


int main() {
    setup_signals();

    queue_init(&queue);

    pthread_t pool[POOL_SIZE];
    for (int i = 0; i < POOL_SIZE; i++) {
        if (pthread_create(&pool[i], NULL, worker, NULL) != 0) {
            perror("pthread_create");
            return FAILURE;
        }
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return FAILURE;
    }
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        return FAILURE;
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    int result = inet_pton(AF_INET, HOST, &server_addr.sin_addr);
    if (result < 0) {
        perror("inet_pton");
        close(fd);
        return FAILURE;
    }

    if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(fd);
        return FAILURE;
    }

    if (listen(fd, 128) == -1) {
        perror("listen");
        close(fd);
        return FAILURE;
    }

    while (1) {
        task_t  t;
        socklen_t client_size = sizeof(t.peer);
        t.fd = accept(fd, (struct sockaddr *)&t.peer, &client_size);
        if (t.fd == -1) {
        if (errno == EINTR) break;
        perror("accept");
        continue;
    }
        queue_push(&queue, t);
    }
    close(fd);

    /*killing thread at ctrl+c*/
    for (int i = 0; i < POOL_SIZE; i++) {
        task_t poison = { 
            .fd = -1 
        };
        queue_push(&queue, poison);
    }

    for (int i = 0; i < POOL_SIZE; i++) {
        pthread_join(pool[i], NULL);
    }

    pthread_mutex_destroy(&queue.mu);
    pthread_cond_destroy(&queue.not_empty);
    pthread_cond_destroy(&queue.not_full);
    
    return SUCCESS;
}