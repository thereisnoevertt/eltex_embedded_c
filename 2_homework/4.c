//Заполнить матрицу числами от 1 до N^2 улиткой

#include <stdio.h>
#define N 5

int main() {
    int matrix[N][N];
    int top = 0, bottom = N - 1;
    int left = 0, right = N - 1;
    int cur = 1;
    int total = N * N;

    while (cur <= total) {
        for (int j = left; j <= right && cur <= total; j++) {
            matrix[top][j] = cur++;
        }
        top++;

        for (int i = top; i <= bottom && cur <= total; i++) {
            matrix[i][right] = cur++;
        }
        right--;

        for (int j = right; j >= left && cur <= total; j--) {
            matrix[bottom][j] = cur++;
        }
        bottom--;

        for (int i = bottom; i >= top && cur <= total; i--) {
            matrix[i][left] = cur++;
        }
        left++;
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            printf("%3d ", matrix[i][j]);
        }
        printf("\n");
    }

    return 0;
}
