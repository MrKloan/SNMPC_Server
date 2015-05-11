#ifndef SERVER_H
#define SERVER_H

void *serverHandler(void*);
void *clientListener(void*);
void *clientDispatcher(void*);
void *clientSender(void*);

PACKET_STATE handlePacket(SNMPC*, Clients*, char*);

#endif // SERVER_H
