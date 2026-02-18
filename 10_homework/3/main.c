#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SIZE 1024
#define MAX_ARGS 64

int main(void) {
    char line[MAX_SIZE];
    char *args[MAX_ARGS];

    while (1) {
        printf("$ ");
        fflush(stdout);

        if (!fgets(line, MAX_SIZE, stdin)) {
            printf("\n");
            break;
        }

        line[strcspn(line, "\n")] = '\0';

        int i = 0;
        args[i] = strtok(line, " ");
        while (args[i] != NULL && i < MAX_ARGS - 1) {
            args[++i] = strtok(NULL, " ");
        }

        args[i] = NULL;

        if (args[0] == NULL) {
            continue;
        }

        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        } 
        else {
            int status;
            waitpid(pid, &status, 0);

            if (WIFEXITED(status)) {
                printf("Process exited with status %d\n",
                       WEXITSTATUS(status));
            } 
        }
    }
    return 0;
}
