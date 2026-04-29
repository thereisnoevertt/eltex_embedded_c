#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


int main() {
    sigset_t old_set, new_set;
    sigaddset(&new_set, SIGINT);

    printf("Receiver PID: %d\nWaiting for signals...\n", getpid());

    if (sigprocmask(SIG_BLOCK, &new_set, &old_set) < 0) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }
        
    /*unblocking sigint*/
    if (sigprocmask(SIG_SETMASK, &old_set, NULL) < 0) {
        perror("sigprocmask 2");
        exit(EXIT_FAILURE);
    }
    
    while (1) {
        pause();
    }

    return 0;
}