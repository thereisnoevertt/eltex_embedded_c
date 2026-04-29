#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <stdlib.h>

int main() {
    pid_t receiver_pid;
    printf("Enter receiver PID: ");
    scanf("%d", &receiver_pid);

    // Send SIGUSR1 signal to the receiver process
    if (kill(receiver_pid, SIGUSR1) == 0) {
        printf("Sent SIGUSR1 to process %d\n", receiver_pid);
    }
    else {
        perror("Failed to send signal");
        exit(EXIT_FAILURE);
    }

    return 0;
}