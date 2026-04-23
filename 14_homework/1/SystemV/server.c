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

    int fd = open(file, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("open");
        exit(1);
    }
    close(fd);

    key_t key = ftok(file, 65);
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    int shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    char *shm = (char *) shmat(shmid, NULL, 0);
    if (shm == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    sem_t *s2c = sem_open("/sem_s2c", O_CREAT, 0666, 0);
    sem_t *c2s = sem_open("/sem_c2s", O_CREAT, 0666, 0);

    if (s2c == SEM_FAILED || c2s == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    snprintf(shm, SHM_SIZE, "Hi!");
    printf("Server: sent Hi!\n");

    sem_post(s2c);

    sem_wait(c2s);

    printf("Server received: %s\n", shm);

    shmdt(shm);
    shmctl(shmid, IPC_RMID, NULL);

    sem_close(s2c);
    sem_close(c2s);

    sem_unlink("/sem_s2c");
    sem_unlink("/sem_c2s");

    return 0;
}