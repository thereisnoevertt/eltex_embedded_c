#ifndef MARKET_H
#define MARKET_H

#include <pthread.h>

#define STALLS 5
#define BUYERS 3

typedef struct {
    int id;
    int count;
    pthread_mutex_t mutex;
} stall;

typedef struct {
    int id;
    int need;
} buyer;

// глобальные переменные
extern stall stalls[STALLS];
extern volatile int loader_running;

// функции
void init_market();
void destroy_market();

void* threadForBuyer(void* arg);
void* threadForLoader(void* arg);

#endif