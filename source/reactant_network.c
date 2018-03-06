#include "reactant_network.h"

// Global constant definitions
const int LISTEN_QUEUE = 16;

// Private helper functions
static int send_to_core(core_t * core, char * message, int size);


unsigned long get_interface()
{
    struct ifaddrs *if_addr, *ifa;
    struct sockaddr_in *address, *subnet, broadcast;
    char a_name[16], s_name[16];
    int i = 0, j = 0;

    fprintf(stderr, "Found interfaces:\n");

    getifaddrs (&if_addr);
    for (ifa = if_addr; ifa; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            address = (struct sockaddr_in *) ifa->ifa_addr;
            subnet = (struct sockaddr_in *) ifa->ifa_netmask;

            strcpy(a_name, inet_ntoa(address->sin_addr));
            strcpy(s_name, inet_ntoa(subnet->sin_addr));

            fprintf(stderr, "%d. Interface: %5s    Address: %15s    Subnet: %15s\n", ++i, ifa->ifa_name, a_name, s_name);
        }
    }

    fprintf(stderr, "Select an interface: ");
    j = i;
    while(scanf("%d", &i) < 0 || i <= 0 || i > j);

    for (ifa = if_addr; ifa && i; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr->sa_family == AF_INET && !(--i))
        {
            break;
        }
    }

    address = (struct sockaddr_in *) ifa->ifa_addr;
    subnet = (struct sockaddr_in *) ifa->ifa_netmask;

    broadcast.sin_addr.s_addr = address->sin_addr.s_addr | ~(subnet->sin_addr.s_addr);
    fprintf(stderr, "Chosen interface: %5s    Broadcast: %s\n", ifa->ifa_name, inet_ntoa(broadcast.sin_addr));

    freeifaddrs(if_addr);
    return broadcast.sin_addr.s_addr;
}

int start_discovery_server(int port)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int bytes = 0;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t) port);
    server_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    struct timespec delay;
    delay.tv_sec = 1;
    delay.tv_nsec = 0;

    int broadcast_permission = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_permission, sizeof(broadcast_permission)) < 0)
    {
        fprintf(stderr, "Could not set socket options!\n");
        close(sock);
        return 1;
    }

    while (1)
    {
        // Wait one second
        nanosleep(&delay, NULL);

        // Broadcast message
        if ((bytes = sendto(sock, "Discovery Broadcast", 19, 0, (struct sockaddr *) &server_addr, sizeof(struct sockaddr))) < 0)
        {
            fprintf(stderr, "Failed to broadcast message!\n");
            break;
        }
    }

    close(sock);
    return 1;
}

int discover_server(int port)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int bytes = 0;
    char message[BUFFER_DEPTH];

    struct sockaddr_in client_addr, server_address;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons((uint16_t) port);
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(client_addr.sin_zero, 0, sizeof(client_addr.sin_zero));

    socklen_t address_length = sizeof(server_address);

    int broadcast_permission = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_permission, sizeof(broadcast_permission)) < 0)
    {
        fprintf(stderr, "Could not set socket options!\n");
        close(sock);
        return 1;
    }

    if (bind(sock, (struct sockaddr *) &client_addr, sizeof(client_addr)) < 0)
    {
        fprintf(stderr, "Could not bind to %d:%d!\n", ntohl(client_addr.sin_addr.s_addr), ntohs(port));
        return 1;
    }

    while ((bytes = recvfrom(sock, message, BUFFER_DEPTH, 0, (struct sockaddr *) &server_address, &address_length)) > 0)
    {
        message[bytes] = '\0';
        fprintf(stderr, "%s\n", message);
    }

    close(sock);

    return 0;
}

