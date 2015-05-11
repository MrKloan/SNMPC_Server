#ifndef USERS_H
#define USERS_H

void addUser(Users**, const char*, const char*, pthread_mutex_t*);
unsigned short addUserToXml(XmlData*, const char*, const char*, pthread_mutex_t*);

void deleteUser(Users**, unsigned short, pthread_mutex_t*);
short deleteUserFromXml(XmlData*, const char*, const char*, pthread_mutex_t*);

short changeUserPassFromXml(XmlData*, const char*, const char*, const char*, pthread_mutex_t*);

Users *getLastUser(Users**);
Users *getUser(Users*, unsigned short);
Users *getUserByName(Users*, const char*);
unsigned short countUsers(Users*);
unsigned short userExists(Users*, const char*);

void freeUsers(Users**, pthread_mutex_t*);
void printUsers(Users*);

#endif // USERS_H
