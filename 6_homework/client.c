#include "server.h"

int main()
{
    int count = 0;
    node *head = NULL; // голова списка
    node *tail = NULL; // хвост списка
    char choice = 0;

    while (1)
    {
        printf("\n 1) Add abonent\n");
        printf(" 2) Delete abonent\n");
        printf(" 3) Search abonents by name\n");
        printf(" 4) Show all abonents\n");
        printf(" 5) Exit\n");
        scanf(" %c", &choice);

        switch (choice)
        {
            case '1':
                addAbonent(&count, &head, &tail);
                break;

            case '2':
                deleteAbonent(&count, &head, &tail);
                break;

            case '3':
                findByName(count, head);
                break;

            case '4':
                showAbonents(count, head);
                break;

            case '5':
                while (head)
                {
                    node *temp = head;
                    head = head->next;
                    free(temp);
                }

                printf("Exiting program\n");
                return SUCCES;

            default:
                printf("Invalid menu option\n");
                break;
        }
    }
}