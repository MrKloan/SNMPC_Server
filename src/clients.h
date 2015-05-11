#ifndef CLIENTS_H
#define CLIENTS_H

void addClient(Clients**, Clients**, pthread_mutex_t*);
void deleteClient(Clients**, unsigned short, pthread_mutex_t*);
void deleteClientByID(Clients**, unsigned short, pthread_mutex_t*);
unsigned short getNewClientID(Clients*, pthread_mutex_t*);

Clients *getLastClient(Clients**);
Clients *retreiveClient(Clients*, pthread_mutex_t*);
Clients *getClient(Clients*, unsigned short);
Clients *getClientByID(Clients*, unsigned short, unsigned short*);
unsigned short countClients(Clients*);

void freeClient(Clients*);
void freeClients(Clients**, pthread_mutex_t*);
void printClients(Clients*);

#endif // CLIENTS_H
