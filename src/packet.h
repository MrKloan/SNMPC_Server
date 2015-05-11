#ifndef PACKET_H
#define PACKET_H

char *formatPacket(unsigned short, ...);
void cipherPacket(SNMPC *, char **);
void uncipherPacket(SNMPC *, char **);

#endif // PACKET_H
