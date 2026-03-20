#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    int fd = open("myfifo", O_RDONLY);

    if (fd == -1) {
        perror("open");
        return 1;
    }

    char buf[3];
    read(fd, buf, 3);
    close(fd);
    write(STDOUT_FILENO, buf, 3);
    unlink("myfifo");
    return 0;
}