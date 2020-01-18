#ifndef LIST_H
#define LIST_H

#include "common.h"

/* Frees list resources */
void destroy(List *list)
{
    Bucket *bucket = list->tail;
    while (bucket != NULL)
    {
        Bucket *current = bucket;
        bucket = bucket->prev;
        free(current);
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

/* Appends a node to the list */
void append(List *list, Node *node)
{
    static int i = 0;
    Bucket *bucket = (Bucket *)malloc(sizeof(Bucket));
    bucket->node = node;
    bucket->next = NULL;
    bucket->prev = NULL;
    bucket->id = i++;
    if (list->head == NULL)
    {
        list->head = bucket;
        list->tail = bucket;
    }
    else
    {
        list->head->next = bucket;
        bucket->prev = list->head;
        list->head = bucket;
    }
    list->size++;
}

/* Returns the node indetified by 'index' and removes the bucket from the list */
Node * slice_index(List *list, int index)
{
    Bucket *bucket = list->tail;

    int i = 0;

    /* Fetch bucket */
    while (bucket != NULL)
    {
        if (index == i++)
            break;
        bucket = bucket->next;
    }

    if (list->head == NULL || bucket == NULL)
        return NULL;

    list->size--;

    /* Remove bucket */
    if (list->head == bucket)
        list->head = bucket->prev;
    if (list->tail == bucket)
        list->tail = bucket->next;
    if (bucket->next != NULL)
        bucket->next->prev = bucket->prev;
    if (bucket->prev != NULL)
        bucket->prev->next = bucket->next;

    Node *node = bucket->node;

    free(bucket);

    return node;
}

/*  Returns the node identified by 'bucket' and removes the bucket from the list */
Node * slice_bucket(List *list, Bucket *bucket)
{
    if (list->head == NULL || bucket == NULL)
        return NULL;

    list->size--;

    /* Remove bucket */
    if (list->head == bucket)
        list->head = bucket->prev;
    if (list->tail == bucket)
        list->tail = bucket->next;
    if (bucket->next != NULL)
        bucket->next->prev = bucket->prev;
    if (bucket->prev != NULL)
        bucket->prev->next = bucket->next;

    Node *node = bucket->node;

    free(bucket);

    return node;
}

int contains(List *list, Node *node)
{
    Bucket *bucket = list->tail;
    while (bucket != NULL)
    {
        if (bucket->node->i == node->i)
            return 1;
        bucket = bucket->next;
    }
    return 0;
}

void print_list(List *list)
{
    Bucket *bucket = list->tail;
    while (bucket != NULL)
    {
        printf("%d\n", bucket->id);
        bucket = bucket->next;
    }
}

void test_list(void)
{
    for (int i = 0; i < 3; i++)
        append(&open, (Node *)malloc(sizeof(Node)));

    printf("[LIST TEST: DELETE]\n");

    print_list(&open);
    printf("%p %p %d\n", open.head, open.tail, open.size); printf("---\n");

    slice_index(&open, 0);
    print_list(&open);
    printf("%p %p %d\n", open.head, open.tail, open.size); printf("---\n");

    slice_index(&open, 0);
    print_list(&open);
    printf("%p %p %d\n", open.head, open.tail, open.size); printf("---\n");

    slice_index(&open, 0);
    print_list(&open);
    printf("%p %p %d\n", open.head, open.tail, open.size); printf("---\n");

    slice_index(&open, 0);
    print_list(&open);
    printf("%p %p %d\n", open.head, open.tail, open.size); printf("---\n");

    printf("[LIST TEST: INSERT]\n");

    append(&open, (Node *)malloc(sizeof(Node)));
    print_list(&open);
    printf("%p %p %d\n", open.head, open.tail, open.size); printf("---\n");

    append(&open, (Node *)malloc(sizeof(Node)));
    print_list(&open);
    printf("%p %p %d\n", open.head, open.tail, open.size); printf("---\n");

    append(&open, (Node *)malloc(sizeof(Node)));
    print_list(&open);
    printf("%p %p %d\n", open.head, open.tail, open.size); printf("---\n");
}

#endif /* LIST_H */
