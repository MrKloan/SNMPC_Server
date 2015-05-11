#include "includes.h"

void *shellHandler(void *data)
{
    SNMPC *snmpc = (SNMPC*)data;

    snmpc->shell.identified = countUsers(snmpc->users) > 0 ? 0 : 1;
    strcpy(snmpc->shell.hostname, SHELL_NAME);

    while(snmpc->running)
    {
        if(snmpc->shell.identified)
            shellInput(snmpc);
        else
            snmpc->shell.identified = shellLogin(snmpc);
    }

    return NULL;
}

unsigned short shellLogin(SNMPC *snmpc)
{
    Users *user;
    char username[SHELL_BUFFER], password[SHELL_BUFFER], buffer[SOCKET_BUFFER];
    SHA1Context sha;

    printf("Username : ");
    fgets(username, SHELL_BUFFER, stdin);
    if(username[strlen(username)-1] == '\n')
        username[strlen(username)-1] = '\0';

    printf("Password : ");
    fgets(password, SHELL_BUFFER, stdin);
    if(password[strlen(password)-1] == '\n')
        password[strlen(password)-1] = '\0';
    SHA1Reset(&sha);

    sprintf(buffer, "%s%s%s", SALT, username, PEPPER);

    SHA1Input(&sha,
        (const unsigned char *)buffer,
        strlen(buffer)
    );

    SHA1Result(&sha);

    strcpy(username, sha.sha_message);

    SHA1Reset(&sha);

    sprintf(buffer, "%s%s%s", SALT, password, PEPPER);

    SHA1Input(&sha,
        (const unsigned char *)buffer,
        strlen(buffer)
    );

    SHA1Result(&sha);

    strcpy(password, sha.sha_message);

    user = getUserByName(snmpc->users, username);

    //Valid login
    if(snmpc->shell.identified || (user != NULL && !strcmp(user->element.password, password)))
    {
        puts("\n[+] Welcome in!");
        return 1;
    }
    else
    {
        puts("[-] Invalid login");
        return 0;
    }
}

void shellInput(SNMPC *snmpc)
{
    printf("%s> ", snmpc->shell.hostname);

    if(fgets(snmpc->shell.cmd, SHELL_BUFFER, stdin) != NULL && snmpc->shell.cmd[0] != '\n')
    {
        if(snmpc->shell.cmd[strlen(snmpc->shell.cmd)-1] == '\n')
            snmpc->shell.cmd[strlen(snmpc->shell.cmd)-1] = '\0';
        commandHandler(snmpc);
    }
}

void commandHandler(SNMPC *snmpc)
{
    unsigned short size;
    char **args = explode(snmpc->shell.cmd, " ", &size);

    if(!strcmp(args[0], "exit"))
        commandExit(snmpc, &args, size);
    else if(!strcmp(args[0], "logout"))
        commandLogout(snmpc);
    else if(!strcmp(args[0], "help") || !strcmp(args[0], "?"))
        commandHelp(snmpc);
    else if(!strcmp(args[0], "hostname"))
        commandHostname(snmpc, args, size);
    else if(!strcmp(args[0], "adduser"))
        commandAdduser(snmpc, args, size);
    else if(!strcmp(args[0], "deluser"))
        commandDeluser(snmpc, args, size);
    else if(!strcmp(args[0], "newdevice"))
        commandNewdevice(snmpc, args, size);
    else if(!strcmp(args[0], "deldevice"))
        commandDeldevice(snmpc, args, size);
    else if(!strcmp(args[0], "list"))
        commandList(snmpc, args, size);
    else if(!strcmp(args[0], "echo"))
        commandEcho(snmpc, args, size);
    else if(!strcmp(args[0], "gnu"))
        commandGnu(snmpc, args, size);
    else if(!strcmp(args[0], "request"))
        commandRequest(snmpc, args, size);
    else if(!strcmp(args[0], "port"))
        commandPort(snmpc, args, size);
    else
        puts("Unknown command");

    freeCommandArgs(&args, size);
}

/**
 * On libère la mémoire allouée par l'explode de la commande
 */
void freeCommandArgs(char ***args, unsigned short size)
{
    unsigned short i;

    for(i=0 ; i <  size ; i++)
        free((*args)[i]);

    free(*args);
}

///Commandes

void commandExit(SNMPC *snmpc, char ***args, unsigned short size)
{
    freeCommandArgs(args, size);
    snmpc->running = 0;

    pthread_exit(NULL);
}

