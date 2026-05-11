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

    struct sockaddr_un addr = {
        .sun_family = AF_LOCAL
    };
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (connect(s, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
        perror("connect");
        close(s);
        exit(EXIT_FAILURE);
    }

    char buf[BUF_SIZE];
    ssize_t r = read(s, buf, sizeof(buf));
    if (r == -1) {
        perror("read");
        close(s);
        exit(EXIT_FAILURE);
    }
    buf[r] = '\0';

    printf("Server said: %s\n", buf);

    const char* reply = "Hi!\n";
    if (write(s, reply, strlen(reply)) == -1) {
        perror("write");
        close(s);
        exit(EXIT_FAILURE);
    }

    close(s);
    return 0;
}