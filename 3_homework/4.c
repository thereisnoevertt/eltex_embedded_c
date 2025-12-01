/*Напишите программу, которая ищет в веденной строке (с клавиатуры)
введенную подстроку (с клавиатуры) и возвращает указатель на начало
подстроки, если подстрока не найдена в указатель записывается NULL.
В качестве строк использовать статические массивы.*/

#include <stdio.h>
#define N 10

int main() {
    char string[N];
    char substring[N];

    int ch;
    int len = 0;

    int subch;
    int sublen = 0;

    //работа со строкой string
    printf("Введите строку не более %d символов: \n", N);
    while (len < N && (ch = getchar()) != '\n' && ch != EOF) {
        string[len] = (char)ch;
        len++;
    }
    string[len] = '\0';

    //работа со строкой substring
    printf("Введите строку не более %d символов: \n", N);
    while (sublen < N && (subch = getchar()) != '\n' && subch != EOF) {
        substring[sublen] = (char)subch;
        sublen++;
    }
    substring[sublen] = '\0';

    char *result = NULL;
    char *s = string;

    for (; *s != '\0'; s++) {
        char *p = s;
        char *q = substring;

        while (*p != '\0' && *q != '\0' && *p == *q) {
            p++;
            q++;
        }

        if (*q == '\0') {
            result = s;
            break;
        }
    }

    if (result == NULL)
        printf("Подстрока не найдена, указатель = NULL\n");
    else
        printf("Найдена! Адрес начала подстроки: %p\n", (void*)result);

}