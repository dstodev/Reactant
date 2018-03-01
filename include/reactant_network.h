#ifndef REACTANT_NETWORK_H
#define REACTANT_NETWORK_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <netdb.h>

#include "reactant_util.h"


#define S_MUL 1000000

extern const int LISTEN_QUEUE;

typedef struct _core_t
{
    struct sockaddr_in * addr;
    int sock;


} core_t;

unsigned long get_interface();
int start_discovery_server(int port);
int discover_server(int port);
int start_core_server(int port);
int start_node_client(core_t * core, char * ip, int port);
int stop_node_client(core_t * core);

#endif // REACTANT_NETWORK_H
