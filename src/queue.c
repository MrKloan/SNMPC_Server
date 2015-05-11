#include "includes.h"

void pushQueue(Queue **queue, const char *value, pthread_mutex_t *mutex)
{
    Queue *last = getLastQueue(queue);
    Queue *temp;

    lockMutex(mutex);
    temp = malloc(sizeof(Queue));
    temp->value = malloc(SOCKET_BUFFER*sizeof(char));
    strcpy(temp->value, value);
    temp->next = NULL;

    if(last == NULL)
        *queue = temp;
    else
        last->next = temp;

    unlockMutex(mutex);
}

char *popQueue(Queue **queue, pthread_mutex_t *mutex)
{
    char *value = NULL;

    lockMutex(mutex);
    if(*queue != NULL)
    {
        Queue *next = (*queue)->next;
        value = malloc(SOCKET_BUFFER * sizeof(char));
        strcpy(value, (*queue)->value);

        free((*queue)->value);
        free(*queue);
        *queue = next;
    }
    unlockMutex(mutex);

    return value;
}

Queue *getLastQueue(Queue **queue)
{
    if(*queue != NULL)
    {
        if((*queue)->next != NULL)
            return getLastQueue(&(*queue)->next);
        else
            return *queue;
    }
    else
        return NULL;
}

Queue *getQueueByIndex(Queue *queue, unsigned short index)
{
    unsigned short i, count = countQueue(queue);

    for(i=0 ; i < count ; i++)
    {
        if(i == index)
            return queue;
        queue = queue->next;
    }

    return NULL;
}

unsigned short countQueue(Queue *queue)
{
    unsigned short i = 0;

    while(queue != NULL)
    {
        i++;
        queue = queue->next;
    }
    return i;
}

void freeQueue(Queue **queue)
{
    while(*queue != NULL)
    {
        Queue *temp = (*queue)->next;
        free(*queue);
        *queue = temp;
    }
}
