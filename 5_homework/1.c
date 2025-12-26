// Переписать абонентский справочник с использованием функций.

#include <stdio.h>
#define N 100

struct abonent
{
    char name[10];
    char second_name[10];
    char tel[10];
};

// функция добавления абонента
void addAbonent(int *count, struct abonent *phonebook)
{
    if (*count >= N - 1)
    {
        printf("Phonebook is full! Cannot add more abonents.\n");
    }
    else
    {
        printf("Enter: name surname phone\n");
        scanf("%9s %9s %9s",
              phonebook[*count].name,
              phonebook[*count].second_name,
              phonebook[*count].tel);
        (*count)++;

        printf("Abonent added. Total: %d\n", *count);
    }
}
// функция удаления абонента
void deleteAbonent(int *count, struct abonent *phonebook)
{
    if (*count == 0)
    {
        printf("Phonebook is empty");
    }
    else
    {
        char number[10];
        printf("Enter number to delete\n");
        scanf(" %9s", number);
        int index = -1;

        for (int i = 0; i < *count; i++)
        {
            int equal = 1;
            for (int j = 0; j < 10; j++)

            {
                if (phonebook[i].tel[j] != number[j])
                {
                    equal = 0;
                    break;
                }
                if (phonebook[i].tel[j] == '\0' && number[j] == '\0')
                {
                    break;
                }
            }
            if (equal)
            {
                index = i;
                break;
            }
        }

        if (index != -1)
        {
            for (int i = index; i < *count - 1; i++)
            {
                phonebook[i] = phonebook[i + 1];
            }
            (*count)--;
            printf("Abonent deleted.\n");
        }
        else
        {
            printf("No subscriber with that number.\n");
        }
    }
}

// функция поиска по имени
void findByName(int *count, struct abonent *phonebook)
{
    printf("Search abonents by name.\n");
    if (*count == 0)
    {
        printf("Phonebook is empty\n");
    }
    else
    {
        printf("Enter name to search\n");
        char name[10];
        scanf(" %9s", name);

        int found = 0;

        for (int i = 0; i < *count; i++)
        {
            int equal = 1;

            for (int j = 0; j < 10; j++)
            {
                if (phonebook[i].name[j] != name[j])
                {
                    equal = 0;
                    break;
                }
                if (phonebook[i].name[j] == '\0' && name[j] == '\0')
                {
                    break;
                }
            }

            if (equal)
            {
                printf("%d) %s %s %s\n",
                       i + 1,
                       phonebook[i].name,
                       phonebook[i].second_name,
                       phonebook[i].tel);
                found = 1;
            }
        }

        if (!found)
        {
            printf("No abonents with that name.\n");
        }
    }
}

// функция вывода всех абонентов
void showAbonents(int *count, struct abonent *phonebook)
{
    printf("All abonent (%d):\n", *count);

    if (*count == 0)
    {
        printf("Phonebook is empty\n");
    }

    else
    {
        for (int i = 0; i < *count; i++)
        {
            printf("%d) %s %s %s\n",
                   i + 1,
                   phonebook[i].name,
                   phonebook[i].second_name,
                   phonebook[i].tel);
        }
    }
}

int main()
{
    struct abonent phonebook[N];
    int count = 0;
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
            addAbonent(&count, phonebook);
            break;

        case '2':
            deleteAbonent(&count, phonebook);
            break;

        case '3':
            findByName(&count, phonebook);
            break;

        case '4':
            showAbonents(&count, phonebook);
            break;

        case '5':
            printf("Exiting program\n");
            return 0;

        default:
            printf("Invalid menu option\n");
            break;
        }
    }
}
