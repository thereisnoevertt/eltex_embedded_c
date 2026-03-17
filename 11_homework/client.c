#include "market.h"
#include <stdio.h>

int main() {
    pthread_t buyer_threads[BUYERS];
    pthread_t loader;

    buyer buyers[BUYERS];

    init_market();

    for (int i = 0; i < BUYERS; i++) {
        buyers[i].id = i + 1;
    }

    pthread_create(&loader, NULL, threadForLoader, NULL);

    for (int i = 0; i < BUYERS; i++) {
        pthread_create(&buyer_threads[i], NULL, threadForBuyer, &buyers[i]);
    }

    for (int i = 0; i < BUYERS; i++) {
        pthread_join(buyer_threads[i], NULL);
    }

    loader_running = 0;
    pthread_join(loader, NULL);

    destroy_market();

    printf("Market has been closed\n");
    return 0;
}