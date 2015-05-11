#include "includes.h"

/**
 * Ajout d'un nouvel élément à la fin de la liste chaînée d'appareils.
 */
void addDevice(Devices **devices, const char *name, const char *ip, const char *cm_public, const char *cm_private, pthread_mutex_t *mutex)
{
    Devices *last = getLastDevice(devices);
    Devices *temp;
    unsigned short i;

    lockMutex(mutex);

    //Initialiser aussi les autres membres de la structure ?
    temp = malloc(sizeof(Devices));
    strcpy(temp->element.name, name);
    strcpy(temp->element.ip, ip);
    strcpy(temp->element.communities[COMMUNITY_PUBLIC], cm_public);
    strcpy(temp->element.communities[COMMUNITY_PRIVATE], cm_private);
    temp->element.disconnected = 0;
    temp->next = NULL;

    for(i=0 ; i < 8 ; i++)
        temp->element.tasks[i] = NULL;

    if(last == NULL)
        *devices = temp;
    else
        last->next = temp;

    unlockMutex(mutex);
}

/**
 * Ajout d'un nouvel appareil au fichier de données XML.
 */
unsigned short addDeviceToXml(XmlData *data, const char *name, const char *ip, const char *cm_public, const char *cm_private, pthread_mutex_t *mutex)
{
    FILE *file;
    unsigned short i;
    xmlNodePtr device;
    xmlNodePtr schedule;
    xmlNodePtr relays[8];

    if((file = fopen(data->path, "wb")))
    {
        lockMutex(mutex);

        //Initialisation du contenu du nouveau noeud <device>
        if((device = xmlNewNode(NULL, (xmlChar*)"device")) == NULL)
            return 0;
        if(xmlNewTextChild(device, NULL, (xmlChar*)"name", (xmlChar*)name) == NULL)
        {
            xmlFreeNode(device);
            return 0;
        }
        if(xmlNewTextChild(device, NULL, (xmlChar*)"ip", (xmlChar*)ip) == NULL)
        {
            xmlFreeNode(device);
            return 0;
        }
        if(xmlNewTextChild(device, NULL, (xmlChar*)"cm_public", (xmlChar*)cm_public) == NULL)
        {
            xmlFreeNode(device);
            return 0;
        }
        if(xmlNewTextChild(device, NULL, (xmlChar*)"cm_private", (xmlChar*)cm_private) == NULL)
        {
            xmlFreeNode(device);
            return 0;
        }
        if((schedule = xmlNewNode(NULL, (xmlChar *)"schedule")) == NULL)
        {
            xmlFreeNode(device);
            return 0;
        }

        for(i = 0; i < 8; i++)
        {
            char buffer[SOCKET_BUFFER];

            sprintf(buffer, "relay");
            if((relays[i] = xmlNewNode(NULL, (xmlChar *)buffer)) == NULL || xmlAddChild(schedule, relays[i]) == NULL)
            {
                xmlFreeNode(device);
                xmlFreeNode(schedule);
                return 0;
            }
            sprintf(buffer, "%hu", i+1);
            if(xmlSetProp(relays[i], (xmlChar *)"number", (xmlChar *)buffer) == NULL)
            {
                xmlFreeNode(device);
                xmlFreeNode(schedule);
                return 0;
            }
        }

        //Ajout du noeud <device> en fin de fichier
        if(xmlAddChild(device, schedule) == NULL || xmlAddChild(data->root, device) == NULL)
        {
            xmlFreeNode(device);
            return 0;
        }

        xmlDocFormatDump(file, data->doc, 1);
        fclose(file);

        unlockMutex(mutex);
        return 1;
    }
    else
        return 0;
}

short changeDeviceConfigFromXml(XmlData *data, const char *name, const char *newname, const char *ip, const char *cm_public, const char *cm_private, pthread_mutex_t *mutex)
{
    FILE *file;
    unsigned short index = 0, found = 0;
    xmlNodePtr children;

    lockMutex(mutex);

    //Lecture des éléments XML
    for(children = data->root->children, index = 0 ; children != NULL ; children = children->next, index++)
    {
        //S'il s'agit du bon device
        if(children->type == XML_ELEMENT_NODE
        && !strcmp((const char*)children->name, "device")
        && !strcmp((const char*)xmlNodeGetContent(children->children), name))
        {
            //Les modifications ne sont effectuées qu'à condition de pouvoir écrire dans le fichier
            if((file = fopen(data->path, "wb")))
            {
                found = 1;

                xmlNodeSetContent(children->children, (xmlChar *)newname);
                xmlNodeSetContent(children->children->next, (xmlChar *)ip);
                xmlNodeSetContent(children->children->next->next, (xmlChar *)cm_public);
                xmlNodeSetContent(children->children->next->next->next, (xmlChar *)cm_private);
                xmlDocFormatDump(file, data->doc, 1);
                fclose(file);
            }
            break;
        }
    }

    unlockMutex(mutex);

    if(found)
        return 1;
    else
        return -1;
}

