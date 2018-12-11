#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <pthread.h>

#define PUT_NUM 30 /* 追加するエントリ数 */
#define GET_NUM 10 /* 取り出すエントリ数 */

struct entry
{
    struct entry *next;
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

int list_enqueue(struct list *list, void *data)
{
    struct entry *e;

    e = malloc(sizeof *e);
    if (e == NULL)
        return (1);
    e->next = NULL;
    e->data = data;
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

struct entry *list_traverse(struct list *list, int (*func)(void *, void *), void *user)
{
    struct entry **prev, *n, *next;

    if (list == NULL)
        return (NULL);

    pthread_mutex_lock(&list->data_lock);
    prev = &list->head;
    for (n = list->head; n != NULL; n = next)
    {
        next = n->next;
        switch (func(n->data, user))
        {
        case 0:
            /* continues */
            prev = &n->next;
            break;
        case 1:
            /* delete the entry */
            *prev = next;
            if (next == NULL)
                list->tail = prev;
            pthread_mutex_unlock(&list->data_lock);
            return (n);
        case -1:
        default:
            /* traversal stops */
            pthread_mutex_unlock(&list->data_lock);
            return (NULL);
        }
    }
    pthread_mutex_unlock(&list->data_lock);
    return (NULL);
}

int print_entry(void *e, void *u)
{
    printf("%s\n", (char *)e);
    return (0);
}

int delete_entry(void *e, void *u)
{
    char *c1 = e, *c2 = u;

    return (!strcmp(c1, c2));
}

void *put(void *arg)
{
    struct list *list = (struct list *)arg;

    /* 追加する */
    for (int i = 0; i < PUT_NUM; i++)
    {
        list_enqueue(list, strdup("data"));
    }
}

void *get(void *arg)
{
    struct list *list = (struct list *)arg;
    struct entry *entry;
    /* 取り出す */
    for (int i = 0; i < GET_NUM; i++)
    {
        entry = list_dequeue(list);
        printf("%s\n", (char *)entry->data);
        free(entry->data);
        free(entry);
    }
}

int main()
{
    struct list *list;
    struct entry *entry;

    list = list_init();

    /* enqueue data */
    list_enqueue(list, strdup("first"));
    list_enqueue(list, strdup("second"));
    list_enqueue(list, strdup("third"));

    /* entry list */
    list_traverse(list, print_entry, NULL);

    /* delete "second" entry */
    entry = list_traverse(list, delete_entry, "second");
    if (entry != NULL)
    {
        free(entry->data);
        free(entry);
    }

    /* dequeue data */
    while ((entry = list_dequeue(list)) != NULL)
    {
        printf("%s\n", (char *)entry->data);
        free(entry->data);
        free(entry);
    }

    pthread_t putter[2], getter[6];

    for (int i = 0; i < 2; i++)
    {
        pthread_create(&putter[i], NULL, put, (void *)list);
    }

    for (int i = 0; i < 6; i++)
    {
        pthread_create(&getter[i], NULL, get, (void *)list);
    }

    for (int i = 0; i < 2; i++)
    {
        pthread_join(putter[i], NULL);
    }

    for (int i = 0; i < 6; i++)
    {
        pthread_join(getter[i], NULL);
    }

    free(list);
    return (0);
}