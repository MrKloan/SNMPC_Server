#ifndef SHELL_H
#define SHELL_H

///Handlers
void *shellHandler(void*);
unsigned short shellLogin(SNMPC*);
void shellInput(SNMPC*);
void commandHandler(SNMPC*);
void freeCommandArgs(char***, unsigned short);

///Commands
void commandExit(SNMPC*, char***, unsigned short);
void commandLogout(SNMPC*);
void commandHelp(SNMPC*);
void commandHostname(SNMPC*, char**, unsigned short);

void commandAdduser(SNMPC*, char**, unsigned short);
void commandDeluser(SNMPC*, char**, unsigned short);

void commandNewdevice(SNMPC*, char**, unsigned short);
void commandDeldevice(SNMPC*, char**, unsigned short);

void commandList(SNMPC*, char**, unsigned short);
void commandEcho(SNMPC*, char**, unsigned short);
void commandGnu(SNMPC*, char**, unsigned short);

void commandRequest(SNMPC*, char**, unsigned short);
void commandPort(SNMPC*, char**, unsigned short);

#endif // SHELL_H