/**
 * Suppression de l'élément de liste chaînée situé à l'indice donné.
 */
void deleteDevice(Devices **devices, unsigned short index, pthread_mutex_t *mutex)
{
    if(index > 0)
    {
        Devices *prev = getDevice(*devices, index-1);
        Devices *next = getDevice(*devices, index+1);

        lockMutex(mutex);

        free(getDevice(*devices, index));
        prev->next = next;
    }
    //Si on veut supprimer le premier utilisateur
    else
    {
        Devices *next = (*devices)->next;

        lockMutex(mutex);

        free(*devices);
        *devices = next;
    }

    unlockMutex(mutex);
}

/**
 * Renvoie l'indice de l'utilisateur dans la liste en cas de succès, -1 sinon.
 */
short deleteDeviceFromXml(XmlData *data, const char *name, pthread_mutex_t *mutex)
{
    FILE *file;
    unsigned short index = 0, found = 0;
    xmlNodePtr children;

    lockMutex(mutex);

    //Lecture des éléments XML
    for(children = data->root->children, index = 0 ; children != NULL ; children = children->next, index++)
    {
        //S'il s'agit du bon device
        if(children->type == XML_ELEMENT_NODE
        && !strcmp((const char*)children->name, "device")
        && !strcmp((const char*)xmlNodeGetContent(children->children), name))
        {
            //Les modifications ne sont effectuées qu'à condition de pouvoir écrire dans le fichier
            if((file = fopen(data->path, "wb")))
            {
                found = 1;

                xmlUnlinkNode(children);
                xmlFreeNode(children);
                xmlDocFormatDump(file, data->doc, 1);
                fclose(file);
            }
            break;
        }
    }

    unlockMutex(mutex);

    //Si l'appareil a été trouvé et supprimé, on renvoie son index
    if(found)
        return index;
    else
        return -1;
}

/**
 * Renvoie un pointeur sur le dernier élément alloué de la liste chaînée d'appareils.
 */
Devices *getLastDevice(Devices **devices)
{
    if(*devices != NULL)
    {
        if((*devices)->next != NULL)
            return getLastDevice(&(*devices)->next);
        else
            return *devices;
    }
    else
        return NULL;
}

/**
 * Renvoie un pointeur sur l'élément de liste chaînée situé à l'indice donné.
 */
Devices *getDevice(Devices *devices, unsigned short index)
{
    unsigned short i;

    for(i=0 ; i < index ; i++)
        devices = devices->next;

    return devices;
}

Devices *getDeviceByName(Devices *devices, const char *name)
{
    while(devices != NULL)
    {
        if(strcmp(devices->element.name, name) == 0)
            return devices;
        devices = devices->next;
    }

    return NULL;
}

/**
 * Renvoie le nombre d'éléments contenus dans la liste chaînée d'appareils.
 */
unsigned short countDevices(Devices *devices)
{
    unsigned short i = 0;

    while(devices != NULL)
    {
        i++;
        devices = devices->next;
    }

    return i;
}

/**
 * Renvoie 1 si un appareil existant porte ce nom, 0 sinon.
 */
unsigned short deviceNameExists(Devices *devices, const char *name)
{
    unsigned short i, count = countDevices(devices);

    for(i=0 ; i < count ; i++)
    {
        Device device = getDevice(devices, i)->element;

        if(strcmp(device.name, name) == 0)
            return 1;
    }

    return 0;
}

/**
 * Renvoie 1 si un appareil existant utilise cette IP, 0 sinon.
 */
unsigned short deviceIpExists(Devices *devices, const char *ip)
{
    unsigned short i, count = countDevices(devices);

    for(i=0 ; i < count ; i++)
    {
        Device device = getDevice(devices, i)->element;

        if(strcmp(device.ip, ip) == 0)
            return 1;
    }

    return 0;
}

/**
 * Libère la mémoire allouée pour la liste chaînée d'appareils.
 */
void freeDevices(Devices **devices, pthread_mutex_t *mutex)
{
    while(*devices != NULL)
    {
        Devices *temp = (*devices)->next;
        free(*devices);
        *devices = temp;
    }
}

