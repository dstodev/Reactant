#include "reactant_network.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>


const int LISTEN_QUEUE = 16;


int start_server(int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int handle = 0;
    int client_size = sizeof(struct sockaddr);

    struct sockaddr_in server_addr, client_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t) port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    // Bind server socket to the given port
    if (bind(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    {
        fprintf(stderr, "Could not bind to %d:%d!\n", server_addr.sin_addr.s_addr, port);
        return 1;
    }

    // Start listening on the provided port
    if (listen(sock, LISTEN_QUEUE) < 0)
    {
        fprintf(stderr, "Failed to listen for incoming connections!\n");
        return 1;
    }

    // Wait for incoming connections
    handle = accept(sock, (struct sockaddr *) &client_addr, (socklen_t *) &client_size);
    if (handle < 0)
    {
        fprintf(stderr, "Failed to accept incoming connection!\n");
        return 1;
    }


    return 0;
}
