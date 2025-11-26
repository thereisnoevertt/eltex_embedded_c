//Заполнить верхний треугольник матрицы 1, а нижний 0.

#include <stdio.h>
#define N 3

int main() {
    int matrix[N][N];

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (j < i) {
                matrix[i][j] = 0;
            }
            else {
                matrix[i][j] = 1;
            }
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

