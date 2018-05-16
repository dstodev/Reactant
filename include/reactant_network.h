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
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include "exsrc_minIni.h"
#include "reactant_util.h"


#define CONF_INI "cfg.ini"

extern const int LISTEN_QUEUE;
extern const int TABLE_SIZE;

typedef struct _core_t
{
    struct sockaddr_in * addr;
    int sock;
    int node_id;
    char key[33];
    char iv[17];

} core_t;

typedef core_t node_t;

typedef struct _channel_t
{
    // char * name; // Don't need name, it's the key of containing hash_data_t
    int size;
    node_t * nodes;

} channel_t;

typedef struct _subscription_t
{
    char channel[250];
    void (*callback)(char *);

} subscription_t;

typedef struct _subpack_t
{
    core_t * core;
    int size;
    subscription_t * subs;
    pthread_mutex_t * lock;

} subpack_t;

typedef struct _fds
{
    fd_set set;
    int max_fd;

} fds;

unsigned long get_interface();
int start_discovery_server(int port);
int discover_server(int port);
int start_core_server(int port, char * key, char * iv);
int start_node_client(core_t * core, unsigned int id, char * ip, int port, char * key, char * iv);
int stop_node_client(core_t * core);

int publish(core_t * core, char * channel, char * message);
int subscribe(core_t * core, char * channel, void (*callback)(char *));

#endif // REACTANT_NETWORK_H
