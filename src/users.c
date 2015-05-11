#include "includes.h"

/**
 * Ajout d'un nouvel élément à la fin de la liste chaînée d'utilisateurs.
 */
void addUser(Users **users, const char *username, const char *password, pthread_mutex_t *mutex)
{
    Users *last = getLastUser(users);
    Users *temp;

    lockMutex(mutex);

    temp = malloc(sizeof(Users));
    strcpy(temp->element.username, username);
    strcpy(temp->element.password, password);
    temp->next = NULL;

    if(last == NULL)
        *users = temp;
    else
        last->next = temp;

    unlockMutex(mutex);
}

/**
 * Ajout d'un nouvel utilisateur au fichier de données XML.
 */
unsigned short addUserToXml(XmlData *data, const char *username, const char *password, pthread_mutex_t *mutex)
{
    FILE *file;
    xmlNodePtr user;

    if((file = fopen(data->path, "wb")))
    {
        lockMutex(mutex);

        //Initialisation du contenu du nouveau noeud <user>
        if((user = xmlNewNode(NULL, (xmlChar*)"user")) == NULL)
            return 0;
        if(xmlNewTextChild(user, NULL, (xmlChar*)"username", (xmlChar*)username) == NULL)
        {
            xmlFreeNode(user);
            return 0;
        }
        if(xmlNewTextChild(user, NULL, (xmlChar*)"password", (xmlChar*)password) == NULL)
        {
            xmlFreeNode(user);
            return 0;
        }

        //Ajout du noeud <user> en fin de fichier
        if(xmlAddChild(data->root, user) == NULL)
        {
            xmlFreeNode(user);
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

/**
 * Suppression de l'élément de liste chaînée situé à l'indice donné.
 */
void deleteUser(Users **users, unsigned short index, pthread_mutex_t *mutex)
{
    if(index > 0)
    {
        Users *prev = getUser(*users, index-1);
        Users *next = getUser(*users, index+1);

        lockMutex(mutex);

        free(getUser(*users, index));
        prev->next = next;
    }
    //Si on veut supprimer le premier utilisateur
    else
    {
        Users *next = (*users)->next;

        lockMutex(mutex);

        free(*users);
        *users = next;
    }

    unlockMutex(mutex);
}

/**
 * Renvoie l'indice de l'utilisateur dans la liste en cas de succès, -1 sinon.
 */
short deleteUserFromXml(XmlData *data, const char *username, const char *password, pthread_mutex_t *mutex)
{
    FILE *file;
    unsigned short index = 0, found = 0;
    xmlNodePtr children;

    lockMutex(mutex);

    //Lecture des éléments XML
    for(children = data->root->children, index = 0 ; children != NULL ; children = children->next, index++)
    {
        //S'il s'agit du bon utilisateur
        if(children->type == XML_ELEMENT_NODE
        && strcmp((const char*)children->name, "user") == 0
        && strcmp((const char*)xmlNodeGetContent(children->children), username) == 0
        && strcmp((const char*)xmlNodeGetContent(children->children->next), password) == 0)
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

    //Si l'utilisateur a été trouvé et supprimé, on renvoie son index
    if(found)
        return index;
    else
        return -1;
}

/**
 * Changement de mot de passe d'un utilisateur dans le fichier de données XML.
 */
short changeUserPassFromXml(XmlData *data, const char *username, const char *oldpassword, const char *newpassword, pthread_mutex_t *mutex)
{
    FILE *file;
    unsigned short index = 0, found = 0;
    xmlNodePtr children;

    lockMutex(mutex);

    //Lecture des éléments XML
    for(children = data->root->children, index = 0 ; children != NULL ; children = children->next, index++)
    {
        //S'il s'agit du bon utilisateur
        if(children->type == XML_ELEMENT_NODE
        && strcmp((const char*)children->name, "user") == 0
        && strcmp((const char*)xmlNodeGetContent(children->children), username) == 0
        && strcmp((const char*)xmlNodeGetContent(children->children->next), oldpassword) == 0)
        {
            //Les modifications ne sont effectuées qu'à condition de pouvoir écrire dans le fichier
            if((file = fopen(data->path, "wb")))
            {
                found = 1;

                xmlNodeSetContent(children->children->next, (xmlChar *)newpassword);
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
 * Renvoie un pointeur sur le dernier élément alloué de la liste chaînée d'utilisateurs.
 */
Users *getLastUser(Users **users)
{
    if(*users != NULL)
    {
        if((*users)->next != NULL)
            return getLastUser(&(*users)->next);
        else
            return *users;
    }
    else
        return NULL;
}

/**
 * Renvoie un pointeur sur l'élément de liste chaînée situé à l'indice donné.
 */
Users *getUser(Users *users, unsigned short index)
{
    unsigned short i;

    for(i=0 ; i < index ; i++)
        users = users->next;

    return users;
}

/**
 * Renvoie 1 si l'utilisateur existe, 0 sinon.
 */
Users *getUserByName(Users *users, const char *username)
{
    unsigned short i, count = countUsers(users);

    for(i=0 ; i < count ; i++)
    {
        User user = getUser(users, i)->element;

        if(strcmp(user.username, username) == 0)
            return getUser(users, i);
    }

    return NULL;
}

/**
 * Renvoie le nombre d'éléments contenus dans la liste chaînée d'utilisateurs.
 */
unsigned short countUsers(Users *users)
{
    unsigned short i = 0;

    while(users != NULL)
    {
        i++;
        users = users->next;
    }

    return i;
}

/**
 * Renvoie 1 si l'utilisateur existe, 0 sinon.
 */
unsigned short userExists(Users *users, const char *username)
{
    unsigned short i, count = countUsers(users);

    for(i=0 ; i < count ; i++)
    {
        User user = getUser(users, i)->element;

        if(strcmp(user.username, username) == 0)
            return 1;
    }

    return 0;
}

/**
 * Libère la mémoire allouée pour la liste chaînée d'utilisateurs.
 */
void freeUsers(Users **users, pthread_mutex_t *mutex)
{
    while(*users != NULL)
    {
        Users *temp = (*users)->next;
        free(*users);
        *users = temp;
    }
}

/**
 * Affiche le contenu de la liste chaînée d'utilisateurs sur stdout.
 */
void printUsers(Users *users)
{
    while(users != NULL)
    {
        printf("%s : %s\n", users->element.username, users->element.password);
        users = users->next;
    }
}
