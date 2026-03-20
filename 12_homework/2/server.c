#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

int main() {
    int file = mkfifo("myfifo", 0666);

    if (file == -1) {
        perror("mkfifo");
        return 1;
    }

    int fd = open("myfifo", O_WRONLY);

    if (fd == -1) {
        perror("open");
        return 1;
    }

    char buf[3] = "Hi!";
    write(fd, buf, 3);

    close(fd);

    return 0;
}