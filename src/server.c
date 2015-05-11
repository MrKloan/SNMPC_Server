#include "includes.h"

/**
 * Thread d'écoute et d'acceptation des connexions client.
 */
void *serverHandler(void *data)
{
    SNMPC *snmpc = (SNMPC*)data;

    printf("\n[+] Server listening on port %hu...\n", snmpc->server.port);
    while(snmpc->running && snmpc->server.running)
    {
        Clients *temp;
        if((temp = socketListen(&snmpc->server)) != NULL)
        {
            addClient(&snmpc->server.clients, &temp, &snmpc->server.lock);

            pthread_mutex_init(&temp->element.lock, NULL);
            pthread_mutex_init(&temp->element.sender_lock, NULL);
            pthread_mutex_init(&temp->element.dispatch_lock, NULL);
            pthread_cond_init(&temp->element.notifier, NULL);
            pthread_cond_init(&temp->element.dispatch_notifier, NULL);

            temp->element.dispatch_queue = NULL;
            temp->element.sender_queue = NULL;
            temp->element.visualizer = NULL;

            pthread_create(&temp->element.listener, NULL, clientListener, snmpc);
            pthread_create(&temp->element.sender, NULL, clientSender, snmpc);
            pthread_create(&temp->element.dispatcher, NULL, clientDispatcher, snmpc);
        }
        else
            printf("\n[-] Error %d: %s", SocketError(), SocketStrError());
    }

    return NULL;
}

void *clientListener(void *data)
{
    SNMPC *snmpc = (SNMPC*)data;
    Clients *client = retreiveClient(snmpc->server.clients, &snmpc->server.lock);

    if(client != NULL)
    {
        printf("[+] New listener %hu\n", client->element.id);
        while(snmpc->running && client->element.connected)
        {
            if(socketReceive(&client->element))
            {
                printf("RECEIVED --> %s\n", client->element.buffer);
                pushQueue(&client->element.dispatch_queue, client->element.buffer, &client->element.dispatch_lock);
                unlockCond(&client->element.dispatch_notifier);
            }
            else
            {
                printf("\n[-] Client %hu was disconnected.\n", client->element.id);
                deleteClientByID(&snmpc->server.clients, client->element.id, &snmpc->server.lock);
                break;
            }
        }
    }

    return NULL;
}

void *clientDispatcher(void *data)
{
    SNMPC *snmpc = (SNMPC*)data;
    Clients *client = retreiveClient(snmpc->server.clients, &snmpc->server.lock);

    if(client != NULL)
    {
        printf("[+] New dispatcher %hu\n", client->element.id);
        while(snmpc->running && client->element.connected)
        {
            char *packet;

            lockMutex(&client->element.lock);
            if(countQueue(client->element.dispatch_queue) == 0)
                lockCond(&client->element.dispatch_notifier, &client->element.lock);

            if(!client->element.connected)
                break;

            packet = popQueue(&client->element.dispatch_queue, &client->element.dispatch_lock);

            if(packet != NULL)
            {
                char **parts;
                unsigned short sizeParts, i;

                parts = explode(packet, PACKET_END, &sizeParts);

                if(parts != NULL && sizeParts > 0)
                {
                    for(i=0 ; i < sizeParts ; i++)
                    {
                        uncipherPacket(snmpc, &parts[i]);

                        if(parts[i] != NULL)
                        {
                            char **packets;
                            unsigned short size, j;

                            printf("PACKET --> %s\n", parts[i]);
                            packets = explode(parts[i], PACKET_END, &size);

                            for(j = 0; j < size; j++)
                            {
                                if(handlePacket(snmpc, client, packets[j]) == PACKET_FAIL)
                                {
                                    unlockMutex(&client->element.lock);
                                    usleep(250);
                                    lockMutex(&client->element.lock);

                                    /*printf("\n[-] Client %hu was disconnected.\n", client->element.id);
                                    deleteClientByID(&snmpc->server.clients, client->element.id, &snmpc->server.lock);*/
                                    socketFree(client->element.socket); //On ne close que la socket de manière à provoquer une erreur dans le listener et free
                                    break;
                                }
                                free(packets[j]);
                            }
                            free(packets);
                            free(parts[i]);
                        }
                    }
                    free(parts);
                }
                free(packet);
            }
            else
                printf("[-] NULL packet aborted\n");
            unlockMutex(&client->element.lock);
        }
    }

    return NULL;
}

