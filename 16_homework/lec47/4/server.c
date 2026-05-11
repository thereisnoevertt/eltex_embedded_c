#define _POSIX_C_SOURCE 200809L
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <stdint.h>
 
#define TCP_PORT 8080
#define UDP_PORT 8081
#define HOST "127.0.0.1"
#define SUCCESS 0
#define FAILURE -1
#define MAX_EVENTS 64
#define TIME_BUF_SIZE 64
#define BACKLOG 16
 
typedef enum {
    FD_TCP_LISTEN,
    FD_UDP,
    FD_TCP_CLIENT
} fd_type_t;
 
typedef struct {
    int fd;
    fd_type_t type;
    char buf[TIME_BUF_SIZE];
    int buf_len;
    int buf_sent;
} ctx_t;
 
static volatile sig_atomic_t g_running = 1;
 
static void on_sigint(int sig) {
    (void)sig;
    g_running = 0;
}
 
static void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) { 
        perror("fcntl F_GETFL"); 
        return; 
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
    }
}
 
static int format_time(char *buf, size_t bufsize) {
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    return (int)strftime(buf, bufsize, "%Y-%m-%d %H:%M:%S\n", &tm);
}
 
static int make_tcp_server(uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) { 
        perror("socket TCP"); 
        return FAILURE; 
    }
 
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt SO_REUSEADDR (TCP)");
        close(fd); return FAILURE;
    }
    set_nonblock(fd);
 
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    if (inet_pton(AF_INET, HOST, &addr.sin_addr) <= 0) {
        perror("inet_pton TCP");
        close(fd); return FAILURE;
    }
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind TCP");
        close(fd); return FAILURE;
    }
    if (listen(fd, BACKLOG) == -1) {
        perror("listen");
        close(fd); return FAILURE;
    }
    return fd;
}
 
static int make_udp_server(uint16_t port) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) { 
        perror("socket UDP"); 
        return FAILURE; 
    }
 
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt SO_REUSEADDR (UDP)");
        close(fd); return FAILURE;
    }
    set_nonblock(fd);
 
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    if (inet_pton(AF_INET, HOST, &addr.sin_addr) <= 0) {
        perror("inet_pton UDP");
        close(fd); return FAILURE;
    }
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind UDP");
        close(fd); return FAILURE;
    }
    return fd;
}
 
ctx_t* add_to_epoll(int fd, int epfd, fd_type_t type, uint32_t events) {
    ctx_t *ctx = calloc(1, sizeof(ctx_t));
    if (!ctx) { perror("calloc"); return NULL; }
    ctx->fd   = fd;
    ctx->type = type;
 
    struct epoll_event ev = { .events = events, .data.ptr = ctx };
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl ADD");
        free(ctx);
        return NULL;
    }
    return ctx;
}
 
int mod_in_epoll(int epfd, ctx_t *ctx, uint32_t events) {
    struct epoll_event ev = { .events = events, .data.ptr = ctx };
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, ctx->fd, &ev) == -1) {
        perror("epoll_ctl MOD");
        return FAILURE;
    }
    return SUCCESS;
}
 
void close_client(int epfd, ctx_t *ctx) {
    epoll_ctl(epfd, EPOLL_CTL_DEL, ctx->fd, NULL);
    close(ctx->fd);
    free(ctx);
}

void try_send_remaining(int epfd, ctx_t *ctx) {
    while (ctx->buf_sent < ctx->buf_len) {
        ssize_t n = send(ctx->fd,
                         ctx->buf + ctx->buf_sent,
                         ctx->buf_len - ctx->buf_sent,
                         MSG_NOSIGNAL);
        if (n > 0) {
            ctx->buf_sent += n;
            continue;
        }
        if (n == -1) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                mod_in_epoll(epfd, ctx, EPOLLOUT | EPOLLET);
                return;
            }
            perror("send");
            close_client(epfd, ctx);
            return;
        }
    }
    close_client(epfd, ctx);
}

 
static void on_tcp_accept(int epfd, int listen_fd) {
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(listen_fd,
                               (struct sockaddr*)&client_addr,
                               &client_len);
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            if (errno == EINTR) continue;
            perror("accept");
            break;
        }
        set_nonblock(client_fd);
 
        ctx_t *ctx = calloc(1, sizeof(ctx_t));
        if (!ctx) {
            perror("calloc client ctx");
            close(client_fd);
            continue;
        }
        ctx->fd       = client_fd;
        ctx->type     = FD_TCP_CLIENT;
        ctx->buf_len  = format_time(ctx->buf, sizeof(ctx->buf));
        ctx->buf_sent = 0;
 
        struct epoll_event ev = {
            .events   = EPOLLOUT | EPOLLET,
            .data.ptr = ctx,
        };
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
            perror("epoll_ctl ADD client");
            close(client_fd);
            free(ctx);
            continue;
        }
 
        printf("[TCP] подключение от %s:%d, fd=%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               client_fd);
 
        try_send_remaining(epfd, ctx);
    }
}
 
