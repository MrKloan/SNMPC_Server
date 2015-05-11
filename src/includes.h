#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#define __USE_XOPEN //for strptime() in time.h
#include <time.h>
#include <string.h>
#include <regex.h>
#include <pthread.h>
#include <signal.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#ifdef WIN32
    #include <windows.h>
    #include <winsock2.h>
#elif defined (linux)
    #include <errno.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
#else
    #error "SNMPC is only developped for Linux & Windows platforms"
#endif

#include "defines.h"
#include "structures.h"
#include "sha1.h"
#include "utils.h"
#include "socket.h"
#include "packet.h"
#include "queue.h"
#include "snmp.h"
#include "users.h"
#include "task.h"
#include "scheduler.h"
#include "devices.h"
#include "clients.h"
#include "server.h"
#include "snmpc.h"
#include "shell.h"
#include "visualization.h"

#endif // MAIN_H
