#include "includes.h"

/**
 * Initialisation des valeurs de la structure SNMPC.
 */
void initSNMPC(SNMPC *snmpc)
{
    initMatrix(snmpc);

    if(!initSNMP())
        exit(EXIT_FAILURE);

    if(loadConfig(snmpc, &snmpc->data[DATA_CONFIG])
    && initUsers(&snmpc->data[DATA_USERS], &snmpc->users, &snmpc->locks[LOCK_USERS])
    && initDevices(&snmpc->data[DATA_DEVICES], &snmpc->devices, &snmpc->locks[LOCK_DEVICES], &snmpc->locks[LOCK_TASKS])
    && initServer(&snmpc->server)
    && initScheduler(snmpc))
        snmpc->running = 1;
    else
        snmpc->running = 0;
}

unsigned short loadConfig(SNMPC *snmpc, XmlData *data)
{
    unsigned short created;

    data->path = FILE_CONFIG;
    snmpc->visualization_rate = 0;
    snmpc->server.port = 0;

    //Si le fichier config.xml n'existe pas, on l'initialise par défaut
    if((created = !file_exists(data->path)))
        createXmlFile(data->path, "config");

    //On ignore les noeuds de mise en forme XML
    xmlKeepBlanksDefault(0);
    //On parse le document et on récupère l'élément racine
    data->doc = xmlParseFile(data->path);

    if(data->doc == NULL)
        return 0;

    data->root = xmlDocGetRootElement(data->doc);

    //Si le fichier contient bien une racine
    if(data->root != NULL)
    {
        //Si le fichier vient d'être créé, on le rempli avec les valeurs par défaut
        if(created)
        {
            FILE *file;
            char buffer[SHELL_BUFFER];

            sprintf(buffer, "%d", SERVER_PORT);
            xmlNewTextChild(data->root, NULL, (xmlChar*)"server_port", (xmlChar*)buffer);
            sprintf(buffer, "%d", REFRESH_RATE);
            xmlNewTextChild(data->root, NULL, (xmlChar*)"visualization_rate", (xmlChar*)buffer);

            if((file = fopen(data->path, "wb")) != NULL)
            {
                xmlDocFormatDump(file, data->doc, 1);
                fclose(file);
            }
            else
            {
                xmlFreeDoc(data->doc);
                return 0;
            }
        }
        //Sinon, on lit le contenu des noeuds
        else
        {
            xmlNodePtr children;

            //Lecture des éléments XML et remplissage de la liste chaînée
            for(children = data->root->children ; children != NULL ; children = children->next)
            {
                if(children->type == XML_ELEMENT_NODE)
                {
                    if(!strcmp((const char*)children->name, "server_port"))
                    {
                        snmpc->server.port = atoi((const char*)xmlNodeGetContent(children));

                        if(snmpc->server.port < 1025 || snmpc->server.port > 65564)
                            snmpc->server.port = SERVER_PORT;

                    }
                    else if(!strcmp((const char*)children->name, "visualization_rate"))
                    {
                        snmpc->visualization_rate = atoi((const char*)xmlNodeGetContent(children));

                        if(snmpc->visualization_rate < 1 || snmpc->visualization_rate > 253)
                            snmpc->visualization_rate = REFRESH_RATE;

                    }
                }
            }
        }

        //Mise par défaut si non initialisé
        if(snmpc->server.port == 0)
            snmpc->server.port = SERVER_PORT;
        if(snmpc->visualization_rate == 0)
            snmpc->visualization_rate = REFRESH_RATE;
        return 1;
    }

    //Sinon free & erreur
    xmlFreeDoc(data->doc);
    return 0;
}

/**
 * Initialisation des données utilisateurs.
 */
unsigned short initUsers(XmlData *data, Users **users, pthread_mutex_t *mutex)
{
    pthread_mutex_init(mutex, NULL);

    if(!loadUsersFile(data))
    {
        fprintf(stderr, "An error occured while loading %s file\n", FILE_USERS);
        return 0;
    }
    else
        loadUsersData(data, users, mutex);

    return 1;
}

/**
 * Chargement du fichier users.xml.
 */
