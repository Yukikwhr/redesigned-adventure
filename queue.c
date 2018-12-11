#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <pthread.h>

struct entry
{
    struct entry *next;
    void *(*func)(void *arg);
    void *data;
};

struct list
{
    pthread_mutex_t data_lock;
    struct entry *head;
    struct entry **tail;
};

struct list *list_init(void)
{
    struct list *list;

    list = malloc(sizeof *list);
    if (list == NULL)
        return (NULL);
    list->head = NULL;
    list->tail = &list->head;
    pthread_mutex_init(&list->data_lock, NULL);
    return (list);
}

int list_enqueue(struct list *list, void *data, void *(*func)(void *arg))
{
    struct entry *e;

    e = malloc(sizeof *e);
    if (e == NULL)
        return (1);
    e->next = NULL;
    e->data = data;
    e->func = func;
    pthread_mutex_lock(&list->data_lock);
    *list->tail = e;
    list->tail = &e->next;
    pthread_mutex_unlock(&list->data_lock);
    return (0);
}

struct entry *list_dequeue(struct list *list)
{
    struct entry *e;

    pthread_mutex_lock(&list->data_lock);
    if (list->head == NULL)
    {
        pthread_mutex_unlock(&list->data_lock);
        return (NULL);
    }
    e = list->head;
    list->head = e->next;
    if (list->head == NULL)
        list->tail = &list->head;
    list->tail = &list->head;
    pthread_mutex_unlock(&list->data_lock);
    return (e);
}

void *test(void *arg)
{
    int *value = (int *)arg;
    printf("%d\n", *(value));
}

int main()
{
    struct list *list;
    struct entry *entry;

    list = list_init();

    int value = 3;
    test((void *)&value);

    list_enqueue(list, (void *)&value, test);

    /* dequeue data */
    while ((entry = list_dequeue(list)) != NULL)
    {
        entry->func(entry->data);
        free(entry);
    }

    free(list);
    return (0);
}