//В приведенном ниже коде измените только одну строку (помеченную),
//чтобы напечатать “Результат: 12.0”.

#include <stdio.h>
int main(void)
{
    float x = 5.0;
    printf("x = %f, ", x);
    float y = 6.0;
    printf("y = %f\n", y);
    float *xp = &y; // Поменял указатель xp, чтобы он указывал на y
    float *yp = &y;
    printf("Результат: %f\n", *xp + *yp);
}