#ifndef _CONFIG_GUARD
#define _CONFIG_GUARD

#include <arpa/inet.h>

#define PORT "3490"
#define SERV_PORT "3491"
#define BACKLOG 10
#define TIMEOUT 20
#define LOCK_TIMEOUT 2
#define BACKUP_TIMEOUT 5

#define MAXSIZE 100
#define BUFSIZE 1024

#define INIT -1
#define STATE -2
#define BACKUP -3
#define BACK_IP -4

#endif
