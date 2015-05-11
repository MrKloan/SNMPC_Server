#ifndef SNMPC_H
#define SNMPC_H

///Gestion des données XML & listes chaînées
void initSNMPC(SNMPC*);
unsigned short loadConfig(SNMPC*, XmlData*);

unsigned short initUsers(XmlData*, Users**, pthread_mutex_t*);
unsigned short loadUsersFile(XmlData*);
void loadUsersData(XmlData*, Users**, pthread_mutex_t*);

unsigned short initDevices(XmlData*, Devices**, pthread_mutex_t*, pthread_mutex_t*);
unsigned short loadDevicesFile(XmlData*);
unsigned short loadDevicesData(XmlData*, Devices**, pthread_mutex_t*, pthread_mutex_t*);

unsigned short initServer(Server*);
unsigned short initScheduler(SNMPC*);

void freeSNMPC(SNMPC*);

///Gestion des mutex & conditions
int lockMutex(pthread_mutex_t*);
int unlockMutex(pthread_mutex_t*);
int lockCond(pthread_cond_t*, pthread_mutex_t*);
int unlockCond(pthread_cond_t*);

///Gestion des threads
void initThreads(SNMPC*);
void waitThread(pthread_t);
void waitThreads(SNMPC*);

///Gestion SNMP
unsigned short initSNMP(void);

///Chargement de la matrice
void initMatrix(SNMPC *);

unsigned short updateVisualization(SNMPC*, unsigned short);
unsigned short updatePort(SNMPC*, unsigned short);

#endif // SNMPC_H
