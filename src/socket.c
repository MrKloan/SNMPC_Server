#include "includes.h"

unsigned short SocketStart(void)
{
#ifdef WIN32
    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2), &wsa) == 0)
        return 1;
    else
        return 0;
#elif defined (linux)
    return 1;
#endif
}

unsigned short SocketEnd(void)
{
#ifdef WIN32
    if(WSACleanup() == 0)
        return 1;
    else
        return 0;
#elif defined (linux)
    return 1;
#endif
}

int SocketError(void)
{
#ifdef WIN32
    return WSAGetLastError();
#elif defined (linux)
    return errno;
#endif
}

char *SocketStrError(void)
{
#ifdef WIN32
    return FormatMessage(WSAGetLastError());
#elif defined (linux)
    return strerror(errno);
#endif
}

unsigned short SocketClear(SOCKET socket)
{
    int value = 1; //true

    if(setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) == SOCKET_ERROR)
        return 0;
    else
        return 1;

}

unsigned short initSocket(SOCKET *sock)
{
    if((*sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
        return 0;
    else
        return 1;
}

unsigned short socketBind(Server *server, unsigned short port)
{
    server->infos.sin_addr.s_addr = htonl(INADDR_ANY);
    server->infos.sin_family = AF_INET;
    server->infos.sin_port = htons(port);

    if(SocketClear(server->socket) && bind(server->socket, (SOCKADDR *)&server->infos, sizeof(server->infos)) != SOCKET_ERROR)
        return 1;
    else
        return 0;
}

Clients *socketListen(Server *server)
{
    if(listen(server->socket, SERVER_WAIT_QUEUE) == SOCKET_ERROR)
        return NULL;
    else
    {
        int info_size;
        Clients *client = malloc(sizeof(Clients));

        client->element.id = getNewClientID(server->clients, &server->lock);
        client->element.socket = INVALID_SOCKET;
        client->next = NULL;

        info_size = sizeof(client->element.infos);

        if((client->element.socket = accept(server->socket, (SOCKADDR *)&client->element.infos, (socklen_t *)&info_size)) != INVALID_SOCKET)
        {
            client->element.connected = 1;
            return client;
        }

        free(client);
        return NULL;
    }
}

unsigned short socketSend(Client *client)
{
    if(send(client->socket, client->buffer, strlen(client->buffer), 0) != SOCKET_ERROR/*< 0*/)
        return 1;
    else
        return 0;
}

unsigned short socketReceive(Client *client)
{
    int n;

    if((n = recv(client->socket, client->buffer, SOCKET_BUFFER-1, 0)) <= 0)
        return 0;
    else
    {
        client->buffer[n] = '\0';
        return 1;
    }

}

void socketFree(SOCKET socket)
{
    closesocket(socket);
}