void commandLogout(SNMPC *snmpc)
{
    snmpc->shell.identified = countUsers(snmpc->users) > 0 ? 0 : 1;
}

void commandHelp(SNMPC *snmpc)
{
    return;
}

void commandHostname(SNMPC *snmpc, char **args, unsigned short size)
{
    if(size == 2)
        strncpy(snmpc->shell.hostname, args[1], strlen(args[1])+1);
    else
        puts("Usage: hostname [new_name]");
}

void commandAdduser(SNMPC *snmpc, char **args, unsigned short size)
{
    if(size == 3)
    {
        char username[SHELL_BUFFER], password[SHELL_BUFFER], buffer[SOCKET_BUFFER];
        SHA1Context sha;

        SHA1Reset(&sha);

        sprintf(buffer, "%s%s%s", SALT, args[1], PEPPER);

        SHA1Input(&sha,
            (const unsigned char *)buffer,
            strlen(buffer)
        );

        SHA1Result(&sha);

        strcpy(username, sha.sha_message);

        SHA1Reset(&sha);

        sprintf(buffer, "%s%s%s", SALT, args[2], PEPPER);

        SHA1Input(&sha,
            (const unsigned char *)buffer,
            strlen(buffer)
        );

        SHA1Result(&sha);

        strcpy(password, sha.sha_message);

        if(!userExists(snmpc->users, username))
        {
            if(addUserToXml(&snmpc->data[DATA_USERS], username, password, &snmpc->locks[LOCK_USERS]))
            {
                addUser(&snmpc->users, username, password, &snmpc->locks[LOCK_USERS]);
                printf("User %s successfully created.\n", args[1]);
            }
            else
                fprintf(stderr, "An error occured while creating user %s.\n", args[1]);
        }
        else
            fprintf(stderr, "User %s already exists.\n", args[1]);
    }
    else
        puts("Usage: adduser [username] [password]");
}

void commandDeluser(SNMPC *snmpc, char **args, unsigned short size)
{
    if(size == 3)
    {
        short index;
        char username[SHELL_BUFFER], password[SHELL_BUFFER], buffer[SOCKET_BUFFER];
        SHA1Context sha;

        SHA1Reset(&sha);

        sprintf(buffer, "%s%s%s", SALT, args[1], PEPPER);

        SHA1Input(&sha,
            (const unsigned char *)buffer,
            strlen(buffer)
        );

        SHA1Result(&sha);

        strcpy(username, sha.sha_message);

        SHA1Reset(&sha);

        sprintf(buffer, "%s%s%s", SALT, args[2], PEPPER);

        SHA1Input(&sha,
            (const unsigned char *)buffer,
            strlen(buffer)
        );

        SHA1Result(&sha);

        strcpy(password, sha.sha_message);


        if((index = deleteUserFromXml(&snmpc->data[DATA_USERS], username, password, &snmpc->locks[LOCK_USERS])) > -1)
        {
            deleteUser(&snmpc->users, (unsigned short)index, &snmpc->locks[LOCK_USERS]);
            printf("User %s successfully deleted\n", args[1]);

            if(snmpc->users == NULL)
                snmpc->shell.identified = 1;
        }
        else
            fprintf(stderr, "An error occured while deleting user %s. Please check for errors in the username or password.\n", args[1]);
    }
    else
        puts("Usage: deluser [username] [password]");
}

