//Вывести квадратную матрицу по заданному N

#include <stdio.h>
#define N 3

int main() {
    int matrix[N][N];
    int value = 0;

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            value += 1;
            matrix[i][j] = value;

        }
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }

    return 0;
}