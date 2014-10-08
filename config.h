#ifndef _CONFIG_GUARD
#define _CONFIG_GUARD

#include <arpa/inet.h>

#define PORT "3490"
#define PORT_START 3490
#define PORT_END   3500

#define BACKLOG 10
#define INIT_TIMEOUT 20
#define LOCK_TIMEOUT 2
#define BACKUP_TIMEOUT 3
#define CONNECT_TIMEOUT 3
#define NB_BACKUP 1

#define MAXSIZE 100
#define BUFSIZE 1024

#define INIT_CLIENT     -1
#define UPDATE_STATE    -2
#define CREATE_SERVER   -3
#define NEW_SERVER      -4
#define PLAYER_MOVED    -5
#define STATE_ACQUIRED  -6
#define NOT_OWNER       -7

#define MOVE_PLAYER     "move_player"
#define REQUEST_STATE   "req_state"

#endif