unsigned short loadUsersFile(XmlData *data)
{
    data->path = FILE_USERS;
    //Si le fichier users.xml n'existe pas, on le crée vide
    if(!file_exists(data->path))
        createXmlFile(data->path, "users");

    //On ignore les noeuds de mise en forme XML
    xmlKeepBlanksDefault(0);
    //On parse le document et on récupère l'élément racine
    data->doc = xmlParseFile(data->path);

    if(data->doc == NULL)
        return 0;

    data->root = xmlDocGetRootElement(data->doc);

    //Si le fichier ne contient pas de racine, erreur
    if(data->root == NULL)
    {
        xmlFreeDoc(data->doc);
        return 0;
    }

    return 1;
}

/**
 * Remplissage de la liste chaînée d'utilisateurs.
 */
void loadUsersData(XmlData *data, Users **users, pthread_mutex_t *mutex)
{
    xmlNodePtr children;
    unsigned short i;

    //On initialise la liste chaînée à NULL
    *users = NULL;

    //Lecture des éléments XML et remplissage de la liste chaînée
    for(children = data->root->children, i = 0 ; children != NULL ; children = children->next)
    {
        if(children->type == XML_ELEMENT_NODE && strcmp((const char*)children->name, "user") == 0)
        {
            xmlNodePtr infos = children->children;
            addUser(users, (const char*)xmlNodeGetContent(infos), (const char*)xmlNodeGetContent(infos->next), mutex);
            i++;
        }
    }

    if(i == 0)
        puts("No user found, use 'adduser [username] [password]' to create one");
}

/**
 * Initialisation des données des appareils.
 */
unsigned short initDevices(XmlData *data, Devices **devices, pthread_mutex_t *mutex, pthread_mutex_t *lock_tasks)
{
    pthread_mutex_init(mutex, NULL);
    pthread_mutex_init(lock_tasks, NULL);

    if(!loadDevicesFile(data))
    {
        fprintf(stderr, "An error occured while loading %s file\n", FILE_DEVICES);
        return 0;
    }
    else if(!loadDevicesData(data, devices, mutex, lock_tasks))
    {
        fprintf(stderr, "An error occured while loading devices data\n");
        return 0;
    }

    return 1;
}

/**
 * Chargement du fichier devices.xml.
 */
unsigned short loadDevicesFile(XmlData *data)
{
    data->path = FILE_DEVICES;
    //Si le fichier device.xml n'existe pas, on le crée vide
    if(!file_exists(data->path))
        createXmlFile(data->path, "devices");

    //On ignore les noeuds de mise en forme XML
    xmlKeepBlanksDefault(0);
    //On parse le document et on récupère l'élément racine
    data->doc = xmlParseFile(data->path);

    if(data->doc == NULL)
        return 0;

    data->root = xmlDocGetRootElement(data->doc);

    //Si le fichier ne contient pas de racine, erreur
    if(data->root == NULL)
    {
        xmlFreeDoc(data->doc);
        return 0;
    }

    return 1;
}

/**
 * Remplissage de la liste chaînée d'appareils.
 */
unsigned short loadDevicesData(XmlData *data, Devices **devices, pthread_mutex_t *mutex, pthread_mutex_t *lock_tasks)
{
    Device *device;
    xmlNodePtr children;
    unsigned short i;

    //On initialise la liste chaînée à NULL
    *devices = NULL;

    //Lecture des éléments XML et remplissage de la liste chaînée
    for(children = data->root->children, i = 0 ; children != NULL ; children = children->next)
    {
        if(children->type == XML_ELEMENT_NODE && strcmp((const char*)children->name, "device") == 0)
        {
            xmlNodePtr infos = children->children, schedule, relays;
            char *buffer;

            addDevice(devices, (const char*)xmlNodeGetContent(infos), (const char*)xmlNodeGetContent(infos->next), (const char*)xmlNodeGetContent(infos->next->next), (const char*)xmlNodeGetContent(infos->next->next->next), mutex);
            device = &getLastDevice(devices)->element;
            i++;

            if(infos->next->next->next != NULL && (schedule = infos->next->next->next->next) != NULL)
            {
                for(relays = schedule->children ; relays != NULL ; relays = relays->next)
                {
                    if(relays->type == XML_ELEMENT_NODE && strcmp((const char*)relays->name, "relay") == 0)
                    {
                        unsigned short nb = atoi((char*)xmlGetProp(relays, (xmlChar*)"number"));
                        xmlNodePtr tasks;

                        if(nb < 1 || nb > 8)
                            return 0;

                        device->tasks[nb-1] = NULL;

                        for(tasks = relays->children ; tasks != NULL ; tasks = tasks->next)
                        {
                            if(tasks->type == XML_ELEMENT_NODE && strcmp((const char*)tasks->name, "task") == 0)
                            {
                                xmlNodePtr name     = tasks->children;
                                xmlNodePtr enabled  = name->next;
                                xmlNodePtr repeat   = enabled->next;
                                xmlNodePtr date     = repeat->next;
                                xmlNodePtr weeks    = date->next;
                                xmlNodePtr days     = weeks->next;
                                xmlNodePtr type     = days->next;
                                xmlNodePtr start    = type->next;
                                xmlNodePtr end      = start->next;

                                addTask(&device->tasks[nb-1],
                                        (const char*)xmlNodeGetContent(name),
                                        (const char*)xmlNodeGetContent(enabled),
                                        (const char*)xmlNodeGetContent(repeat),
                                        (const char*)xmlNodeGetContent(date),
                                        (const char*)xmlNodeGetContent(weeks),
                                        (const char*)xmlNodeGetContent(days),
                                        (const char*)xmlNodeGetContent(type),
                                        (const char*)xmlNodeGetContent(start),
                                        (const char*)xmlNodeGetContent(end),
                                        lock_tasks);
                            }
                        }
                    }
                }
            }

            //getDeviceData pour mettre à jour la structure
            buffer = getDeviceData(device);
            free(buffer);
        }
    }

    if(i == 0)
        puts("No device found, use 'newdevice [name] [ip]' to create one");

    return 1;
}

