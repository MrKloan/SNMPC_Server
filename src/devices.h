#ifndef DEVICES_H
#define DEVICES_H

void addDevice(Devices**, const char*, const char*, const char*, const char*, pthread_mutex_t*);
unsigned short addDeviceToXml(XmlData*, const char*, const char*, const char*, const char*, pthread_mutex_t*);

void deleteDevice(Devices**, unsigned short, pthread_mutex_t*);
short deleteDeviceFromXml(XmlData*, const char*, pthread_mutex_t*);

short changeDeviceConfigFromXml(XmlData *, const char *, const char *, const char *, const char *, const char *, pthread_mutex_t *);

Devices *getLastDevice(Devices**);
Devices *getDevice(Devices*, unsigned short);
Devices *getDeviceByName(Devices*, const char*);
unsigned short countDevices(Devices*);
unsigned short deviceNameExists(Devices*, const char*);
unsigned short deviceIpExists(Devices*, const char*);

void freeDevices(Devices**, pthread_mutex_t*);
void printDevices(Devices*);
char *getDeviceData(Device *);
void sendDevicesData(SNMPC*, Client*);

#endif // DEVICES_H
