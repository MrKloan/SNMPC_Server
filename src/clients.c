#include "includes.h"

/**
 * Ajout d'un nouvel élément à la fin de la liste chaînée d'utilisateurs.
 */
void addClient(Clients **clients, Clients **new_client, pthread_mutex_t *mutex)
{
    Clients *last = getLastClient(clients);
    Clients *temp;

    lockMutex(mutex);

    temp = *new_client;

    if(last == NULL)
        *clients = temp;
    else
        last->next = temp;

    unlockMutex(mutex);
}

/**
 * Suppression de l'élément de liste chaînée situé à l'indice donné.
 */
void deleteClient(Clients **clients, unsigned short index, pthread_mutex_t *mutex)
{
    if(index > 0)
    {
        Clients *prev = getClient(*clients, index-1);
        Clients *next = getClient(*clients, index+1);
        Clients *clt = getClient(*clients, index);

        lockMutex(mutex);

        /*clt->element.connected = 0;
        if(clt->element.visualizer != NULL)
        {
            pthread_cancel(clt->element.visualizer->handler);
            free(clt->element.visualizer);
        }
        pthread_cancel(clt->element.visualizer->handler);
        free(clt->element.visualizer);
        pthread_mutex_destroy(&clt->element.lock);
        pthread_mutex_destroy(&clt->element.sender_lock);
        pthread_cond_broadcast(&clt->element.notifier);
        pthread_cond_destroy(&clt->element.notifier);
        pthread_mutex_destroy(&clt->element.dispatch_lock);
        pthread_cond_broadcast(&clt->element.dispatch_notifier);
        pthread_cond_destroy(&clt->element.dispatch_notifier);
        closesocket(clt->element.socket);
        freeQueue(&clt->element.dispatch_queue);
        freeQueue(&clt->element.sender_queue);
        free(clt);*/
        freeClient(clt);

        prev->next = next;
    }
    //Si on veut supprimer le premier utilisateur
    else
    {
        Clients *next = (*clients)->next;

        lockMutex(mutex);

        /*(*clients)->element.connected = 0;
        if((*clients)->element.visualizer != NULL)
        {
            pthread_cancel((*clients)->element.visualizer->handler);
            free((*clients)->element.visualizer);
        }
        pthread_mutex_destroy(&(*clients)->element.lock);
        pthread_mutex_destroy(&(*clients)->element.sender_lock);
        pthread_cond_broadcast(&(*clients)->element.notifier);
        pthread_cond_destroy(&(*clients)->element.notifier);
        pthread_mutex_destroy(&(*clients)->element.dispatch_lock);
        pthread_cond_broadcast(&(*clients)->element.dispatch_notifier);
        pthread_cond_destroy(&(*clients)->element.dispatch_notifier);
        closesocket((*clients)->element.socket);
        freeQueue(&(*clients)->element.dispatch_queue);
        freeQueue(&(*clients)->element.sender_queue);
        free(*clients);*/
        freeClient(*clients);

        *clients = next;
    }

    unlockMutex(mutex);
}

/**
 * Suppression de l'élément de liste chaînée ayant l'ID donné.
 */
void deleteClientByID(Clients **clients, unsigned short id, pthread_mutex_t *mutex)
{
    unsigned short index;

    getClientByID(*clients, id, &index);
    printf("DELETING Client %hu, Index %hu\n", id, index);
    deleteClient(clients, index, mutex);
}

/**
 * Renvoie le premier ID libre pouvant être attribué à un client
 */
unsigned short getNewClientID(Clients *clients, pthread_mutex_t *mutex)
{
    unsigned short i, count = countClients(clients);

    lockMutex(mutex);
    for(i=0 ; i < count ; i++)
    {
        if(clients != NULL && clients->next != NULL && clients->next->element.id != i+1)
        {
            return i+1;
        }
        clients = clients->next;
    }
    unlockMutex(mutex);

    return i;
}

/**
 * Renvoie un pointeur sur le dernier élément alloué de la liste chaînée d'utilisateurs.
 */
