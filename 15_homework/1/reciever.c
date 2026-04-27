#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>


void sig_handler(int sig) {
    printf("Received signal SIGUSR1 %d\n", sig);
}

int main() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGUSR1, &sa, NULL);
    
    printf("Receiver PID: %d\nWaiting for signals...\n", getpid());


    while (1) {
        pause();
    }

    return 0;
}