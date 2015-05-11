#include "includes.h"

void *visualizerHandler(void *data)
{
    Visualizer *visualizer = (Visualizer *) data;

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while(visualizer->visualizing)
    {
        Devices *temp = visualizer->device;

        while(temp != NULL)
        {
            SNMPRequest *request;
            unsigned short i, size;
            char **values = NULL;
            char buffer[SOCKET_BUFFER];

            if(temp->element.disconnected)
            {
                temp = temp->next;
                continue;
            }

            for(i = 0; i < 9; i++)
            {
                request = InitRequest(&temp->element, COMMUNITY_PUBLIC);
                if(i == 0)
                    sprintf(buffer, "digitalInput");
                else
                    sprintf(buffer, "relay%hu", i);

                values = SendRequest(&request, buffer, NULL, NULL, &size);

                if(values != NULL)
                {
                    char *packet = formatPacket(4, "visualization", temp->element.name, buffer, values[0]);

                    puts(packet);
                    pushQueue(&visualizer->client->sender_queue, packet, &visualizer->client->sender_lock);
                    free(packet);
                }
                else //if values == NULL
                {
                    char *packet = formatPacket(2, visualizer->device->element.name, "disconnected");

                    pushQueue(&visualizer->client->sender_queue, packet, &visualizer->client->sender_lock);
                    unlockCond(&visualizer->client->notifier);
                    free(packet);

                    break;
                }
            }
            unlockCond(&visualizer->client->notifier);
            temp = temp->next;
        }
        snmpc_sleep(*visualizer->visualization_rate);
    }
    visualizer->client->visualizer = NULL;
    free(visualizer);

    return NULL;
}