Clients *getLastClient(Clients **clients)
{
    if(*clients != NULL)
    {
        if((*clients)->next != NULL)
            return getLastClient(&(*clients)->next);
        else
            return *clients;
    }
    else
        return NULL;
}

/**
 * Renvoie un pointeur sur le dernier élément alloué de la liste chaînée d'utilisateurs en verrouillant la mutex.
 */
Clients *retreiveClient(Clients *clients, pthread_mutex_t *mutex)
{
    lockMutex(mutex);

    while(clients != NULL && clients->next != NULL)
        clients = clients->next;

    unlockMutex(mutex);

    return clients;
}

/**
 * Renvoie un pointeur sur l'élément de liste chaînée situé à l'indice donné.
 */
Clients *getClient(Clients *clients, unsigned short index)
{
    unsigned short i;

    for(i=0 ; i < index ; i++)
        clients = clients->next;

    return clients;
}

/**
 * Renvoie un pointeur sur l'élément de liste chaînée identifié par l'ID donné.
 */
Clients *getClientByID(Clients *clients, unsigned short id, unsigned short *index)
{
    unsigned short i, count = countClients(clients);

    for(*index = i = 0 ; i < count ; i++)
    {
        *index = i;
        if(clients->element.id == id)
            return clients;
        clients = clients->next;
    }

    return clients;
}

/**
 * Renvoie le nombre d'éléments contenus dans la liste chaînée d'utilisateurs.
 */
unsigned short countClients(Clients *clients)
{
    unsigned short i = 0;

    while(clients != NULL)
    {
        i++;
        clients = clients->next;
    }

    return i;
}

void freeClient(Clients *client)
{
    client->element.connected = 0;

    if(client->element.visualizer != NULL)
    {
        pthread_cancel(client->element.visualizer->handler);
        free(client->element.visualizer);
    }

    pthread_mutex_destroy(&client->element.lock);
    pthread_mutex_destroy(&client->element.sender_lock);
    pthread_mutex_destroy(&client->element.dispatch_lock);

    pthread_cond_broadcast(&client->element.notifier);
    pthread_cond_destroy(&client->element.notifier);

    pthread_cond_broadcast(&client->element.dispatch_notifier);
    pthread_cond_destroy(&client->element.dispatch_notifier);

    closesocket(client->element.socket);

    freeQueue(&client->element.dispatch_queue);
    freeQueue(&client->element.sender_queue);

    free(client);
}

/**
 * Libère la mémoire allouée pour la liste chaînée d'utilisateurs.
 */
void freeClients(Clients **clients, pthread_mutex_t *mutex)
{
    lockMutex(mutex);

    while(*clients != NULL)
    {
        Clients *temp = (*clients)->next;

        /*(*clients)->element.connected = 0;
        if((*clients)->element.visualizer != NULL)
        {
            pthread_cancel((*clients)->element.visualizer->handler);
            free((*clients)->element.visualizer);
        }
        pthread_mutex_destroy(&(*clients)->element.lock);
        pthread_mutex_destroy(&(*clients)->element.sender_lock);
        pthread_mutex_destroy(&(*clients)->element.dispatch_lock);
        pthread_cond_broadcast(&(*clients)->element.notifier);
        pthread_cond_destroy(&(*clients)->element.notifier);
        pthread_cond_broadcast(&(*clients)->element.dispatch_notifier);
        pthread_cond_destroy(&(*clients)->element.dispatch_notifier);
        closesocket((*clients)->element.socket);
        freeQueue(&(*clients)->element.dispatch_queue);
        freeQueue(&(*clients)->element.sender_queue);
        free(*clients);*/
        freeClient(*clients);

        *clients = temp;
    }

    unlockMutex(mutex);
}

/**
 * Affiche le contenu de la liste chaînée d'utilisateurs sur stdout.
 */
void printClients(Clients *clients)
{
    while(clients != NULL)
    {
        printf("Client %d\n", clients->element.id);
        clients = clients->next;
    }
}
