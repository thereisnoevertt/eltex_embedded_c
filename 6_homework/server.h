#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#define N 100

enum
{
    ERROR = 1,
    SUCCES = 0
};

typedef struct abonent
{
    char name[10];
    char second_name[10];
    char tel[10];
} abonent;

typedef struct node
{
    abonent *data;
    struct node *next;
    struct node *prev;
} node;

// инициализация узла списка
node *node_init(abonent *src);

// функция добавления абонента
int addAbonent(int *count, node **head, node **tail);

// функция удаления абонента
void deleteAbonent(int *count, node **head, node **tail);

// функция поиска по имени
void findByName(const int count, const node *head);

// функция вывода всех абонентов
void showAbonents(const int count, const node *head);

#endif