void *clientSender(void *data)
{
    SNMPC *snmpc = (SNMPC*)data;
    Clients *client = retreiveClient(snmpc->server.clients, &snmpc->server.lock);

    if(client != NULL)
    {
        printf("[+] New sender %hu\n", client->element.id);
        while(snmpc->running && client->element.connected)
        {
            char *packet;

            lockMutex(&client->element.lock);

            if(countQueue(client->element.sender_queue) == 0)
                lockCond(&client->element.notifier, &client->element.lock);

            if(!client->element.connected)
                break;

            packet = popQueue(&client->element.sender_queue, &client->element.sender_lock);

            if(packet != NULL)
            {
                printf("SENT --> %s\n", packet);

                cipherPacket(snmpc, &packet);
                strcpy(client->element.buffer, packet);
                if(socketSend(&client->element))
                {
                    printf("CIPHERED --> %s\n", packet);
                }
                else
                {
                    printf("\n[-] Error with socket connection to client %hu. Disconnecting.\n", client->element.id);
                    unlockMutex(&client->element.lock);
                    deleteClientByID(&snmpc->server.clients, client->element.id, &snmpc->server.lock);
                    break;
                }

                free(packet);
            }
            unlockMutex(&client->element.lock);
        }
    }

    return NULL;
}

PACKET_STATE handlePacket(SNMPC *snmpc, Clients *client, char *packet)
{
    Devices *devices;
    unsigned short i, size;

    PACKET_STATE value = PACKET_OK;

    char *commands[12] = {
        "loginServer",
        "addDevice",
        "delDevice",
        "delAccount",
        "visualization",
        "visualizationRate",
        "changeProfile",
        "changeConfig",
        "addTask",
        "deleteTask",
        "error",
        "modifyTask"
    };

    char **args = explode(packet, PACKET_DELIMITER, &size);

    //Connexion d'un client
    if(!strcmp(args[0], commands[0]))
    {
        Users *user = getUserByName(snmpc->users, args[1]);

        if(user != NULL)
        {
            if(!strcmp(args[2], user->element.password))
            {
                char *buffer = formatPacket(2, commands[0], "valid");

                pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);

                unlockCond(&client->element.notifier);
                unlockMutex(&client->element.lock);
                usleep(250);
                lockMutex(&client->element.lock);

                sendDevicesData(snmpc, &client->element);

                free(buffer);
            }
            else
            {
                char *buffer = formatPacket(2, commands[0], "failed");

                printf("\n[-] Client %hu tried to login with invalid informations.\n", client->element.id);
                pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                unlockCond(&client->element.notifier);

                free(buffer);
                value = PACKET_FAIL;
            }
        }
        else
        {
            char *buffer = formatPacket(2, commands[0], "failed");

            printf("\n[-] Client %hu tried to login with invalid informations.\n", client->element.id);
            pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
            unlockCond(&client->element.notifier);

            free(buffer);
            value = PACKET_FAIL;
        }

    }
    //add device
    else if(!strcmp(args[0], commands[1]))
    {
        char *buffer;

        if(size == 7)
        {
            Users *user = getUserByName(snmpc->users, args[5]);

            if(user != NULL)
            {
                if(!strcmp(args[6], user->element.password))
                {
                    if(!deviceNameExists(snmpc->devices, args[1]))
                    {
                        if(!deviceIpExists(snmpc->devices, args[2]))
                        {
                            if(addDeviceToXml(&snmpc->data[DATA_DEVICES], args[1], args[2], args[3], args[4], &snmpc->locks[LOCK_DEVICES]))
                            {
                                Clients *temp;

                                addDevice(&snmpc->devices, args[1], args[2],  args[3], args[4], &snmpc->locks[LOCK_DEVICES]);
                                printf("Device %s [%s] successfully created.\n", args[1], args[2]);

                                devices = getLastDevice(&snmpc->devices);

                                temp = snmpc->server.clients;
                                while(temp != NULL)
                                {
                                    buffer = formatPacket(5, "addDevice",
                                                         devices->element.name,
                                                         devices->element.ip,
                                                         devices->element.communities[0],
                                                         devices->element.communities[1]);

                                    pushQueue(&temp->element.sender_queue, buffer, &temp->element.sender_lock);
                                    free(buffer);

                                    buffer = getDeviceData(&devices->element);

                                    pushQueue(&temp->element.sender_queue, buffer, &temp->element.sender_lock);
                                    free(buffer);

                                    unlockCond(&temp->element.notifier);

                                    temp = temp->next;
                                }
                            }
                            else
                            {
                                fprintf(stderr, "An error occured while creating device %s [%s].\n", args[1], args[2]);

                                buffer = formatPacket(2, commands[10], "An error occured while creating device.");
                                pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                                unlockCond(&client->element.notifier);

                                free(buffer);
                            }
                        }
                        else
                        {
                            fprintf(stderr, "IP address %s already linked to a device.", args[2]);

                            buffer = formatPacket(2, commands[10], "IP address already linked to a device.");
                            pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                            unlockCond(&client->element.notifier);

                            free(buffer);
                        }
                    }
                }
                else
                {
                    fprintf(stderr, "Invalid user information. Aborting request : %s\n", packet);

                    buffer = formatPacket(2, commands[10], "Invalid user information. Aborting request.");
                    pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                    unlockCond(&client->element.notifier);

                    free(buffer);
                }
            }
            else
            {
                fprintf(stderr, "Invalid user information. Aborting request : %s\n", packet);

                buffer = formatPacket(2, commands[10], "Invalid user information. Aborting request.");
                pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                unlockCond(&client->element.notifier);

                free(buffer);
            }
        }
        else
        {
            fprintf(stderr, "Invalid packet size: %s\n", packet);

            buffer = formatPacket(2, commands[10], "Invalid addDevice order received. Your request has been aborted.");
            pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
            unlockCond(&client->element.notifier);

            free(buffer);
        }

    }
    //del device
    else if(!strcmp(args[0], commands[2]))
    {
        char *buffer;

        if(size == 4)
        {
            Users *user = getUserByName(snmpc->users, args[2]);

            if(user != NULL)
            {
                if(!strcmp(args[3], user->element.password))
                {
                    short index;

                    if((index = deleteDeviceFromXml(&snmpc->data[DATA_DEVICES], args[1], &snmpc->locks[LOCK_DEVICES])) > -1)
                    {
                        Clients *temp = snmpc->server.clients;

                        deleteDevice(&snmpc->devices, (unsigned short)index, &snmpc->locks[LOCK_DEVICES]);
                        printf("Device %s successfully deleted\n", args[1]);

                        buffer = formatPacket(2, commands[2], args[1]);

                        if(snmpc->devices == NULL)
                        {
                            char *buf = formatPacket(1, "noDevice");
                            buffer = realloc(buffer, (strlen(buffer) + strlen(buf) + 1) * sizeof(char));
                            strcat(buffer, buf);
                            free(buf);
                        }

                        while(temp != NULL)
                        {
                            pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                            unlockCond(&client->element.notifier);

                            temp = temp->next;
                        }

                        free(buffer);
                    }
                    else
                    {
                        fprintf(stderr, "An error occured while deleting device %s.\n", args[1]);

                        buffer = formatPacket(2, commands[10], "An error occured while deleting device.");
                        pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                        unlockCond(&client->element.notifier);

                        free(buffer);
                    }
                }
            }
            else
            {
                fprintf(stderr, "Invalid user information. Aborting request : %s\n", packet);

                buffer = formatPacket(2, commands[10], "Invalid user information. Aborting request.");
                pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                unlockCond(&client->element.notifier);

                free(buffer);
            }
        }
        else
        {
            fprintf(stderr, "Invalid packet size: %s\n", packet);

            buffer = formatPacket(2, commands[10], "Invalid delDevice order received. Your request has been aborted.");
            pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
            unlockCond(&client->element.notifier);

            free(buffer);
        }
    }
    //delAccount
    else if(!strcmp(args[0], commands[3]))
    {
        char *buffer;

        if(size == 3)
        {
            Users *user = getUserByName(snmpc->users, args[1]);

            if(user != NULL && !strcmp(args[2], user->element.password))
            {
                short index;

                if((index = deleteUserFromXml(&snmpc->data[DATA_USERS], args[1], args[2], &snmpc->locks[LOCK_USERS])) > -1)
                {
                    deleteUser(&snmpc->users, (unsigned short)index, &snmpc->locks[LOCK_USERS]);
                    printf("User %s successfully deleted\n", args[1]);

                    buffer = formatPacket(2, "closeAccount", args[1]);

                    pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                    unlockCond(&client->element.notifier);

                    free(buffer);

                    //deleteClientByID(&snmpc->server.clients, client->element.id, &snmpc->server.lock);

                    if(snmpc->users == NULL)
                        snmpc->shell.identified = 1;
                }
                else
                {
                    fprintf(stderr, "Unable to delete user %s from XML.\n", args[1]);

                    buffer = formatPacket(2, commands[10], "An error occured while deleting the user.");
                    pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                    unlockCond(&client->element.notifier);

                    free(buffer);
                }
            }
            else
            {
                fprintf(stderr, "Invalid user information. Aborting request : %s.\n", packet);

                buffer = formatPacket(2, commands[10], "Invalid user information. Aborting request.");
                pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                unlockCond(&client->element.notifier);

                free(buffer);
            }
        }
        else
        {
            fprintf(stderr, "Invalid packet size: %s", packet);

            buffer = formatPacket(2, commands[10], "Invalid closeAccount order received. Your request has been aborted.");
            pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
            unlockCond(&client->element.notifier);

            free(buffer);
        }
    }
    //visualization
    else if(!strcmp(args[0], commands[4]))
    {
        if(size == 2)
        {
            if(strcmp(args[1], "start") == 0)
            {
                Visualizer *visualizer;

                visualizer = malloc(sizeof(Visualizer));

                visualizer->visualizing = 1;
                visualizer->client = &client->element;
                visualizer->device = snmpc->devices;
                visualizer->visualization_rate = &snmpc->visualization_rate;
                client->element.visualizer = visualizer;

                pthread_create(&visualizer->handler, NULL, visualizerHandler, visualizer);
            }
            else if(strcmp(args[1], "stop") == 0)
            {
                if(client->element.visualizer != NULL)
                    client->element.visualizer->visualizing = 0;
            }
            else if(strcmp(args[1], "crash") == 0)
            {
                if(client->element.visualizer != NULL)
                {
                    client->element.visualizer->visualizing = 0;
                    pthread_cancel(client->element.visualizer->handler);
                    free(client->element.visualizer);
                    client->element.visualizer = NULL;
                }
            }
        }
        else
        {
            char *buffer = formatPacket(2, commands[10], "Invalid Visualization order received. Your request has been aborted.");

            fprintf(stderr, "Invalid Visualization packet received.\n");

            pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
            unlockCond(&client->element.notifier);

            free(buffer);
        }
    }
    //changeProfile
    else if(!strcmp(args[0], commands[6]))
    {
        char *buffer;

        if(size == 4)
        {
            Users *user = getUserByName(snmpc->users, args[1]);

            if(user != NULL && !strcmp(args[2], user->element.password))
            {
                if(changeUserPassFromXml(&snmpc->data[DATA_USERS], args[1], args[2], args[3], &snmpc->locks[LOCK_USERS]))
                {
                    strcpy(user->element.password, args[3]);
                }
                else
                {
                    fprintf(stderr, "An error occured while updating password of user %s\n", args[1]);

                    buffer = formatPacket(2, commands[10], "An error occured while updating your informations. Changes will not be applied.");
                    pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                    unlockCond(&client->element.notifier);

                    free(buffer);
                }
            }
            else
            {
                fprintf(stderr, "Invalid user information. Aborting request : %s\n", packet);

                buffer = formatPacket(2, commands[10], "Invalid user information. Aborting request.");
                pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                unlockCond(&client->element.notifier);

                free(buffer);
            }
        }
        else
        {
            fprintf(stderr, "Invalid packet size: %s\n", packet);

            buffer = formatPacket(2, commands[10], "Invalid changeProfile order received. Your request has been aborted.");
            pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
            unlockCond(&client->element.notifier);

            free(buffer);
        }
    }
    //changeConfig
    else if(!strcmp(args[0], commands[7]))
    {
        char *buffer;

        if(size == 6)
        {
            Devices *device = getDeviceByName(snmpc->devices, args[1]);

            if(device != NULL)
            {
                /*if(!deviceNameExists(snmpc->devices, args[2]))
                {
                    if(!deviceIpExists(snmpc->devices, args[3]))
                    {*/
                        if(changeDeviceConfigFromXml(&snmpc->data[DATA_DEVICES], args[1], args[2], args[3], args[4], args[5], &snmpc->locks[LOCK_DEVICES]))
                        {
                            strcpy(device->element.name, args[2]);
                            strcpy(device->element.ip, args[3]);
                            strcpy(device->element.communities[0], args[4]);
                            strcpy(device->element.communities[1], args[5]);

                            //delDevice
                            buffer = formatPacket(2, commands[2], args[1]);

                            pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                            //unlockCond(&client->element.notifier);

                            free(buffer);

                            //addDevice
                            buffer = formatPacket(5, commands[1],
                                     device->element.name,
                                     device->element.ip,
                                     device->element.communities[0],
                                     device->element.communities[1]);

                            pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                            free(buffer);

                            //getDeviceData
                            buffer = getDeviceData(&device->element);

                            pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                            free(buffer);

                            unlockCond(&client->element.notifier);
                        }
                        else
                        {
                            fprintf(stderr, "An error occured while updating configuration of %s device\n", args[1]);

                            buffer = formatPacket(2, commands[10], "An error occured while updating device configuration. Changes will not be applied.");
                            pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                            unlockCond(&client->element.notifier);

                            free(buffer);
                        }
                    /*}
                }*/
            }
            else
            {
                fprintf(stderr, "Unknown device %s.\n", args[1]);

                buffer = formatPacket(2, commands[10], "Unknown device.");
                pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                unlockCond(&client->element.notifier);

                free(buffer);
            }
        }
        else
        {
            fprintf(stderr, "Invalid packet size: %s\n", packet);

            buffer = formatPacket(2, commands[10], "Invalid changeConfig order received. Your request has been aborted.");
            pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
            unlockCond(&client->element.notifier);

            free(buffer);
        }
    }
    //addTask
    else if(!strcmp(args[0], commands[8]))
    {
        char *buffer;

        if(size == 12)
        {
            if((devices = getDeviceByName(snmpc->devices, args[1])) != NULL)
            {
                unsigned short index = atoi(args[2]);

                if(index > 0 && index < 9)
                {
                    Tasks *task;

                    if((task = getTaskByName(devices->element.tasks[index-1], args[3])) == NULL)
                    {
                        Clients *clients = snmpc->server.clients;

                        addTask(&devices->element.tasks[index-1], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], &snmpc->locks[LOCK_TASKS]);
                        addTaskToXml(&snmpc->data[DATA_DEVICES], &getLastTask(&devices->element.tasks[index-1])->element, &devices->element, index, &snmpc->locks[LOCK_TASKS]);

                        buffer = formatPacket(12,
                                            "addTask",
                                            args[1],
                                            args[2],
                                            args[3],
                                            args[4],
                                            args[5],
                                            args[6],
                                            args[7],
                                            args[8],
                                            args[9],
                                            args[10],
                                            args[11]);

                        while(clients != NULL)
                        {
                            pushQueue(&clients->element.sender_queue, buffer, &clients->element.sender_lock);
                            unlockCond(&clients->element.notifier);

                            clients = clients->next;
                        }

                        free(buffer);
                    }
                    else
                    {
                        fprintf(stderr, "New task name already exists: %s\n", args[3]);

                        buffer = formatPacket(2, commands[10], "New task name already exists.");
                        pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                        unlockCond(&client->element.notifier);

                        free(buffer);
                    }
                }
                else
                {
                    fprintf(stderr, "Invalid relay number specified: %hu\n", index);

                    buffer = formatPacket(2, commands[10], "Invalid relay number specified.");
                    pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                    unlockCond(&client->element.notifier);

                    free(buffer);
                }
            }
            else
            {
                fprintf(stderr, "Unknown device specified. Aborting request : %s\n", packet);

                buffer = formatPacket(2, commands[10], "Unknown device specified. Aborting request.");
                pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                unlockCond(&client->element.notifier);

                free(buffer);
            }
        }
        else
        {
            fprintf(stderr, "Invalid packet size: %s\n", packet);

            buffer = formatPacket(2, commands[10], "Invalid addTask order received. Your request has been aborted.");
            pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
            unlockCond(&client->element.notifier);

            free(buffer);
        }
    }
    //deleteTask
    else if(!strcmp(args[0], commands[9]))
    {
        char *buffer;

        if(size == 6)
        {
            Users *user = getUserByName(snmpc->users, args[4]);

            if(user != NULL && !strcmp(args[5], user->element.password))
            {
                if((devices = getDeviceByName(snmpc->devices, args[1])) != NULL)
                {
                    Tasks *task;
                    unsigned short index = atoi(args[2]);

                    if(index > 0 && index < 9)
                    {
                        if((task = getTaskByName(devices->element.tasks[index-1], args[3])) != NULL)
                        {
                            Clients *clients = snmpc->server.clients;

                            if(deleteTaskFromXml(&snmpc->data[DATA_DEVICES], &task->element, &devices->element, index, &snmpc->locks[LOCK_TASKS]))
                            {
                                deleteTaskByName(&devices->element.tasks[index-1], args[3], &snmpc->locks[LOCK_TASKS]);

                                buffer = formatPacket(4, commands[9], args[1], args[2], args[3]);

                                while(clients != NULL)
                                {
                                    pushQueue(&clients->element.sender_queue, buffer, &clients->element.sender_lock);
                                    unlockCond(&clients->element.notifier);

                                    clients = clients->next;
                                }

                                free(buffer);
                            }
                            else
                            {
                                fprintf(stderr, "An error occured while deleting task from XML data file : %s\n", packet);

                                buffer = formatPacket(2, commands[10], "An error occured while deleting task.");
                                pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                                unlockCond(&client->element.notifier);

                                free(buffer);
                            }
                        }
                    }
                    else
                    {
                        fprintf(stderr, "Invalid relay number specified : %hu\n", index);

                        buffer = formatPacket(2, commands[10], "Invalid relay number specified.");
                        pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                        unlockCond(&client->element.notifier);

                        free(buffer);
                    }
                }
                else
                {
                    fprintf(stderr, "Unknown device specified. Aborting request : %s\n", packet);

                    buffer = formatPacket(2, commands[10], "Unknown device specified. Aborting request.");
                    pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                    unlockCond(&client->element.notifier);

                    free(buffer);
                }
            }
            else
            {
                fprintf(stderr, "Invalid user information. Aborting request : %s\n", packet);

                buffer = formatPacket(2, commands[10], "Invalid user information. Aborting request.");
                pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                unlockCond(&client->element.notifier);

                free(buffer);
            }
        }
        else
        {
            fprintf(stderr, "Invalid packet size: %s\n", packet);

            buffer = formatPacket(2, commands[10], "Invalid deleteTask order received. Your request has been aborted.");
            pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
            unlockCond(&client->element.notifier);

            free(buffer);
        }
    }
    //modifyTask
    else if(!strcmp(args[0], commands[11]))
    {
        char *buffer;

        if(size == 13)
        {
            if((devices = getDeviceByName(snmpc->devices, args[1])) != NULL)
            {
                unsigned short index = atoi(args[2]);

                if(index > 0 && index < 9)
                {
                    Tasks *task;

                    if((task = getTaskByName(devices->element.tasks[index-1], args[3])) != NULL)
                    {
                        Clients *clients = snmpc->server.clients;

                        modifyTask(&task->element, args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], &snmpc->locks[LOCK_TASKS]);
                        if(modifyTaskFromXml(&snmpc->data[DATA_DEVICES], args[3], &task->element, &devices->element, index, &snmpc->locks[LOCK_TASKS]))
                        {
                            buffer = formatPacket(13,
                                                "modifyTask",
                                                args[1],
                                                args[2],
                                                args[3],
                                                args[4],
                                                args[5],
                                                args[6],
                                                args[7],
                                                args[8],
                                                args[9],
                                                args[10],
                                                args[11],
                                                args[12]);

                            while(clients != NULL)
                            {
                                pushQueue(&clients->element.sender_queue, buffer, &clients->element.sender_lock);
                                unlockCond(&clients->element.notifier);

                                clients = clients->next;
                            }

                            free(buffer);
                        }
                        else
                        {
                            fprintf(stderr, "An error occured while updating task informations: %s\n", packet);

                            buffer = formatPacket(2, commands[10], "An error occured while updating task.");
                            pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                            unlockCond(&client->element.notifier);

                            free(buffer);
                        }
                    }
                    else
                    {
                        fprintf(stderr, "Invalid task name specified: %hu\n", index);

                        buffer = formatPacket(2, commands[10], "Invalid task name specified.");
                        pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                        unlockCond(&client->element.notifier);

                        free(buffer);
                    }
                }
                else
                {
                    fprintf(stderr, "Invalid relay number specified: %hu\n", index);

                    buffer = formatPacket(2, commands[10], "Invalid relay number specified.");
                    pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                    unlockCond(&client->element.notifier);

                    free(buffer);
                }
            }
            else
            {
                fprintf(stderr, "Unknown device specified. Aborting request : %s\n", packet);

                buffer = formatPacket(2, commands[10], "Unknown device specified. Aborting request.");
                pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
                unlockCond(&client->element.notifier);

                free(buffer);
            }
        }
        else
        {
            fprintf(stderr, "Invalid packet size: %s\n", packet);

            buffer = formatPacket(2, commands[10], "Invalid modifyTask order received. Your request has been aborted.");
            pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
            unlockCond(&client->element.notifier);

            free(buffer);
        }
    }
    //DEVICES
    else if((devices = getDeviceByName(snmpc->devices, args[0])) != NULL)
    {
        SNMPRequest *request;

        if(size == 3)
        {
            char **requestValues = NULL;
            unsigned short requestSize = 0;

            if(strcmp(args[1], commands[5]) == 0)
            {
                char *packet;
                char buffer[10];

                snmpc->visualization_rate = updateVisualization(snmpc, atoi(args[2]));

                sprintf(buffer, "%hu", snmpc->visualization_rate);
                packet = formatPacket(2, "visualizationRate", buffer);

                pushQueue(&client->element.sender_queue, packet, &client->element.sender_lock);
                unlockCond(&client->element.notifier);

                free(packet);
            }
            else
            {
                request = InitRequest(&devices->element, COMMUNITY_PRIVATE);
                if(strcmp(args[2], "on") == 0)
                {
                    strcpy(args[2], "1");
                    requestValues = SendRequest(&request, args[1], "i", &args[2], &requestSize);
                }
                else if(strcmp(args[2], "off") == 0)
                {
                    strcpy(args[2], "0");
                    requestValues = SendRequest(&request, args[1], "i", &args[2], &requestSize);
                }
                else if(regex_verification(args[2], IP_ADDR) == 1)
                {
                    printf("\nPACKET IP : %s\n", packet);
                    requestValues = SendRequest(&request, args[1], "a", &args[2], &requestSize);
                }
                else if(regex_verification(args[2], MAC_ADDR) == 1)
                {
                    //char *str;
                    printf("\nPACKET MAC : %s\n", packet);
                    fprintf(stderr, "[-] Aborting MAC packet.\n");

                    /*while((str = strchr(args[2], ':')))
                    {
                        *str = ' ';
                        //strcpy(str, str+1);
                    }
                    printf("\nPACKET MAC test: %s\n", args[2]);
                    requestValues = SendRequest(&request, args[1], "x", &args[2], &requestSize);*/
                }
                else if(atoi(args[2]))
                {
                   printf("\nPACKET ATOI : %s\n", packet);
                   requestValues = SendRequest(&request, args[1], "i", &args[2], &requestSize);
                }
                else
                {
                    printf("\nPACKET STR : %s\n", packet);
                    requestValues = SendRequest(&request, args[1], "s", &args[2], &requestSize);
                }
                CloseRequest(&request);


                if(requestValues != NULL)
                {
                    char *requestPacket;
                    Clients *temp = snmpc->server.clients;

                    if(strlen(requestValues[0]) == 0)
                    {
                        unsigned short i;
                        char buffer[SOCKET_BUFFER];

                        for(i = 0; i < 8; i++)
                        {
                            sprintf(buffer, "relay%hudescription", i+1);
                            if(strcmp(args[1], buffer) == 0)
                            {
                                requestValues[0] = realloc(requestValues[0], strlen(buffer) * sizeof(char));
                                sprintf(buffer, "Relay %hu", i+1);
                                strcpy(requestValues[0], buffer);
                                break;
                            }
                        }
                    }

                    if(!strcmp(args[1], "deviceIPAddress"))
                    {
                        if(changeDeviceConfigFromXml(&snmpc->data[DATA_DEVICES],
                                                    devices->element.name,
                                                    devices->element.name,
                                                    requestValues[0],
                                                    devices->element.communities[0],
                                                    devices->element.communities[1],
                                                    &snmpc->locks[LOCK_DEVICES]))
                            strcpy(devices->element.ip, requestValues[0]);
                    }

                    requestPacket = formatPacket(3, args[0], args[1], requestValues[0]);
                    while(temp != NULL)
                    {
                        pushQueue(&temp->element.sender_queue, requestPacket, &temp->element.sender_lock);
                        unlockCond(&temp->element.notifier);

                        temp = temp->next;
                    }

                    /*pushQueue(&client->element.sender_queue, requestPacket, &client->element.sender_lock);
                    unlockCond(&client->element.notifier);*/

                    free(requestPacket);

                    for(i = 0; i < requestSize; i++)
                        free(requestValues[i]);
                    free(requestValues);
                }
                else //if(requestValues == NULL)
                {
                    char *requestPacket;

                    //requestPacket = formatPacket(3, args[0], args[1], "failed");
                    requestPacket = formatPacket(2, args[0], "disconnected");

                    pushQueue(&client->element.sender_queue, requestPacket, &client->element.sender_lock);
                    unlockCond(&client->element.notifier);

                    free(requestPacket);
                }
            }
        }
        else
        {
            char *buffer = formatPacket(2, commands[10], "Invalid device order received. Your request has been aborted.");

            fprintf(stderr, "Invalid packet size: %s\n", packet);

            pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
            unlockCond(&client->element.notifier);

            free(buffer);
        }
    }
    else
    {
        char *buffer = formatPacket(2, commands[10], "An unresolvable packet has been sent. Your request has been aborted.");

        fprintf(stderr, "Unknown command '%s' in packet: %s\n", args[0], packet);

        pushQueue(&client->element.sender_queue, buffer, &client->element.sender_lock);
        unlockCond(&client->element.notifier);

        free(buffer);
    }

    for(i=0 ; i < size ; i++)
        free(args[i]);
    free(args);

    return value;
}
