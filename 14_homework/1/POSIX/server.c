#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#define SHM_SIZE 128
#define SHM_NAME "/shm_posix"

int main(){
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        return 1;
    }

    char* shm = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    close(fd);

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

    munmap(shm, SHM_SIZE);
    shm_unlink(SHM_NAME);

    sem_close(s2c);
    sem_close(c2s);

    sem_unlink("/sem_s2c");
    sem_unlink("/sem_c2s");
    
}