#include "includes.h"

/**
 * Ajout d'un nouvel élément à la fin de la liste chaînée de tâches.
 */
void addTask(Tasks **tasks, const char *name, const char *enabled, const char *repeat, const char *date, const char *weeks, const char *days, const char *type, const char *start, const char *end, pthread_mutex_t *mutex)
{
    Tasks *last = getLastTask(tasks);
    Tasks *temp;
    char **args;
    unsigned short i, size = 0;

    lockMutex(mutex);

    temp = malloc(sizeof(Tasks));

    strcpy(temp->element.name, name);
    temp->element.enabled = atoi(enabled);
    temp->element.repeat = atoi(repeat);
    strcpy(temp->element.date, date);
    temp->element.weeks = atoi(weeks);
    temp->element.type = atoi(type);
    temp->element.processing = 0;

    args = explode((char*)days, ",", &size);
    if(size == 7)
    {
        for(i=0 ; i < 7 ; i++)
        {
            temp->element.days[i] = atoi(args[i]);
            free(args[i]);
        }
        free(args);
    }

    args = explode((char*)start, "h", &size);
    if(size == 2)
    {
        temp->element.hours[0] = atoi(args[0]);
        temp->element.minutes[0] = atoi(args[1]);

        free(args[0]);
        free(args[1]);
        free(args);
    }

    args = explode((char*)end, "h", &size);
    if(size == 2)
    {
        temp->element.hours[1] = atoi(args[0]);
        temp->element.minutes[1] = atoi(args[1]);

        free(args[0]);
        free(args[1]);
        free(args);
    }

    temp->next = NULL;

    if(last == NULL)
        *tasks = temp;
    else
        last->next = temp;

    unlockMutex(mutex);
}

unsigned short addTaskToXml(XmlData *data, Task *task, Device *device, unsigned short relay_number, pthread_mutex_t *mutex)
{
    FILE *file;

    if((file = fopen(data->path, "wb")) != NULL)
    {
        xmlNodePtr children;

        lockMutex(mutex);

        for(children = data->root->children ; children != NULL ; children = children->next)
        {
            if(children->type == XML_ELEMENT_NODE
            && strcmp((const char*)children->name, "device") == 0
            && strcmp((const char*)xmlNodeGetContent(children->children), device->name) == 0)
            {
                xmlNodePtr schedule;

                for(schedule = children->children ; schedule != NULL ; schedule = schedule->next)
                {
                    if(schedule->type == XML_ELEMENT_NODE
                    && strcmp((const char*)schedule->name, "schedule") == 0)
                    {
                        xmlNodePtr relay;

                        for(relay = schedule->children ; relay != NULL ; relay = relay->next)
                        {
                            if(relay->type == XML_ELEMENT_NODE
                            && strcmp((const char*)relay->name, "relay") == 0)
                            {
                                 unsigned short nb = atoi((char*)xmlGetProp(relay, (xmlChar*)"number"));

                                 if(nb == relay_number)
                                 {
                                    xmlNodePtr task_node;
                                    char buffer[SOCKET_BUFFER];

                                    if((task_node = xmlNewNode(NULL, (xmlChar*)"task")) == NULL)
                                    {
                                        fclose(file);
                                        unlockMutex(mutex);
                                        return 0;
                                    }

                                    xmlNewTextChild(task_node, NULL, (xmlChar*)"name", (xmlChar*)task->name);

                                    sprintf(buffer, "%hu", task->enabled);
                                    xmlNewTextChild(task_node, NULL, (xmlChar*)"enabled", (xmlChar*)buffer);

                                    sprintf(buffer, "%hu", task->repeat);
                                    xmlNewTextChild(task_node, NULL, (xmlChar*)"repeat", (xmlChar*)buffer);

                                    xmlNewTextChild(task_node, NULL, (xmlChar*)"date", (xmlChar*)task->date);

                                    sprintf(buffer, "%hu", task->weeks);
                                    xmlNewTextChild(task_node, NULL, (xmlChar*)"weeks", (xmlChar*)buffer);

                                    sprintf(buffer, "%hu,%hu,%hu,%hu,%hu,%hu,%hu", task->days[0], task->days[1], task->days[2], task->days[3], task->days[4], task->days[5], task->days[6]);
                                    xmlNewTextChild(task_node, NULL, (xmlChar*)"days", (xmlChar*)buffer);

                                    sprintf(buffer, "%hu", task->type);
                                    xmlNewTextChild(task_node, NULL, (xmlChar*)"type", (xmlChar*)buffer);

                                    sprintf(buffer, "%huh%hu", task->hours[0], task->minutes[0]);
                                    xmlNewTextChild(task_node, NULL, (xmlChar*)"start", (xmlChar*)buffer);

                                    sprintf(buffer, "%huh%hu", task->hours[1], task->minutes[1]);
                                    xmlNewTextChild(task_node, NULL, (xmlChar*)"end", (xmlChar*)buffer);

                                    if(xmlAddChild(relay, task_node) == NULL)
                                    {
                                        xmlFreeNode(task_node);
                                        fclose(file);
                                        unlockMutex(mutex);
                                        return 0;
                                    }

                                    xmlDocFormatDump(file, data->doc, 1);
                                    fclose(file);
                                    unlockMutex(mutex);
                                    return 1;
                                 }
                            }
                        }
                    }
                }
            }
        }

        xmlDocFormatDump(file, data->doc, 1);
        fclose(file);
        unlockMutex(mutex);
    }

    return 0;
}

