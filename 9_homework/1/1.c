#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main() {
    const char* text = "String from file";
    char ch;
    /*flags for open if file not created, write only, set file size to 0 if already created*/
    int fd = open("output.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);

    if (fd < 0) {
        perror("error in open");
        return 1;
    }

    write(fd, text, strlen(text));
    close(fd);

    /*repeat open with new flag*/
    fd = open("output.txt", O_RDONLY);

    if (fd < 0) {
        perror("error in repeat open");
        return 1;
    }

    /*set offset of file*/
    off_t pos = lseek(fd, 0, SEEK_END);
    while (pos > 0) {
        lseek(fd, --pos, SEEK_SET);
        read(fd, &ch, 1);
        write(STDOUT_FILENO, &ch, 1);
    }

    write (STDOUT_FILENO, "\n", 1);
    close(fd);

    return 0;
}