#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


int main() {
    sigset_t set;
    int signal;
    sigaddset(&set, SIGUSR1);

    printf("Receiver PID: %d\nWaiting for signals...\n", getpid());

    if (sigprocmask(SIG_BLOCK, &set, 0) < 0) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }
    
    if (sigwait(&set, &signal) != 0) {
        perror("sigwait");
        exit(EXIT_FAILURE);
    }

    printf("Sigwait returned %d\n", signal);

    return 0;
}