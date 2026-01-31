#include <stdio.h>
#include "calc.h"

int main()
{
    int choice, a, b;

    while (1)
    {
        printf(
            "\n1) Addition\n"
            "2) Subtraction\n"
            "3) Multiplication\n"
            "4) Division\n"
            "5) Exit\n");

        printf("Choose an option: ");
        scanf("%d", &choice);

        if (choice == 5)
            break;

        printf("Enter two integers: ");
        scanf("%d %d", &a, &b);

        switch (choice)
        {
        case 1:
            printf("Result: %d\n", add(a, b));
            break;
        case 2:
            printf("Result: %d\n", sub(a, b));
            break;
        case 3:
            printf("Result: %d\n", mul(a, b));
            break;
        case 4:
            if (b == 0)
                printf("Error: division by zero\n");
            else
                printf("Result: %d\n", divide(a, b));
            break;
        default:
            printf("Invalid menu option\n");
        }
    }

    return 0;
}