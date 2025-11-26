//Вывести заданный массив размером N в обратном порядке.

#include <stdio.h>
#define N 5

int main() {
    int array[N];

    for (int i = 0; i < N; i++) {
        scanf("%d", &array[i]);
    }

    for (int i = 0; i < N/2; i++) {
        int t = array[i];
        array[i] = array[N - i - 1];
        array[N - i - 1] = t;
    }

    for (int i = 0; i < N; i++) {
        printf("%d ", array[i]);
    }

    return 0;
}
