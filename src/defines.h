#ifndef DEFINES_H
#define DEFINES_H

#if defined (linux)
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket(s) close(s)
    #define snmpc_sleep(s) usleep(s*1000000)
#elif defined WIN32
    #define snmpc_sleep(s) Sleep(s)
#endif

#define SALT ""
#define PEPPER ""

#define SHELL_NAME "SNMPC"
#define PACKET_DELIMITER ";"
#define PACKET_END "|"

#define SERVER_PORT 5903
#define SERVER_WAIT_QUEUE 5
#define REFRESH_RATE 5

#define SCHEDULER_TIMER 1
#define TIMESTAMP_DAY 86400
#define TIMESTAMP_WEEK 604800

#define SOCKET_BUFFER 5120
#define SHELL_BUFFER 50
#define SNMP_BUFFER 50

#define DATA_FOLDER "./data"
#define FILE_USERS "data/users.xml"
#define FILE_DEVICES "data/devices.xml"
#define FILE_CONFIG "data/config.xml"
#define FILE_MIB "data/TCW.mib"

#define XML_HEADER "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
#define SNMP_REQUEST_HEAD "Teracom::"
#define SNMP_REQUEST_TAIL ".0"

typedef enum {
    DATA_USERS,
    DATA_DEVICES,
    DATA_CONFIG
} DATA;

typedef enum {
    LOCK_USERS,
    LOCK_DEVICES,
    LOCK_TASKS
} LOCK;

typedef enum {
    COMMUNITY_PUBLIC,
    COMMUNITY_PRIVATE
} COMMUNITIES;

typedef enum {
    PACKET_OK,
    PACKET_FAIL
} PACKET_STATE;

enum {
    IPPORT_ADDR,
    IP_ADDR,
    MAC_ADDR
};

enum {
    TASK_ONOFF,
    TASK_PULSE
};

#endif // DEFINES_H