/**
 * Affiche le contenu de la liste chaînée d'appareils sur stdout.
 */
void printDevices(Devices *devices)
{
    while(devices != NULL)
    {
        printf("%s [%s]\n", devices->element.name, devices->element.ip);
        devices = devices->next;
    }
}

char *getDeviceData(Device *device)
{
    ushort  i, j, jump = 0;
    char    buffer[SOCKET_BUFFER], *packet;
    char    *commands[12] = {
        "name",
        "version",
        "date",
        "dhcpConfig",
        "deviceIPAddress",
        "subnetMask",
        "gateway",
        "deviceMACAddress",
        "trapEnabled",
        "trapReceiverIPAddress",
        "trapCommunity",
        "digitalInput"
    };

    packet = malloc(SOCKET_BUFFER * sizeof(char));
    packet[0] = '\0';

    for(i = 0; i < 12; i++)
    {
        SNMPRequest *request;
        char **requestValues = NULL;
        unsigned short requestSize;

        request = InitRequest(device, COMMUNITY_PUBLIC);
        requestValues = SendRequest(&request, commands[i], NULL, NULL, &requestSize);

        if(requestValues != NULL)
        {
            char *requestPacket;

            //Si nom du device invalide (!= TCW10B ou TCW181B-CM)
            if(!strcmp(commands[i], "name") && strcmp(requestValues[0], "TCW180B") != 0 && strcmp(requestValues[0], "TCW181B-CM") != 0)
            {
                requestPacket = formatPacket(2, device->name, "disconnected");
                strcat(packet, requestPacket);
                free(requestPacket);

                requestPacket = formatPacket(2, "error", "You configured an unsupported device. Please plugin a Teracom TCW180B or TCW181B-CM instead.");
                strcat(packet, requestPacket);
                free(requestPacket);

                fprintf(stderr, "[-] An invalid device (%s [%s]) has been configured : '%s'\n", device->name, device->ip, requestValues[0]);

                jump = 1;
                break;
            }

            //Dans tous les cas, les données sont formattées
            requestPacket = formatPacket(3, device->name, commands[i], requestValues[0]);
            strcat(packet, requestPacket);
            free(requestPacket);

            for(j = 0; j < requestSize; j++)
                free(requestValues[j]);
            free(requestValues);
        }
        else if(requestValues == NULL /*&& !strcmp(commands[i], "name")*/)
        {
            char *requestPacket;

            requestPacket = formatPacket(2, device->name, "disconnected");
            strcat(packet, requestPacket);
            //pushQueue(&client->sender_queue, requestPacket, &client->sender_lock);
            jump = 1;

            free(requestPacket);

            break;
        }

        if(request != NULL)
            CloseRequest(&request);
    }

    if(jump == 0)
    {
        for(i = 0; i < 8; i++)
        {
            SNMPRequest *request;
            char **requestValues = NULL;
            unsigned short requestSize;

            //relays
            sprintf(buffer, "relay%hu", i+1);

            request = InitRequest(device, COMMUNITY_PUBLIC);
            requestValues = SendRequest(&request, buffer, NULL, NULL, &requestSize);

            if(requestValues != NULL)
            {
                char *requestPacket;

                requestPacket = formatPacket(3, device->name, buffer, requestValues[0]);
                device->relays_state[i] = (unsigned short)atoi(requestValues[0]);

                strcat(packet, requestPacket);
                //pushQueue(&client->sender_queue, requestPacket, &client->sender_lock);

                free(requestPacket);

                for(j = 0; j < requestSize; j++)
                    free(requestValues[j]);
                free(requestValues);
            }
            else if(requestValues == NULL)
            {
                char *requestPacket;

                requestPacket = formatPacket(2, device->name, "disconnected");
                strcat(packet, requestPacket);
                free(requestPacket);

                break;
            }

            if(request != NULL)
                CloseRequest(&request);

            //relays description
            sprintf(buffer, "relay%hudescription", i+1);

            request = InitRequest(device, COMMUNITY_PUBLIC);
            requestValues = SendRequest(&request, buffer, NULL, NULL, &requestSize);

            if(requestValues != NULL)
            {
                char *requestPacket;

                if(!strcmp(requestValues[0], ""))
                {
                    char buf[20];

                    free(requestValues[0]);
                    requestValues[0] = malloc(strlen(buf) * sizeof(char));
                    sprintf(buf, "Relay %hu", i+1);
                    strcpy(requestValues[0], buf);
                }

                requestPacket = formatPacket(3, device->name, buffer, requestValues[0]);

                strcat(packet, requestPacket);
                //pushQueue(&client->sender_queue, requestPacket, &client->sender_lock);

                free(requestPacket);

                for(j = 0; j < requestSize; j++)
                    free(requestValues[j]);
                free(requestValues);
            }
            else if(requestValues == NULL)
            {
                char *requestPacket;

                requestPacket = formatPacket(2, device->name, "disconnected");
                strcat(packet, requestPacket);
                free(requestPacket);

                break;
            }

            if(request != NULL)
                CloseRequest(&request);

            //pulse duration
            sprintf(buffer, "relay%huPulseDuration", i+1);

            request = InitRequest(device, COMMUNITY_PUBLIC);
            requestValues = SendRequest(&request, buffer, NULL, NULL, &requestSize);

            if(requestValues != NULL)
            {
                char *requestPacket;

                requestPacket = formatPacket(3, device->name, buffer, requestValues[0]);
                device->pulse_duration[i] = (unsigned short)atoi(requestValues[0]);

                strcat(packet, requestPacket);
                //pushQueue(&client->sender_queue, requestPacket, &client->sender_lock);

                free(requestPacket);

                for(j = 0; j < requestSize; j++)
                    free(requestValues[j]);
                free(requestValues);
            }
            else if(requestValues == NULL)
            {
                char *requestPacket;

                requestPacket = formatPacket(2, device->name, "disconnected");
                strcat(packet, requestPacket);
                free(requestPacket);

                break;
            }

            if(request != NULL)
                CloseRequest(&request);
        }

        for(i=0 ; i < 8 ; i++)
        {
            Tasks *tasks = device->tasks[i];

            while(tasks != NULL)
            {
                char task_buffer[7][SOCKET_BUFFER];
                char *requestPacket;

                sprintf(buffer, "%hu", i+1);
                sprintf(task_buffer[0], "%hu", tasks->element.enabled);
                sprintf(task_buffer[1], "%hu", tasks->element.repeat);
                sprintf(task_buffer[2], "%hu", tasks->element.weeks);
                sprintf(task_buffer[3], "%hu,%hu,%hu,%hu,%hu,%hu,%hu", tasks->element.days[0], tasks->element.days[1], tasks->element.days[2], tasks->element.days[3], tasks->element.days[4], tasks->element.days[5], tasks->element.days[6]);
                sprintf(task_buffer[4], "%hu", tasks->element.type);
                sprintf(task_buffer[5], "%huh%hu", tasks->element.hours[0], tasks->element.minutes[0]);
                sprintf(task_buffer[6], "%huh%hu", tasks->element.hours[1], tasks->element.minutes[1]);

                requestPacket = formatPacket(12,
                                            "addTask",
                                            device->name,
                                            buffer,
                                            tasks->element.name,
                                            task_buffer[0],
                                            task_buffer[1],
                                            tasks->element.date,
                                            task_buffer[2],
                                            task_buffer[3],
                                            task_buffer[4],
                                            task_buffer[5],
                                            task_buffer[6]);
                strcat(packet, requestPacket);
                free(requestPacket);

                tasks = tasks->next;
            }
        }
    }

    return packet;
}

