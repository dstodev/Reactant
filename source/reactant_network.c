#include "reactant_network.h"

// Global constant definitions
const int LISTEN_QUEUE = 16;
const int TABLE_SIZE = 10;

// Private helper functions
static int _send_to_core(core_t * core, char * message, int size)
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
                        fprintf(stderr, "Connection to the Core has been lost!\n");
                        break;

                    default:
                        // Unknown
                        fprintf(stderr, "An unknown error occurred while attempting to publish to the Core!\n");
                }

                return 1;
            }
        }
        else
        {
            // Connection to core was never established
            fprintf(stderr, "Connection to the Core has not yet been established!\n");
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

static int _send_to_node(struct sockaddr_in addr, char * message, int size)
{
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (message)
    {
        // Ignore SIGPIPE signals
        signal(SIGPIPE, SIG_IGN);

        // Connect to Node device
        if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0)
        // Connection failed
        {
            fprintf(stderr, "Could not connect to node [%d]!\n", addr.sin_addr.s_addr);
            return 1;
        }
        else
        // Connection succeeded
        {
            if (write(sock, message, size) < 0)
            {
                switch (errno)
                {
                    case EPIPE:
                        // Connection to node was lost
                        fprintf(stderr, "Connection to Node [%d] has been lost!\n", addr.sin_addr.s_addr);
                        break;

                    default:
                        // Unknown
                        fprintf(stderr, "An unknown error occurred while attempting to write to Node [%d]!\n", addr.sin_addr.s_addr);
                }

                close(sock);
                return 1;
            }

            close(sock);
        }
    }
    else
    {
        // Invalid parameters
        return 1;
    }

    return 0;
}

static uint32_t _hash_channel(void * name)
{
    char * str = (char *) name;
    int hash = 0;

    for (int i = 0; i < strlen(str); ++i)
    {
        hash += str[i];
        hash ^= str[i];
    }

    return (hash % 10);

}

static uint8_t _compare_channel(void * lhs, void * rhs)
{
    return (strcmp((char *) lhs, (char *) rhs) == 0);
}

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
    int rval;
    message_t message;

    hash_table_t table;
    ht_construct(&table, TABLE_SIZE, sizeof(char *), sizeof(channel_t), &_hash_channel, &_compare_channel);
    //           table   10          key size        value size           hash function   compare function

    hash_data_t search;
    channel_t  * channel_list;

    struct AES_ctx context;
    const char * key = "01234567012345670123456701234567";  // Test key (32 bytes)
    const char * iv = "0123456701234567";   // Test IV (16 bytes)

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int handle = 0;
    int client_size = sizeof(struct sockaddr);

    char buffer[256];
    int bytes = 0;

    char channel[250];

    struct sockaddr_in server_addr, client_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t) port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    // Bind server socket to the given port
    if (bind(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    {
        fprintf(stderr, "Could not bind to %d:%d!\n", ntohl(server_addr.sin_addr.s_addr), ntohs(port));
        close(sock);
        return 1;
    }

    // Start listening on the provided port
    if (listen(sock, LISTEN_QUEUE) < 0)
    {
        fprintf(stderr, "Could not listen for incoming connections!\n");
        close(sock);
        return 1;
    }

    while(1)
    {
        // Clear buffers
        memset(buffer, 0, sizeof(buffer));
        memset(channel, 0, sizeof(channel));

        // Wait for incoming connections
        handle = accept(sock, (struct sockaddr *) &client_addr, (socklen_t *) &client_size);
        if (handle < 0)
        {
            fprintf(stderr, "Failed to accept incoming connection!\n");
            close(sock);
            return 1;
        }

        // Read incoming message
        bytes = read(handle, buffer, sizeof(buffer));
        if (bytes != sizeof(buffer))
        {
            fprintf(stderr, "Invalid initial read, rval: [%d]!\n", bytes);
            close(sock);
            return 1;
        }

        message_initialize(&message);
        memcpy(message.message_string, buffer, 256);

        // Decrypt message (AES256)
        AES_init_ctx_iv(&context, (const uint8_t *) key, (const uint8_t *) iv);
        AES_CBC_decrypt_buffer(&context, (uint8_t *) message.message_string, 256);

        // Generate message struct from message
        message_unpack(&message);

        switch (message.source_id)
        {
            case 0:
                // Message is a "Publish" message

                strcpy(channel, message.payload);

                // Read payload message
                bytes = read(handle, buffer, sizeof(buffer));
                if (bytes != sizeof(buffer))
                {
                    fprintf(stderr, "Invalid payload read, rval: [%d]!\n", bytes);
                    close(sock);
                    return 1;
                }

                message_initialize(&message);
                memcpy(message.message_string, buffer, 256);

                // Decrypt message (AES256)
                AES_init_ctx_iv(&context, (const uint8_t *) key, (const uint8_t *) iv);
                AES_CBC_decrypt_buffer(&context, (uint8_t *) message.message_string, 256);

                // Generate message struct from message
                message_unpack(&message);

                fprintf(stderr, "Publishing \"%s\" to \"%s\"\n", message.payload, channel);

                // Find channel in table
                if (ht_search(&table, &search, channel) == HT_DNE)
                // Channel hasn't been created yet; no devices are subscribed to the target channel
                {
                    fprintf(stderr, "Could not find \"%s\" in table!\n", channel);
                }
                else
                // Relay message to all devices subscribed to the target channel
                {
                    channel_list = (channel_t *) search.value;
                    for (int i = 0; i < channel_list->size; ++i)
                    {
                        _send_to_node(channel_list->addresses[i], buffer, 256);
                    }
                }

                break;

            default:
                // Message is a "Subscribe" message

                if ((rval = ht_search(&table, &search, channel)) == HT_DNE)
                // Channel doesn't yet exist in table, create it
                {
                    channel_list = calloc(1, sizeof(channel_t));
                    channel_list->addresses = calloc(1, sizeof(struct sockaddr_in));
                    channel_list->addresses[0] = client_addr;
                    channel_list->size = 1;

                    ht_insert(&table, channel, channel_list);

                    free(channel_list); // TODO: Don't do this?
                }
                else if (rval == SUCCESS)
                // Append address to list of addresses
                {
                    channel_list = (channel_t *) search.value;
                    channel_list->addresses = realloc(channel_list->addresses, channel_list->size + 1);
                    channel_list->addresses[channel_list->size] = client_addr;
                    channel_list->size += 1;
                }
                else
                {
                    fprintf(stderr, "An unknown error occurred when handling Subscription message: [%d]!\n", rval);
                }
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
            fprintf(stderr, "Could not connect to server!\n");
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
            fprintf(stderr, "Cannot publish to channel name of length 250 or greater!\n");
            return 1;
        }

        if (strlen(payload) >= 250)
        {
            // TODO: Allow arbitrary message size
            fprintf(stderr, "Cannot publish message of length 250 or greater!\n");
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
        _send_to_core(core, message.message_string, 256);

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

        message_debug_hex(message.message_string);

        // Encrypt message (AES256)
        AES_init_ctx_iv(&context, (const uint8_t *) key, (const uint8_t *) iv);
        AES_CBC_encrypt_buffer(&context, (uint8_t *) message.message_string, 256);

        fprintf(stderr, "\n");
        message_debug_hex(message.message_string);

        // Send message
        _send_to_core(core, message.message_string, 256);
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
