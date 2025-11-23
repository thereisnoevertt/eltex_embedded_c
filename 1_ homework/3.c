//Найти количество единиц в двоичном представлении
//целого положительного числа (число вводится с клавиатуры).

#include <stdio.h>

int main() {
    unsigned int x;
    printf("Введите целое положительное число\n");
    scanf("%d", &x);

    if (x <= 0) {
        printf("Введённое число не удовлетворяет условию\n");
        return 1;
    }
    int count = 0;
    for (int i = sizeof(int) * 8 -1; i >= 0; i--) {
        if ((x >> i) & 1) {
            count += 1;
        }
    }
    printf("Количество единиц = %d\n", count);
    return 0;
}