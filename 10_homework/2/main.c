#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

#define SUCCES 52

int main() {
    int status1, status2;

    pid_t pid1 = fork();

    if (pid1 < 0) {
        perror("fork for pid1");
        exit(1);
    }

    if (pid1 == 0) {
        //process 1
        int status3, status4;

        pid_t pid3 = fork();

        if (pid3 < 0) { 
            perror("fork for pid3"); 
            exit(1); 
        }

        if (pid3 == 0) {
            printf("I am process 3\nMy pid %d\nMy parent pid %d\n", getpid(), getppid());
            sleep(1);
            exit(SUCCES);
        }

        pid_t pid4 = fork();

        if (pid4 < 0) { 
            perror("fork for pid4"); 
            exit(1); 
        }

        if (pid4 == 0) {
            printf("I am process 4\nMy pid %d\nMy parent pid %d\n", getpid(), getppid());
            sleep(1);
            exit(SUCCES);
        }

        /* parent in branch 1 waits for children 3 and 4 */
        waitpid(pid3, &status3, 0);
        waitpid(pid4, &status4, 0);

        if (WIFEXITED(status3) && WEXITSTATUS(status3) == SUCCES &&
            WIFEXITED(status4) && WEXITSTATUS(status4) == SUCCES) {
            printf("I am process 1\nMy pid %d\nMy parent pid %d\n", getpid(), getppid());
            sleep(1);
            exit(SUCCES);
        }
    }

    //parent creates process 2
    if (pid1 > 0) {
        int status5;
        pid_t pid2 = fork();

        if (pid2 < 0) { 
            perror("fork for pid2"); 
            exit(1); 
        }

        if (pid2 == 0) {
            //process 2
            // process 2 creates pid5
            pid_t pid5 = fork();
            if (pid5 < 0) { 
                perror("fork for pid5"); 
                exit(1); 
            }

            if (pid5 == 0) {
                printf("I am process 5\nMy pid %d\nMy parent pid %d\n", getpid(), getppid());
                sleep(1);
                exit(SUCCES);
            }

            // process 2 waits for pid5
            waitpid(pid5, &status5, 0);

            if (WIFEXITED(status5) && WEXITSTATUS(status5) == SUCCES) {
                printf("I am process 2\nMy pid %d\nMy parent pid %d\n", getpid(), getppid());
                sleep(1);
                exit(SUCCES);
            }
        }

        //parent waits for pid1 and pid2
        waitpid(pid1, &status1, 0);
        waitpid(pid2, &status2, 0);

        if (WIFEXITED(status1) && WEXITSTATUS(status1) == SUCCES &&
            WIFEXITED(status2) && WEXITSTATUS(status2) == SUCCES) {
            printf("I am parent process\nMy pid %d\nMy parent pid %d\n", getpid(), getppid());
            sleep(1);
            exit(SUCCES);
        }
    }
    return 0;
}