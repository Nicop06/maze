#ifndef _CONFIG_GUARD
#define _CONFIG_GUARD

#include <climits>
#include <arpa/inet.h>

#define PORT "3490"
#define SERV_PORT "3491"
#define BACKLOG 10
#define TIMEOUT 20
#define BACKUP_TIMEOUT 5

#define MAXSIZE 100
#define BUFSIZE 1024

#define INIT INT_MAX-1
#define STATE INT_MAX-2
#define BACKUP INT_MAX-3
#define BACK_IP INT_MAX-4

#endif
