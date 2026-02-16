#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>

int main() {
    pid_t pid = fork();
    if (pid < 0){
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        printf("My pid %d\n", getpid());
        printf("My parent pid %d\n", getppid());
        sleep(2);
        exit(52);
    }
    else {
        printf("My pid %d\n", getpid());
        printf("My parent's pid %d\n", getppid());

        int status;
        pid_t finished_process = waitpid(pid, &status, 0);

        if (finished_process == -1){
            perror("waitpid");
            return 1;
        }

        if (WIFEXITED(status)) {
            printf("Child finished normal with %d status\n", WEXITSTATUS(status));
        }
    }
    return 0;
}