#ifndef STRUCTURES_H
#define STRUCTURES_H

#if defined (linux)
    typedef int SOCKET;
    typedef struct sockaddr_in SOCKADDR_IN;
    typedef struct sockaddr SOCKADDR;
    typedef struct in_addr IN_ADDR;
    typedef struct hostent HOSTENT;
#endif

typedef struct SHA1Context {
    unsigned Message_Digest[5];
    char sha_message[41];

    unsigned Length_Low;
    unsigned Length_High;

    unsigned char Message_Block[64];
    int Message_Block_Index;

    int Computed;
    int Corrupted;
} SHA1Context;

typedef struct Matrix {
    unsigned short matrix[4][8];
    unsigned short columns;
    unsigned short lines;
} Matrix;

typedef struct Shell {
    pthread_t handler;
    char hostname[26];
    char cmd[SHELL_BUFFER];
    unsigned short identified;
} Shell;

typedef struct User {
    char username[41], password[41];
} User;

typedef struct Users {
    User element;
    struct Users *next;
} Users;

typedef struct Task {
    char name[25];
    unsigned short enabled;
    unsigned short repeat;
    char date[11]; //JJ/MM/AAAA
    unsigned short weeks;
    unsigned short days[7];
    unsigned short type;
    unsigned short hours[2];
    unsigned short minutes[2];

    unsigned short processing;
} Task;

typedef struct Tasks {
    Task            element;
    struct Tasks    *next;
} Tasks;

typedef struct Device
{
    char name[20];
    char version[10];
    char date[10];

    char communities[2][14];

    char ip[16];
    unsigned short dhcp_config;
    char subnet_mask[16];
    char gateway[16];
    char dns[16];
    char mac_addr[18];

    char relays_name[8][12];
    unsigned short relays_state[8];
    unsigned short pulse_state[8];
    unsigned short pulse_duration[8];
    unsigned short digital_input;

    unsigned short restart;
    unsigned short disconnected;
    Tasks *tasks[8];
} Device;

typedef struct Devices
{
    Device element;
    struct Devices *next;
} Devices;

typedef struct SNMPRequest
{
    netsnmp_session session;
    netsnmp_session *ss;
    netsnmp_pdu *pdu;
    netsnmp_pdu *response;

    oid anOID[MAX_OID_LEN];
    size_t anOID_len;
    COMMUNITIES cm;

    netsnmp_variable_list *vars;
    struct tree *mib_tree;
    int status;

    unsigned short *device_disconnected;
} SNMPRequest;

typedef struct XmlData {
    char *path;
    xmlDocPtr doc;
    xmlNodePtr root;
} XmlData;

typedef struct Queue {
    char *value;
    struct Queue *next;
} Queue;

typedef struct Vizualizer {
    unsigned short visualizing;
    Devices *device;
    struct Client  *client;
    pthread_t handler;
    unsigned short *visualization_rate;
} Visualizer;

typedef struct Client {
    unsigned short id, connected;
    SOCKET socket;
    SOCKADDR_IN infos;

    pthread_t sender, listener;
    pthread_t dispatcher;
    pthread_mutex_t lock;
    pthread_mutex_t sender_lock;
    pthread_mutex_t dispatch_lock;
    pthread_cond_t notifier;
    pthread_cond_t dispatch_notifier;

    Queue *sender_queue;
    Queue *dispatch_queue;

    Visualizer *visualizer;

    char buffer[SOCKET_BUFFER];
} Client;

typedef struct Clients {
    Client element;
    struct Clients *next;
} Clients;

typedef struct Server {
    SOCKET socket;
    unsigned short port, running;
    SOCKADDR_IN infos;

    pthread_t handler;
    pthread_mutex_t lock;

    Clients *clients;
} Server;

typedef struct Scheduler {
    unsigned short running;
    pthread_t handler;
    pthread_mutex_t lock;
} Scheduler;

typedef struct TaskHandler {
    Server *server;
    Device *device;
    Task *task;

    pthread_t handler;
    unsigned short relay;
} TaskHandler;

typedef struct SNMPC {
    unsigned short running;
    pthread_mutex_t locks[3];

    Matrix matrix;
    Server server;
    Shell shell;
    Scheduler scheduler;

    XmlData data[3];
    Users *users;
    Devices *devices;
    unsigned short visualization_rate;
} SNMPC;

#endif // STRUCTURES_H
