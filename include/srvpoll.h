#ifndef SRVPOLL_H
#define SRVPOLL_H

#include <poll.h>

#define MAX_CLIENTS 256
#define PORT 8080
#define BUFF_SIZE 4096

typedef enum {
    STATE_NEW,
    STATE_CONNECTED,
    STATE_DISCONNECTED
} state_e

// Structure to hold client state
typedef struct{
    int fd;
    state e_state;
    char buffer[4096]
} clientstate_t;

void int_clients(clientstate_t* states);
int find_free_slot(client_state_t* states);
int find_slot_by_fd(clientstate_t* states, int fd);
#endif