/**
 * Suppression de l'élément de liste chaînée situé à l'indice donné.
 */
void deleteTask(Tasks **tasks, unsigned short index, pthread_mutex_t *mutex)
{
    if(index > 0)
    {
        Tasks *prev = getTask(*tasks, index-1);
        Tasks *next = getTask(*tasks, index+1);

        lockMutex(mutex);

        free(getTask(*tasks, index));
        prev->next = next;
    }
    //Si on veut supprimer le premier utilisateur
    else
    {
        Tasks *next = (*tasks)->next;

        lockMutex(mutex);

        free(*tasks);
        *tasks = next;
    }

    unlockMutex(mutex);
}

unsigned short deleteTaskFromXml(XmlData *data, Task *task, Device *device, unsigned short relay_number, pthread_mutex_t *mutex)
{
    FILE *file;

    if((file = fopen(data->path, "wb")) != NULL)
    {
        xmlNodePtr children;

        lockMutex(mutex);

        for(children = data->root->children ; children != NULL ; children = children->next)
        {
            if(children->type == XML_ELEMENT_NODE
            && strcmp((const char*)children->name, "device") == 0
            && strcmp((const char*)xmlNodeGetContent(children->children), device->name) == 0)
            {
                xmlNodePtr schedule;

                for(schedule = children->children ; schedule != NULL ; schedule = schedule->next)
                {
                    if(schedule->type == XML_ELEMENT_NODE
                    && strcmp((const char*)schedule->name, "schedule") == 0)
                    {
                        xmlNodePtr relay;

                        for(relay = schedule->children ; relay != NULL ; relay = relay->next)
                        {
                            if(relay->type == XML_ELEMENT_NODE
                            && strcmp((const char*)relay->name, "relay") == 0)
                            {
                                unsigned short nb = atoi((char*)xmlGetProp(relay, (xmlChar*)"number"));

                                if(nb == relay_number)
                                {
                                    xmlNodePtr tasks;

                                    for(tasks = relay->children ; tasks != NULL ; tasks = tasks->children)
                                    {
                                        if(tasks->type == XML_ELEMENT_NODE
                                        && strcmp((const char*)tasks->name, "task") == 0
                                        && strcmp((const char*)xmlNodeGetContent(tasks->children), task->name) == 0)
                                        {
                                            xmlUnlinkNode(tasks);
                                            xmlFreeNode(tasks);

                                            xmlDocFormatDump(file, data->doc, 1);
                                            fclose(file);
                                            unlockMutex(mutex);
                                            return 1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        xmlDocFormatDump(file, data->doc, 1);
        fclose(file);
        unlockMutex(mutex);
    }

    return 0;
}

/**
 * Mise à jour des informations de la tâche
 */
void modifyTask(Task *task, const char *name, const char *enabled, const char *repeat, const char *date, const char *weeks, const char *days, const char *type, const char *start, const char *end, pthread_mutex_t *mutex)
{
    char **args;
    unsigned short i, size = 0;

    lockMutex(mutex);

    strcpy(task->name, name);
    task->enabled = atoi(enabled);
    task->repeat = atoi(repeat);
    strcpy(task->date, date);
    task->weeks = atoi(weeks);
    task->type = atoi(type);
    task->processing = 0;

    args = explode((char*)days, ",", &size);
    if(size == 7)
    {
        for(i=0 ; i < 7 ; i++)
        {
            task->days[i] = atoi(args[i]);
            free(args[i]);
        }
        free(args);
    }

    args = explode((char*)start, "h", &size);
    if(size == 2)
    {
        task->hours[0] = atoi(args[0]);
        task->minutes[0] = atoi(args[1]);

        free(args[0]);
        free(args[1]);
        free(args);
    }

    args = explode((char*)end, "h", &size);
    if(size == 2)
    {
        task->hours[1] = atoi(args[0]);
        task->minutes[1] = atoi(args[1]);

        free(args[0]);
        free(args[1]);
        free(args);
    }

    unlockMutex(mutex);
}

unsigned short modifyTaskFromXml(XmlData *data, const char *old_name, Task *task, Device *device, unsigned short relay_number, pthread_mutex_t *mutex)
{
    FILE *file;

    if((file = fopen(data->path, "wb")) != NULL)
    {
        xmlNodePtr children;

        lockMutex(mutex);

        for(children = data->root->children ; children != NULL ; children = children->next)
        {
            if(children->type == XML_ELEMENT_NODE
            && strcmp((const char*)children->name, "device") == 0
            && strcmp((const char*)xmlNodeGetContent(children->children), device->name) == 0)
            {
                xmlNodePtr schedule;

                for(schedule = children->children ; schedule != NULL ; schedule = schedule->next)
                {
                    if(schedule->type == XML_ELEMENT_NODE
                    && strcmp((const char*)schedule->name, "schedule") == 0)
                    {
                        xmlNodePtr relay;

                        for(relay = schedule->children ; relay != NULL ; relay = relay->next)
                        {
                            if(relay->type == XML_ELEMENT_NODE
                            && strcmp((const char*)relay->name, "relay") == 0)
                            {
                                unsigned short nb = atoi((char*)xmlGetProp(relay, (xmlChar*)"number"));

                                if(nb == relay_number)
                                {
                                    xmlNodePtr tasks;

                                    for(tasks = relay->children ; tasks != NULL ; tasks = tasks->children)
                                    {
                                        if(tasks->type == XML_ELEMENT_NODE
                                        && strcmp((const char*)tasks->name, "task") == 0
                                        && strcmp((const char*)xmlNodeGetContent(tasks->children), old_name) == 0)
                                        {
                                            //On se trouve dans le noeud correspond à notre tâche
                                            xmlNodePtr infos;

                                            for(infos = tasks->children ; infos != NULL ; infos = infos->next)
                                            {
                                                if(infos->type == XML_ELEMENT_NODE)
                                                {
                                                    char buffer[14];

                                                    if(!strcmp((const char*)infos->name, "name"))
                                                        xmlNodeSetContent(infos, (xmlChar*)task->name);
                                                    else if(!strcmp((const char*)infos->name, "enabled"))
                                                    {
                                                        sprintf(buffer, "%hu", task->enabled);
                                                        xmlNodeSetContent(infos, (xmlChar*)buffer);
                                                    }
                                                    else if(!strcmp((const char*)infos->name, "date"))
                                                        xmlNodeSetContent(infos, (xmlChar*)task->date);
                                                    else if(!strcmp((const char*)infos->name, "weeks"))
                                                    {
                                                        sprintf(buffer, "%hu", task->weeks);
                                                        xmlNodeSetContent(infos, (xmlChar*)buffer);
                                                    }
                                                    else if(!strcmp((const char*)infos->name, "days"))
                                                    {
                                                        unsigned short i;

                                                        buffer[0] = '\0';
                                                        for(i=0 ; i < 7 ; i++)
                                                        {
                                                            char temp[2];

                                                            sprintf(temp, "%hu", task->days[i]);
                                                            strcat(buffer, temp);

                                                            if(i < 6)
                                                                strcat(buffer, ",");
                                                        }

                                                        xmlNodeSetContent(infos, (xmlChar*)buffer);
                                                    }
                                                    else if(!strcmp((const char*)infos->name, "type"))
                                                    {
                                                        sprintf(buffer, "%hu", task->type);
                                                        xmlNodeSetContent(infos, (xmlChar*)buffer);
                                                    }
                                                    else if(!strcmp((const char*)infos->name, "start"))
                                                    {
                                                        sprintf(buffer, "%huh%hu", task->hours[0], task->minutes[0]);
                                                        xmlNodeSetContent(infos, (xmlChar*)buffer);
                                                    }
                                                    else if(!strcmp((const char*)infos->name, "end"))
                                                    {
                                                        sprintf(buffer, "%huh%hu", task->hours[1], task->minutes[1]);
                                                        xmlNodeSetContent(infos, (xmlChar*)buffer);
                                                    }
                                                }
                                            }

                                            xmlDocFormatDump(file, data->doc, 1);
                                            fclose(file);
                                            unlockMutex(mutex);
                                            return 1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        xmlDocFormatDump(file, data->doc, 1);
        fclose(file);
        unlockMutex(mutex);
    }

    return 0;
}

/**
 * Suppression de l'élément de liste chaînée situé à l'indice donné.
 */
void deleteTaskByName(Tasks **tasks, const char *name, pthread_mutex_t *mutex)
{
    Tasks *temp = *tasks;
    unsigned short index;

    for(index = 0; temp != NULL; index++, temp = temp->next)
    {
        if(strcmp(temp->element.name, name) == 0)
        {
            deleteTask(tasks, index, mutex);
            break;
        }
    }
}

/**
 * Renvoie un pointeur sur le dernier élément alloué de la liste chaînée d'appareils.
 */
Tasks *getLastTask(Tasks **tasks)
{
    if(*tasks != NULL)
    {
        if((*tasks)->next != NULL)
            return getLastTask(&(*tasks)->next);
        else
            return *tasks;
    }
    else
        return NULL;
}

Tasks *getTaskByName(Tasks *tasks, const char *name)
{
    while(tasks != NULL)
    {
        if(strcmp(tasks->element.name, name) == 0)
            return tasks;
        tasks = tasks->next;
    }

    return NULL;
}

/**
 * Renvoie un pointeur sur l'élément de liste chaînée situé à l'indice donné.
 */
Tasks *getTask(Tasks *tasks, unsigned short index)
{
    unsigned short i;

    for(i=0 ; i < index ; i++)
        tasks = tasks->next;

    return tasks;
}

/**
 * Renvoie le nombre d'éléments contenus dans la liste chaînée d'appareils.
 */
unsigned short countTasks(Tasks *tasks)
{
    unsigned short i = 0;

    while(tasks != NULL)
    {
        i++;
        tasks = tasks->next;
    }

    return i;
}

/**
 * Libère la mémoire allouée pour la liste chaînée d'appareils.
 */
void freeTasks(Tasks **tasks, pthread_mutex_t *mutex)
{
    while(*tasks != NULL)
    {
        Tasks *temp = (*tasks)->next;
        free(*tasks);
        *tasks = temp;
    }
}
