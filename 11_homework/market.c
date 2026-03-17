#include "market.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

stall stalls[STALLS];
volatile int loader_running = 1;

void enterStallForBuyer(buyer* b, int stall_id){
    stall* s = &stalls[stall_id];

    if (pthread_mutex_trylock(&s->mutex) != 0){
        return;
    }

    printf("Покупатель %d зашел в ларек %d, товаров: %d\n",
           b->id, stall_id, s->count);

    if (s->count > 0) {
        int taken = (b->need >= s->count) ? s->count : b->need;

        s->count -= taken;
        b->need -= taken;

        printf("У покупателя %d стало потребности: %d\n",
               b->id, b->need);
    }

    pthread_mutex_unlock(&s->mutex);
}

void enterStallForLoader(int stall_id) {
    stall *s = &stalls[stall_id];

    if (pthread_mutex_trylock(&s->mutex) != 0) {
        return;
    }

    s->count += 200;

    printf("Погрузчик зашел в ларек %d, теперь товаров: %d\n",
           stall_id, s->count);

    pthread_mutex_unlock(&s->mutex);
}

void* threadForBuyer(void* arg) {
    buyer* b = (buyer*)arg;

    unsigned int seed = time(NULL) ^ b->id;
    b->need = 9900 + rand_r(&seed) % 10100;

    while (b->need > 0) {
        printf("Покупатель %d проснулся, текущее кол-во потребности = %d\n",
               b->id, b->need);

        int stall_id = rand_r(&seed) % STALLS;

        enterStallForBuyer(b, stall_id);

        printf("Покупатель %d уснул на 2 секунды\n", b->id);
        sleep(2);
    }

    printf("Покупатель %d завершил покупки\n", b->id);
    return NULL;
}

void* threadForLoader(void* arg) {
    unsigned int seed = time(NULL);

    while(loader_running) {
        printf("Погрузчик проснулся\n");

        int stall_id = rand_r(&seed) % STALLS;

        enterStallForLoader(stall_id);

        printf("Погрузчик уснул на 1 секунду\n");
        sleep(1);
    }

    printf("Погрузчик завершил работу\n");
    return NULL;
}


void init_market() {
    srand(time(NULL));

    for (int i = 0; i < STALLS; i++) {
        stalls[i].id = i;
        stalls[i].count = 900 + rand() % 201;
        pthread_mutex_init(&stalls[i].mutex, NULL);
    }
}

void destroy_market() {
    for (int i = 0; i < STALLS; i++) {
        pthread_mutex_destroy(&stalls[i].mutex);
    }
}