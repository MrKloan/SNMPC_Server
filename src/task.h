#ifndef TASK_H_INCLUDED
#define TASK_H_INCLUDED

void addTask(Tasks **, const char *,const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, pthread_mutex_t*);
unsigned short addTaskToXml(XmlData *, Task *, Device *, unsigned short, pthread_mutex_t *);

void deleteTask(Tasks**, unsigned short, pthread_mutex_t*);
void deleteTaskByName(Tasks **, const char *, pthread_mutex_t *);
unsigned short deleteTaskFromXml(XmlData *, Task *, Device *, unsigned short, pthread_mutex_t *);

void modifyTask(Task *, const char *,const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, pthread_mutex_t*);
unsigned short modifyTaskFromXml(XmlData *, const char *, Task *, Device *, unsigned short, pthread_mutex_t *);

Tasks *getLastTask(Tasks**);
Tasks *getTask(Tasks*, unsigned short);
Tasks *getTaskByName(Tasks*, const char*);
unsigned short countTasks(Tasks*);
unsigned short taskNameExists(Tasks*, const char*);
unsigned short taskIpExists(Tasks*, const char*);

void freeTasks(Tasks**, pthread_mutex_t*);
void printTasks(Tasks*);
char *getTaskData(Task *);
void sendTasksData(SNMPC*, Client*);

#endif // TASK_H_INCLUDED