static void on_tcp_writable(int epfd, ctx_t *ctx) {
    try_send_remaining(epfd, ctx);
}
 
static void on_udp_request(int udp_fd) {
    while (1) {
        char req[256];
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
 
        ssize_t n = recvfrom(udp_fd, req, sizeof(req), 0,
                             (struct sockaddr*)&client_addr,
                             &client_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            if (errno == EINTR) continue;
            perror("recvfrom");
            break;
        }
 
        char time_buf[TIME_BUF_SIZE];
        int len = format_time(time_buf, sizeof(time_buf));
 
        ssize_t sent = sendto(udp_fd, time_buf, len, 0,
                              (struct sockaddr*)&client_addr,
                              client_len);
        if (sent < 0) {
            perror("sendto");
            continue;
        }
 
        printf("[UDP] %.*s -> %s:%d\n",
               len - 1, time_buf,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
    }
}


int main(void) {
    signal(SIGINT,  on_sigint);
    signal(SIGTERM, on_sigint);
    signal(SIGPIPE, SIG_IGN);
 
    int epfd = epoll_create1(0);
    if (epfd == -1) { 
        perror("epoll_create1"); 
        return FAILURE; 
    }
 
    int tcp_fd = make_tcp_server(TCP_PORT);
    int udp_fd = make_udp_server(UDP_PORT);
    if (tcp_fd == FAILURE || udp_fd == FAILURE) {
        if (tcp_fd >= 0) {
            close(tcp_fd);
        }
        if (udp_fd >= 0) {
            close(udp_fd);
        }

        close(epfd);
        return FAILURE;
    }
 
    ctx_t *tcp_ctx = add_to_epoll(tcp_fd, epfd, FD_TCP_LISTEN,
                                  EPOLLIN | EPOLLET);
    ctx_t *udp_ctx = add_to_epoll(udp_fd, epfd, FD_UDP,
                                  EPOLLIN | EPOLLET);
    if (!tcp_ctx || !udp_ctx) {
        close(tcp_fd); close(udp_fd); close(epfd);
        return FAILURE;
    }
 
    printf("Сервер слушает TCP %s:%d и UDP %s:%d. Ctrl+C для остановки.\n",
           HOST, TCP_PORT, HOST, UDP_PORT);
 
    struct epoll_event events[MAX_EVENTS];
    while (g_running) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (n == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }
        for (int i = 0; i < n; i++) {
            ctx_t   *ctx = events[i].data.ptr;
            uint32_t ev  = events[i].events;
 
            if (ev & (EPOLLERR | EPOLLHUP)) {
                if (ctx->type == FD_TCP_CLIENT) {
                    close_client(epfd, ctx);
                    continue;
                }
                fprintf(stderr, "EPOLLERR/HUP на серверном fd=%d\n", ctx->fd);
                g_running = 0;
                break;
            }
 
            switch (ctx->type) {
                case FD_TCP_LISTEN:
                    on_tcp_accept(epfd, ctx->fd);
                    break;
                case FD_UDP:
                    on_udp_request(ctx->fd);
                    break;
                case FD_TCP_CLIENT:
                    on_tcp_writable(epfd, ctx);
                    break;
            }
        }
    }
 
    printf("Завершение работы...\n");
    close(tcp_fd);
    close(udp_fd);
    close(epfd);
    free(tcp_ctx);
    free(udp_ctx);
    return SUCCESS;
}