/**
 * Initialisation des élément de la structure Server.
 */
unsigned short initServer(Server *server)
{
    pthread_mutex_init(&server->lock, NULL);
    server->clients = NULL;

    if(SocketStart() && initSocket(&server->socket) && socketBind(server, server->port))
    {
        server->running = 1;
        return 1;
    }
    else
    {
        server->running = 0;
        return 0;
    }
}

unsigned short initScheduler(SNMPC *snmpc)
{
    pthread_mutex_init(&snmpc->scheduler.lock, NULL);

    if(pthread_create(&snmpc->scheduler.handler, NULL, scheduler, snmpc) != 0)
    {
        snmpc->scheduler.running = 0;
        return 0;
    }
    else
    {
        snmpc->scheduler.running = 1;
        return 1;
    }
}

/**
 * Initialisation NetSNMP
 */
unsigned short initSNMP(void)
{
    init_snmp("SNMPC");

    if(!file_exists(FILE_MIB))
    {
        fprintf(stderr, "[-] Unable to locate MIB file under data/TCW.mib. Exiting.\n");
        return 0;
    }

    init_mib();
    add_mibdir(DATA_FOLDER);

    return 1;
}

/**
 * Libération de la mémoire allouée par le programme.
 */
void freeSNMPC(SNMPC *snmpc)
{
    snmpc->server.running = 0;

    xmlFreeDoc(snmpc->data[DATA_USERS].doc);
    xmlFreeDoc(snmpc->data[DATA_DEVICES].doc);
    xmlFreeDoc(snmpc->data[DATA_CONFIG].doc);

    freeUsers(&snmpc->users, &snmpc->locks[LOCK_USERS]);
    freeDevices(&snmpc->devices, &snmpc->locks[LOCK_DEVICES]);
    freeClients(&snmpc->server.clients, &snmpc->server.lock);

    closesocket(snmpc->server.socket);

    pthread_mutex_destroy(&snmpc->locks[LOCK_USERS]);
    pthread_mutex_destroy(&snmpc->locks[LOCK_DEVICES]);
    pthread_mutex_destroy(&snmpc->locks[LOCK_TASKS]);
    pthread_mutex_destroy(&snmpc->server.lock);
    pthread_mutex_destroy(&snmpc->scheduler.lock);

    SocketEnd();
}

int lockMutex(pthread_mutex_t *mutex)
{
    return pthread_mutex_lock(mutex);
}

int unlockMutex(pthread_mutex_t *mutex)
{
    return pthread_mutex_unlock(mutex);
}

int lockCond(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    return pthread_cond_wait(cond, mutex);
}

int unlockCond(pthread_cond_t *cond)
{
    return pthread_cond_broadcast(cond);
}

/**
 * Initialisation des threads de base du serveur.
 */