void sendDevicesData(SNMPC *snmpc, Client *client)
{
    Devices *temp = snmpc->devices;
    char *buffer;

    //Si aucun device n'est configuré
    if(temp == NULL)
    {
        buffer = formatPacket(1, "noDevice");
        pushQueue(&client->sender_queue, buffer, &client->sender_lock);
        unlockCond(&client->notifier);
        free(buffer);
    }
    //Sinon, on les parcourt
    else while(temp != NULL)
    {
        buffer = formatPacket(5, "addDevice",
                             temp->element.name,
                             temp->element.ip,
                             temp->element.communities[0],
                             temp->element.communities[1]);

        pushQueue(&client->sender_queue, buffer, &client->sender_lock);
        free(buffer);

        buffer = getDeviceData(&temp->element);

        pushQueue(&client->sender_queue, buffer, &client->sender_lock);
        free(buffer);

        unlockCond(&client->notifier);


        unlockMutex(&client->lock);
        usleep(250);
        lockMutex(&client->lock);

        temp = temp->next;
    }

    //visualization rate
    buffer = malloc(SOCKET_BUFFER * sizeof(char));

    sprintf(buffer,"%s;%hu|", "visualizationRate", snmpc->visualization_rate);
    pushQueue(&client->sender_queue, buffer, &client->sender_lock);

    free(buffer);

    unlockCond(&client->notifier);
}
