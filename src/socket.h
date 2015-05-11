#ifndef SOCKET_H
#define SOCKET_H

unsigned short SocketStart(void);
unsigned short SocketEnd(void);

int SocketError(void);
char *SocketStrError(void);
unsigned short SocketClear(SOCKET);

unsigned short initSocket(SOCKET*);
unsigned short socketBind(Server*, unsigned short);
Clients *socketListen(Server*);
unsigned short socketSend(Client*);
unsigned short socketReceive(Client*);
void socketFree(SOCKET);

#endif // SOCKET_H
