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

        // Ищем '|'
        int pipe_pos = -1;
        for (int j = 0; args[j] != NULL; j++) {
            if (strcmp(args[j], "|") == 0) {
                pipe_pos = j;
                break;
            }
        }

        if (pipe_pos != -1) {
            // Разбиваем на две команды
            char *left[MAX_ARGS];
            char *right[MAX_ARGS];

            for (int j = 0; j < pipe_pos; j++)
                left[j] = args[j];
            left[pipe_pos] = NULL;

            int j = 0;
            for (int k = pipe_pos + 1; args[k] != NULL; k++)
                right[j++] = args[k];
            right[j] = NULL;

            int fds[2];
            if (pipe(fds) == -1) {
                perror("pipe");
                continue;
            }

            pid_t pid1 = fork();
            if (pid1 < 0) {
                perror("fork");
                continue;
            }
            if (pid1 == 0) {
                close(fds[0]);
                dup2(fds[1], STDOUT_FILENO);
                close(fds[1]);
                execvp(left[0], left);
                perror("execvp left");
                exit(EXIT_FAILURE);
            }

            pid_t pid2 = fork();
            if (pid2 < 0) {
                perror("fork");
                continue;
            }
            if (pid2 == 0) {
                close(fds[1]);
                dup2(fds[0], STDIN_FILENO);
                close(fds[0]);
                execvp(right[0], right);
                perror("execvp right");
                exit(EXIT_FAILURE);
            }

            close(fds[0]);
            close(fds[1]);
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);

        } 
        else {

            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                continue;
            }
            if (pid == 0) {
                execvp(args[0], args);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else {
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)) {
                    printf("Process exited with status %d\n", WEXITSTATUS(status));
                }
            }
        }
    }

    return 0;
}