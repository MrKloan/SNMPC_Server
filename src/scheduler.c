#include "includes.h"

void *scheduler(void *data)
{
    SNMPC *snmpc = (SNMPC*)data;

    while(snmpc->running && snmpc->scheduler.running)
    {
        Devices *devices = snmpc->devices;

        while(devices != NULL)
        {
            unsigned short i;
            Device *device = &devices->element;
            time_t timestamp = time(NULL);
            struct tm currentTime = *localtime(&timestamp);
            currentTime.tm_year += 1900;
            currentTime.tm_mon += 1;

            //printf("Handling tasks for device %s...\n", device->name);

            for(i=0 ; i < 8 ; i++)
            {
                Tasks *tasks = device->tasks[i];

                while(tasks != NULL)
                {
                    Task *task = &tasks->element;

                    if(task->enabled)
                    {
                        //Si repeat, vérification des semaines
                        if(task->repeat)
                        {
                            //timestamp de date d'origine + x*timestamp de temps de semaine
                            //si compris dans fourchette timestamp de début de semaine actuelle et timestamp de fin de semaine actuelle, OK
                            struct tm src;
                            time_t srcTimestamp;
                            unsigned short i;
                            unsigned short nbWeeks = 52/task->weeks;

                            strptime(task->date, "%d/%m/%Y", &src);
                            srcTimestamp = mktime(&src);

                            for(i=0 ; i < nbWeeks ; i++)
                            {
                                time_t temp = srcTimestamp + (i*task->weeks)*TIMESTAMP_WEEK;
                                time_t start = getWeekStart(&timestamp);
                                time_t end = getWeekEnd(&timestamp);

                                if(temp >= start && temp <= end)
                                {
                                    unsigned short j;

                                    for(j=0 ; j < 7 ; j++)
                                    {
                                        short day = currentTime.tm_wday-1;
                                        if(day == -1)
                                            day = 6;

                                        //Si nous sommes dans l'un des jours actifs de la tâche
                                        if(j == day && task->days[j])
                                        {
                                            //On vérifie heures & minutes de début de tâche
                                            if(task->hours[0] == currentTime.tm_hour && task->minutes[0] == currentTime.tm_min && !task->processing)
                                            {
                                                TaskHandler *handle = malloc(sizeof(TaskHandler));
                                                handle->server = &snmpc->server;
                                                handle->device = device;
                                                handle->task = task;
                                                handle->relay = i+1;

                                                task->processing = 1;

                                                pthread_create(&handle->handler, NULL, taskHandler, (void*)handle);

                                            }
                                            //On vérifie heures & minutes de fin de tâche
                                            else if(task->hours[1] == currentTime.tm_hour && task->minutes[1] == currentTime.tm_min && task->processing)
                                            {
                                                TaskHandler *handle = malloc(sizeof(TaskHandler));
                                                handle->server = &snmpc->server;
                                                handle->device = device;
                                                handle->task = task;
                                                handle->relay = i+1;

                                                task->processing = 0;

                                                pthread_create(&handle->handler, NULL, taskHandler, (void*)handle);
                                            }
                                            else if(task->type == TASK_PULSE && task->processing && task->hours[0] == currentTime.tm_hour && task->minutes[0] == currentTime.tm_min+1)
                                                task->processing = 0;
                                        }
                                    }
                                }
                            }
                        }
                        //Sinon, on compare la date actuelle
                        else
                        {
                            char buffer[11];
                            sprintf(buffer, "%d/%d/%d", currentTime.tm_mday, currentTime.tm_mon, currentTime.tm_year);

                            //Si on est à la bonne date
                            if(!strcmp(task->date, buffer))
                            {
                                //On vérifie heures & minutes de début de tâche
                                if(task->hours[0] == currentTime.tm_hour && task->minutes[0] == currentTime.tm_min && !task->processing)
                                {
                                    TaskHandler *handle = malloc(sizeof(TaskHandler));
                                    handle->server = &snmpc->server;
                                    handle->device = device;
                                    handle->task = task;
                                    handle->relay = i+1;

                                    task->processing = 1;

                                    pthread_create(&handle->handler, NULL, taskHandler, (void*)handle);

                                }
                                //On vérifie heures & minutes de fin de tâche
                                else if(task->hours[1] == currentTime.tm_hour && task->minutes[1] == currentTime.tm_min && task->processing)
                                {
                                    TaskHandler *handle = malloc(sizeof(TaskHandler));
                                    handle->server = &snmpc->server;
                                    handle->device = device;
                                    handle->task = task;
                                    handle->relay = i+1;

                                    task->processing = 0;

                                    pthread_create(&handle->handler, NULL, taskHandler, (void*)handle);
                                }
                                else if(task->type == TASK_PULSE && task->processing && task->hours[0] == currentTime.tm_hour && task->minutes[0] == currentTime.tm_min+1)
                                    task->processing = 0;
                            }
                        }
                    }

                    tasks = tasks->next;
                }
            }

            devices = devices->next;
        }

        snmpc_sleep(SCHEDULER_TIMER);
    }

    return NULL;
}

void *taskHandler(void *data)
{
    TaskHandler *handle = (TaskHandler*)data;

    SNMPRequest *request;
    char **values = NULL, *buffer[2] = {NULL};
    unsigned short size = 0;

    request = InitRequest(handle->device, COMMUNITY_PRIVATE);

    switch(handle->task->type)
    {
        case TASK_ONOFF:
            buffer[0] = malloc(7*sizeof(char));
            sprintf(buffer[0], "relay%hu", handle->relay);

            buffer[1] = malloc(2*sizeof(char));
            sprintf(buffer[1], "%hu", !handle->device->relays_state[handle->relay-1]);

            values = SendRequest(&request, buffer[0], "i", &buffer[1], &size);

            if(values != NULL && size > 0)
            {
                char *packet;
                unsigned short i;
                Clients *clients;

                handle->device->relays_state[handle->relay-1] = (unsigned short)atoi(values[0]);
                packet = formatPacket(3, handle->device->name, buffer[0], buffer[1]);

                clients = handle->server->clients;
                while(clients != NULL)
                {
                    pushQueue(&clients->element.sender_queue, packet, &clients->element.sender_lock);
                    unlockCond(&clients->element.notifier);

                    clients = clients->next;
                }

                free(packet);
                for(i=0 ; i < size ; i++)
                    free(values[i]);
                free(values);
            }

            free(buffer[0]);
            free(buffer[1]);
            break;

        case TASK_PULSE:
            buffer[0] = malloc(7*sizeof(char));
            sprintf(buffer[0], "pulse%hu", handle->relay);

            buffer[1] = malloc(2*sizeof(char));
            sprintf(buffer[1], "%hu", !handle->device->relays_state[handle->relay-1]);

            values = SendRequest(&request, buffer[0], "i", &buffer[1], &size);

            if(values != NULL && size > 0)
            {
                char *packet;
                unsigned short i;
                Clients *clients;

                packet = formatPacket(3, handle->device->name, buffer[0], buffer[1]);

                clients = handle->server->clients;
                while(clients != NULL)
                {
                    pushQueue(&clients->element.sender_queue, packet, &clients->element.sender_lock);
                    unlockCond(&clients->element.notifier);

                    clients = clients->next;
                }

                free(packet);
                for(i=0 ; i < size ; i++)
                    free(values[i]);
                free(values);
            }

            free(buffer[0]);
            free(buffer[1]);
            break;
    }

    CloseRequest(&request);

    free(handle);
    return NULL;
}
