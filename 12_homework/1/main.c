#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    int pipefd[2] = {0, 1};
    pipe(pipefd);

    if (pipefd[0] < 0 || pipefd[1] < 0) {
        perror("pipe");
        return 1;
    }

    int status;

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        close(pipefd[1]);
        char buffer[3];
        read(pipefd[0], buffer, 3);
        write(STDOUT_FILENO, buffer, 3);
        close(pipefd[0]);
        exit(0);
    }

    close(pipefd[0]);
    char buffer[3] = {"Hi!"};
    write(pipefd[1], buffer, 3);
    close(pipefd[1]);

    pid_t finished_process = waitpid (pid, &status, 0);

    if (finished_process < 0) {
        perror("waitpid");
        return 1;
    }

    return 0;
}