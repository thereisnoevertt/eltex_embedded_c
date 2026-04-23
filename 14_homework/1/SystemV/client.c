#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>

#define SHM_SIZE 128

int main() {
    const char *file = "/tmp/shmfile";

    key_t key = ftok(file, 65);
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    int shmid = shmget(key, SHM_SIZE, 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    char *shm = (char *) shmat(shmid, NULL, 0);
    if (shm == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    sem_t *s2c = sem_open("/sem_s2c", 0);
    sem_t *c2s = sem_open("/sem_c2s", 0);

    if (s2c == SEM_FAILED || c2s == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    sem_wait(s2c);

    printf("Client received: %s\n", shm);

    snprintf(shm, SHM_SIZE, "Hello!");

    sem_post(c2s);

    shmdt(shm);

    sem_close(s2c);
    sem_close(c2s);

    return 0;
}