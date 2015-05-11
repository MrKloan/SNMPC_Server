#ifndef SNMP_H_INCLUDED
#define SNMP_H_INCLUDED

SNMPRequest *InitRequest(Device*, COMMUNITIES);
char **SendRequest(SNMPRequest**, const char*, char*, char**, unsigned short *);
void CloseRequest(SNMPRequest**);

#endif // SNMP_H_INCLUDED