void commandNewdevice(SNMPC *snmpc, char **args, unsigned short size)
{
    if(size == 5)
    {
        if(!deviceNameExists(snmpc->devices, args[1]))
        {
            if(!deviceIpExists(snmpc->devices, args[2]))
            {
                if(addDeviceToXml(&snmpc->data[DATA_DEVICES], args[1], args[2], args[3], args[4], &snmpc->locks[LOCK_DEVICES]))
                {
                    Clients *temp;
                    Devices *devices;
                    char *buffer;

                    addDevice(&snmpc->devices, args[1], args[2],  args[3], args[4], &snmpc->locks[LOCK_DEVICES]);

                    devices = getLastDevice(&snmpc->devices);
                    free(getDeviceData(&devices->element));
                    printf("Device %s [%s] successfully created.\n", args[1], args[2]);

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
                    fprintf(stderr, "An error occured while creating device %s [%s].\n", args[1], args[2]);
            }
            else
                fprintf(stderr, "IP adress %s already linked to a device.", args[2]);
        }
        else
            fprintf(stderr, "Device name %s already in use.\n", args[1]);
    }
    else
        puts("Usage: newdevice [name] [ip] [cm_public] [cm_private]");
}

void commandDeldevice(SNMPC *snmpc, char **args, unsigned short size)
{
    if(size == 2)
    {
        short index;

        if((index = deleteDeviceFromXml(&snmpc->data[DATA_DEVICES], args[1], &snmpc->locks[LOCK_DEVICES])) > -1)
        {
            char *packet;
            Clients *clients;

            deleteDevice(&snmpc->devices, (unsigned short)index, &snmpc->locks[LOCK_DEVICES]);
            printf("Device %s successfully deleted\n", args[1]);

            packet = formatPacket(2, "delDevice", args[1]);

            if(snmpc->devices == NULL)
            {
                char *buffer = formatPacket(1, "noDevice");
                packet = realloc(packet, (strlen(packet) + strlen(buffer) + 1) * sizeof(char));
                strcat(packet, buffer);
                free(buffer);
            }

            clients = snmpc->server.clients;
            while(clients != NULL)
            {
                pushQueue(&clients->element.sender_queue, packet, &clients->element.sender_lock);
                unlockCond(&clients->element.notifier);
                clients = clients->next;
            }
            free(packet);
        }
        else
            fprintf(stderr, "An error occured while deleting device %s. Please make sure that the specified name is correct.\n", args[1]);
    }
    else
        puts("Usage: deldevice [name]");
}

void commandList(SNMPC *snmpc, char **args, unsigned short size)
{
    if(size == 2)
    {
        if(!strcmp(args[1], "users"))
            printUsers(snmpc->users);
        else if(!strcmp(args[1], "devices"))
            printDevices(snmpc->devices);
        else if(!strcmp(args[1], "schedules"))
            puts("Not implemented yet.");
        else if(!strcmp(args[1], "clients"))
            printClients(snmpc->server.clients);
        else
            puts("Usage: list [users|devices|schedules|clients]");
    }
    else
        puts("Usage: list [users|devices|schedules]");
}

void commandEcho(SNMPC *snmpc, char **args, unsigned short size)
{
    if(size > 1)
    {
        unsigned short i;

        for(i=1 ; i < size ; i++)
            printf("%s ", args[i]);

        puts("");
    }
    else
        puts("Usage: echo [...]");
}

void commandGnu(SNMPC *snmpc, char **args, unsigned short size)
{
    puts("^__^");
    puts("(oo)\\________");
    puts("(__)\\        )\\/\\");
    puts("    ||------w|");
    puts("    ||      ||");
}

void commandRequest(SNMPC *snmpc, char **args, unsigned short size)
{
    SNMPRequest *request;
    Devices *device;
    char **requestValues = NULL;
    unsigned short requestSize = 0, i;

    if(size == 3)
    {
        if((device = getDeviceByName(snmpc->devices, args[1])) != NULL)
        {
            request = InitRequest(&device->element, COMMUNITY_PUBLIC);
            requestValues = SendRequest(&request, args[2], NULL, NULL, &requestSize);
            CloseRequest(&request);
        }
        else
            fprintf(stderr, "Unknown device name %s.\n", args[1]);
    }
    else if(size == 5)
    {
        if((device = getDeviceByName(snmpc->devices, args[1])) != NULL)
        {
            request = InitRequest(&device->element, COMMUNITY_PRIVATE);
            requestValues = SendRequest(&request, args[2], &args[3][0], &args[4], &requestSize);
            CloseRequest(&request);
        }
        else
            fprintf(stderr, "Unknown device name %s.\n", args[1]);
    }
    else
        puts("Usage: request [device_name] [command] {[mode] [value]}");

    if(requestValues != NULL)
    {
        for(i = 0; i < requestSize; i++)
            free(requestValues[i]);
        free(requestValues);
    }
}

void commandPort(SNMPC *snmpc, char **args, unsigned short size)
{
    if(size == 2)
    {
        size_t port = atoi(args[1]);

        if(port >= 1025 && port <= 65564)
        {
            snmpc->server.port = updatePort(snmpc, (unsigned short)port);
            printf("Server port has been successfully set to %hu\n", snmpc->server.port);
            puts("Please restart the application if you want the changes to take effect.");

            /*puts("Reloading server...");
            snmpc->server.running = 0;
            freeClients(&snmpc->server.clients, &snmpc->server.lock);
            closesocket(snmpc->server.socket);
            pthread_mutex_destroy(&snmpc->server.lock);

            initServer(&snmpc->server);*/
        }
        else
            puts("Usable ports: 1025 - 65564");
    }
    else
        puts("Usage: port [port_number]");
}