int start_core_server(int port)
{
    message_t message;

    struct AES_ctx context;
    const char * key = "01234567012345670123456701234567";  // 32 bytes
    const char * iv = "0123456701234567";   // 16 bytes

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int handle = 0;
    int client_size = sizeof(struct sockaddr);

    char buffer[256];
    int bytes = 0;

    char channel[250];
    char payload[250];

    struct sockaddr_in server_addr, client_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t) port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    // Bind server socket to the given port
    if (bind(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    {
        fprintf(stderr, "%s %d:%d!\n", "Could not bind to", ntohl(server_addr.sin_addr.s_addr), ntohs(port));
        close(sock);
        return 1;
    }

    // Start listening on the provided port
    if (listen(sock, LISTEN_QUEUE) < 0)
    {
        fprintf(stderr, "%s\n", "Could not listen for incoming connections!");
        close(sock);
        return 1;
    }

    while(1)
    {
        // Clear buffers
        memset(buffer, 0, sizeof(buffer));
        memset(channel, 0, sizeof(channel));
        memset(payload, 0, sizeof(payload));

        // Wait for incoming connections
        handle = accept(sock, (struct sockaddr *) &client_addr, (socklen_t *) &client_size);
        if (handle < 0)
        {
            fprintf(stderr, "%s\n", "Failed to accept incoming connection!");
            close(sock);
            return 1;
        }

        // Read incoming message
        bytes = read(handle, buffer, sizeof(buffer));
        if (bytes != sizeof(buffer))
        {
            fprintf(stderr, "%s %d!\n", "Invalid initial read, rval:", bytes);
            close(sock);
            return 1;
        }

        // Decrypt message (AES256)
        AES_init_ctx_iv(&context, (const uint8_t *) key, (const uint8_t *) iv);
        AES_CBC_decrypt_buffer(&context, (uint8_t *) buffer, 256);

        // Generate message struct from message
        message_initialize(&message);
        memcpy(message.message_string, buffer, 256);
        message_unpack(&message);

        switch (message.source_id)
        {
            case 0:
                // Publish message
                strcpy(channel, message.payload);

                // Read payload message
                bytes = read(handle, buffer, sizeof(buffer));
                if (bytes != sizeof(buffer))
                {
                    fprintf(stderr, "%s %d!\n", "Invalid payload read, rval:", bytes);
                    close(sock);
                    return 1;
                }

                // Decrypt message (AES256)
                AES_init_ctx_iv(&context, (const uint8_t *) key, (const uint8_t *) iv);
                AES_CBC_decrypt_buffer(&context, (uint8_t *) buffer, 256);

                // Generate message struct from message
                message_initialize(&message);
                memcpy(message.message_string, buffer, 256);
                message_unpack(&message);

                strcpy(payload, message.payload);

                fprintf(stderr, "Publishing \"%s\" to \"%s\"\n", payload, channel);

                break;

            default:
                // Subscribe message
                ;
        }
    }

    return 0;
}

int start_node_client(core_t * core, char * ip, int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in server_addr;

    if (core && ip)
    {
        // Ignore SIGPIPE signals
        signal(SIGPIPE, SIG_IGN);

        // Clear core struct
        memset(core, 0, sizeof(*core));

        server_addr.sin_family = AF_INET;   // IPv4
        server_addr.sin_port = htons(port); // Port
        inet_pton(AF_INET, ip, &server_addr.sin_addr);  // Convert string represenation of IP address to integer value
        memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

        // Connect to Core device
        if (connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
        {
            // Connection failed
            fprintf(stderr, "%s\n", "Could not connect to server!");
            return 1;
        }
        else
        {
            // Connection succeeded
            core->addr = calloc(1, sizeof(server_addr));
            *core->addr = server_addr;
            core->sock = sock;
        }
    }
    else
    {
        // Invalid parameters
        return 1;
    }

    return 0;
}

int stop_node_client(core_t * core)
{
    if (core)
    {
        // Free address struct
        if (core->addr)
        {
            free(core->addr);
        }

        // Close socket
        if(core->sock)
        {
            close(core->sock);
            core->sock = 0;
        }
    }
    else
    {
        // Invalid parameters
        return 1;
    }

    return 0;
}

int publish(core_t * core, char * channel, char * payload)
{
    message_t message;

    struct AES_ctx context;
    const char * key = "01234567012345670123456701234567";  // 32 bytes
    const char * iv = "0123456701234567";   // 16 bytes

    if (core && channel && payload)
    {
        if (strlen(channel) >= 250)
        {
            fprintf(stderr, "%s\n", "Cannot publish to channel name of length 250 or greater!");
            return 1;
        }

        if (strlen(payload) >= 250)
        {
            // TODO: Allow arbitrary message size
            fprintf(stderr, "%s\n", "Cannot publish message of length 250 or greater!");
            return 1;
        }

        /*
         * Send channel designation message
         */

        // Set up message
        message_initialize(&message);
        message.bytes_remaining = strlen(channel);  // Bytes Remaining
        message.source_id = 0;  // Source ID = 0, ID irrelevant for publish
        strcpy(message.payload, channel);   // Payload

        // Serialize message
        message_pack(&message);
        fprintf(stderr, "%x %x %s\n", message.bytes_remaining, message.source_id, message.payload);

        // Encrypt message (AES256)
        AES_init_ctx_iv(&context, (const uint8_t *) key, (const uint8_t *) iv);
        AES_CBC_encrypt_buffer(&context, (uint8_t *) message.message_string, 256);

        // Send message
        send_to_core(core, message.message_string, 256);

        /*
         * Send payload message
         */

        // Set up message
        message_initialize(&message);
        message.bytes_remaining = strlen(payload);
        message.source_id = 0;
        strcpy(message.payload, payload);

        // Serialize message
        message_pack(&message);

        // Encrypt message (AES256)
        AES_init_ctx_iv(&context, (const uint8_t *) key, (const uint8_t *) iv);
        AES_CBC_encrypt_buffer(&context, (uint8_t *) message.message_string, 256);

        // Send message
        send_to_core(core, message.message_string, 256);
    }
    else
    {
        // Invalid parameters
        return 1;
    }

    return 0;
}

int subscribe(core_t * core, char * channel)
{
    return 0;
}

static int send_to_core(core_t * core, char * message, int size)
{
    if (core && message)
    {
        if (core->sock)
        {
            if (write(core->sock, message, size) < 0)
            {
                switch (errno)
                {
                    case EPIPE:
                        // Connection to core was lost
                        fprintf(stderr, "%s\n", "Connection to the Core has been lost!");
                        break;

                    default:
                        // Unknown
                        fprintf(stderr, "%s\n", "An unknown error occurred while attempting to publish to the Core!");
                }

                return 1;
            }
        }
        else
        {
            // Connection to core was never established
            fprintf(stderr, "%s\n", "Connection to the Core has not yet been established!");
            return 1;
        }
    }
    else
    {
        // Invalid parameters
        return 1;
    }

    return 0;
}