void initThreads(SNMPC *snmpc)
{
    pthread_create(&snmpc->server.handler, NULL, serverHandler, snmpc);
    pthread_create(&snmpc->shell.handler, NULL, shellHandler, snmpc);
}

void waitThread(pthread_t tid)
{
    pthread_join(tid, NULL);
}

/**
 * Fonction bloquant le fil d'exécution en attendant l'arrêt des threads spécifiés.
 * Blocage hiérarchique (!).
 */
void waitThreads(SNMPC *snmpc)
{
    waitThread(snmpc->shell.handler);
    waitThread(snmpc->scheduler.handler);
    //waitThread(snmpc->server.handler); //La libération de la socket arrête l'écoute et, par extension, le thread
}

/**
* Fonction de chargement de la matrice
*/
void initMatrix(SNMPC *snmpc)
{
    snmpc->matrix.matrix[0][0] = 1;
    snmpc->matrix.matrix[0][1] = 0;
    snmpc->matrix.matrix[0][2] = 0;
    snmpc->matrix.matrix[0][3] = 0;
    snmpc->matrix.matrix[0][4] = 1;
    snmpc->matrix.matrix[0][5] = 1;
    snmpc->matrix.matrix[0][6] = 0;
    snmpc->matrix.matrix[0][7] = 0;

    snmpc->matrix.matrix[1][0] = 0;
    snmpc->matrix.matrix[1][1] = 1;
    snmpc->matrix.matrix[1][2] = 0;
    snmpc->matrix.matrix[1][3] = 0;
    snmpc->matrix.matrix[1][4] = 0;
    snmpc->matrix.matrix[1][5] = 0;
    snmpc->matrix.matrix[1][6] = 1;
    snmpc->matrix.matrix[1][7] = 0;

    snmpc->matrix.matrix[2][0] = 0;
    snmpc->matrix.matrix[2][1] = 0;
    snmpc->matrix.matrix[2][2] = 1;
    snmpc->matrix.matrix[2][3] = 0;
    snmpc->matrix.matrix[2][4] = 1;
    snmpc->matrix.matrix[2][5] = 1;
    snmpc->matrix.matrix[2][6] = 1;
    snmpc->matrix.matrix[2][7] = 1;

    snmpc->matrix.matrix[3][0] = 0;
    snmpc->matrix.matrix[3][1] = 0;
    snmpc->matrix.matrix[3][2] = 0;
    snmpc->matrix.matrix[3][3] = 1;
    snmpc->matrix.matrix[3][4] = 0;
    snmpc->matrix.matrix[3][5] = 0;
    snmpc->matrix.matrix[3][6] = 0;
    snmpc->matrix.matrix[3][7] = 1;

    snmpc->matrix.columns = 8;
    snmpc->matrix.lines   = 4;
}

unsigned short updateVisualization(SNMPC *snmpc, unsigned short new_rate)
{
    FILE *file;
    XmlData *data = &snmpc->data[DATA_CONFIG];

    if(new_rate < 1)
        new_rate = 1;
    if(new_rate > 253)
        new_rate = 253;

    if((file = fopen(data->path, "wb")) != NULL)
    {
        xmlNodePtr children;

        for(children = data->root->children ; children != NULL ; children = children->next)
        {
            if(children->type == XML_ELEMENT_NODE && !strcmp((const char*)children->name, "visualization_rate"))
            {
                char buffer[4];

                sprintf(buffer, "%hu", new_rate);
                xmlNodeSetContent(children, (xmlChar*)buffer);
                break;
            }
        }

        xmlDocFormatDump(file, data->doc, 1);
        fclose(file);
    }

    return new_rate;
}

unsigned short updatePort(SNMPC *snmpc, unsigned short new_port)
{
    FILE *file;
    XmlData *data = &snmpc->data[DATA_CONFIG];

    if(new_port < 1025 || new_port > 65564)
        new_port = SERVER_PORT;

    if((file = fopen(data->path, "wb")) != NULL)
    {
        xmlNodePtr children;

        for(children = data->root->children ; children != NULL ; children = children->next)
        {
            if(children->type == XML_ELEMENT_NODE && !strcmp((const char*)children->name, "server_port"))
            {
                char buffer[6];

                sprintf(buffer, "%hu", new_port);
                xmlNodeSetContent(children, (xmlChar*)buffer);
                break;
            }
        }

        xmlDocFormatDump(file, data->doc, 1);
        fclose(file);
    }

    return new_port;
}
