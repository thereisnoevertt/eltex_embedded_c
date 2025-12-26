#include "server.h"

// инициализация узла списка
node *node_init(abonent *src)
{
    if (!src)
    {
        return NULL;
    }
    node *new_node = malloc(sizeof(*new_node));
    if (!new_node)
    {
        return NULL;
    }
    new_node->data = src;
    new_node->prev = NULL;
    new_node->next = NULL;
    return new_node;
}

// функция добавления абонента
int addAbonent(int *count, node **head, node **tail)
{
    abonent *a = malloc(sizeof(abonent));
    if (!a)
    {
        printf("Memory allocation failed\n");
        return ERROR;
    }

    printf("Enter: name surname phone\n");
    if (scanf("%9s %9s %9s", a->name, a->second_name, a->tel) != 3)
    {
        free(a);
        return ERROR;
    }

    node *n = node_init(a);
    if (!n)
    {
        free(a);
        return ERROR;
    }

    if (*head == NULL)
    {
        // первый элемент
        *head = n;
        *tail = n;
    }
    else
    {
        // вставка в хвост
        n->prev = *tail;
        (*tail)->next = n;
        *tail = n;
    }

    (*count)++;
    printf("Abonent added. Total: %d\n", *count);
    return SUCCES;
}

// функция удаления абонента
void deleteAbonent(int *count, node **head, node **tail)
{
    if (*count == 0)
    {
        printf("Phonebook is empty\n");
        return;
    }

    char number[10];
    printf("Enter number to delete\n");
    scanf(" %9s", number);

    node *temp = *head;
    node *to_delete = NULL;

    // поиск узла для удаления
    while (temp != NULL)
    {
        int equal = 1;
        for (int j = 0; j < 10; j++)
        {
            if (temp->data->tel[j] != number[j])
            {
                equal = 0;
                break;
            }
            if (temp->data->tel[j] == '\0' && number[j] == '\0')
            {
                break;
            }
        }
        if (equal)
        {
            to_delete = temp;
            break;
        }
        temp = temp->next;
    }

    if (to_delete == NULL)
    {
        printf("No subscriber with that number.\n");
        return;
    }

    // удаление узла
    if (to_delete->prev == NULL && to_delete->next == NULL)
    {
        // единственный узел
        *head = NULL;
        *tail = NULL;
    }
    else if (to_delete->prev == NULL)
    {
        // первый узел
        *head = to_delete->next;
        (*head)->prev = NULL;
    }
    else if (to_delete->next == NULL)
    {
        // последний узел
        *tail = to_delete->prev;
        (*tail)->next = NULL;
    }
    else
    {
        // средний узел
        to_delete->prev->next = to_delete->next;
        to_delete->next->prev = to_delete->prev;
    }
    free(to_delete->data);
    free(to_delete);
    (*count)--;
    printf("Abonent deleted. Total: %d\n", *count);
}

// функция поиска по имени
void findByName(const int count, const node *head)
{
    printf("Search abonents by name.\n");
    if (count == 0)
    {
        printf("Phonebook is empty\n");
        return;
    }

    printf("Enter name to search\n");
    int c;
    char name[10];
    while ((c = getchar()) != '\n' && c != EOF)
    {
        scanf(" %9s", name);
    }

    int found = 0;
    const node *temp = head;
    int found_num = 1;

    printf("List of abonents with that name:\n");
    while (temp != NULL)
    {
        printf(" %s \n", temp->data->name);
        int equal = 1;
        for (int j = 0; j < 10; j++)
        {
            if (temp->data->name[j] != name[j])
            {
                equal = 0;
                break;
            }
            if (temp->data->name[j] == '\0' && name[j] == '\0')
            {
                break;
            }
        }
        if (equal == 1)
        {
            printf("%d) %s %s - %s\n", found_num++,
                   temp->data->name, temp->data->second_name, temp->data->tel);
            found++;
        }
        temp = temp->next;
    }

    if (found == 0)
    {
        printf("No abonents with that name\n");
    }
}

// функция вывода всех абонентов
void showAbonents(const int count, const node *head)
{
    printf("All abonents (%d):\n", count);

    if (count == 0)
    {
        printf("Phonebook is empty\n");
    }

    else
    {
        const node *temp = head;
        int i = 0;
        while (temp != NULL)
        {
            printf("%d) %s %s %s\n",
                   i + 1,
                   temp->data->name,
                   temp->data->second_name,
                   temp->data->tel);
            temp = temp->next;
            i++;
        }
    }
}