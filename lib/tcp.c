#include "tcp.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

/*######################
# Variable definitions #
######################*/
const int LISTEN_QUEUE = 16;

/*######################
# Function definitions #
######################*/
int start_server(int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int handle, client_size = sizeof(struct sockaddr);

    struct sockaddr_in addr, client;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t) port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

    // Bind server socket to the given port
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0)
    {
        fprintf(stderr, "Could not bind to %d:%d!\n", addr.sin_addr.s_addr, port);
        return 1;
    }

    // Start listening on the provided port
    if (listen(sock, LISTEN_QUEUE) < 0)
    {
        fprintf(stderr, "Failed to listen for incoming connections!\n");
        return 1;
    }

    // Wait for incoming connections
    handle = accept(sock, (struct sockaddr *) &client, (socklen_t *) &client_size);
    if (handle < 0)
    {
        fprintf(stderr, "Failed to accept incoming connection!\n");
        return 1;
    }



    return 0;
}
