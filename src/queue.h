#ifndef QUEUE_H_INCLUDED
#define QUEUE_H_INCLUDED

void pushQueue(Queue **, const char *, pthread_mutex_t *);
Queue *getLastQueue(Queue **);
char *popQueue(Queue **, pthread_mutex_t *);
Queue *getQueueByIndex(Queue *, unsigned short);
unsigned short countQueue(Queue *);
void freeQueue(Queue **);

#endif // QUEUE_H_INCLUDED
