#include "reactant_network.h"

// Global constant definitions
const int LISTEN_QUEUE = 16;
const int TABLE_SIZE = 10;

// Private helper functions
static int _send_to_core(core_t * core, char * message, int size)
{
    int bytes;

    if (core && message)
    {
        if (core->sock)
        {
            if ((bytes = write(core->sock, message, size)) != size)
            {
                switch (errno)
                {
                    case EPIPE:
                        // Connection to Core was lost
                        debug_output("Connection to the Core has been lost!\n");
                        break;

                    default:
                        // Unknown
                        debug_output("An unknown error occurred while attempting to publish to the Core!\n");
                }

                close(core->sock);
                return 1;
            }
        }
        else
        {
            // Connection to Core was never established
            debug_output("Connection to the Core has not yet been established!\n");
            return 1;
        }
    }
    else
    {
        // Invalid parameters
        debug_output("<_send_to_core> Invalid parameter(s)!\n");
        return 1;
    }

    return 0;
}

static int _send_to_node(node_t * node, char * message, int size)
{
    int bytes;

    if (node && message)
    {
        if (node->sock)
        {
            if ((bytes = write(node->sock, message, size)) != size)
            {
                switch (errno)
                {
                    case EPIPE:
                        // Connection to Node was lost
                        debug_output("Connection to Node [%x] has been lost!\n", node->node_id);
                        break;

                    default:
                        // Unknown
                        debug_output("An unknown error occurred while attempting to write to Node [%x]!\n", node->node_id);
                }

                return 1;
            }
        }
        else
        {
            // Connection to Node was never established
            debug_output("Connection to Node [%x] has not yet been established!\n", node->node_id);
            return 1;
        }
    }
    else
    {
        // Invalid parameters
        debug_output("<_send_to_node> Invalid parameter(s)!\n");
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
        hash ^= str[i];
        hash += str[i];
    }

    return (hash % 10);

}

static uint8_t _compare_channel(void * lhs, void * rhs)
{
    return (strcmp((char *) lhs, (char *) rhs) == 0);
}

static void * _subscription_listener(void * _pack)
{
    subpack_t * pack = (subpack_t *) _pack;

    message_t message;
    char buffer[256];
    char channel[250];
    int bytes = 0;
    char found;

    struct AES_ctx context;
    const char * key = "01234567012345670123456701234567";  // Test key (32 bytes)
    const char * iv = "0123456701234567";   // Test IV (16 bytes)

    // Wait for and handle incoming relayed messages
    while (1)
    {
        // Clear buffers
        memset(buffer, 0, sizeof(buffer));
        memset(channel, 0, sizeof(channel));
        found = 0;

        //// GET CHANNEL /////////////////////////////////////////////////////////////////
        if ((bytes = read(pack->core->sock, buffer, sizeof(buffer))) != sizeof(buffer)) //
        {                                                                               //
            debug_output("Invalid channel read, rval: [%d]!\n", bytes);                 //
            continue;                                                                   //
        }                                                                               //
                                                                                        //
        message_initialize(&message);                                                   //
        memcpy(message.message_string, buffer, 256);                                    //
                                                                                        //
        // Decrypt message (AES256)                                                     //
        AES_init_ctx_iv(&context, (const uint8_t *) key, (const uint8_t *) iv);         //
        AES_CBC_decrypt_buffer(&context, (uint8_t *) message.message_string, 256);      //
                                                                                        //
        // Generate message struct from message                                         //
        message_unpack(&message);                                                       //
                                                                                        //
        strcpy(channel, message.payload);                                               //
        //////////////////////////////////////////////////////////////////////////////////

        //// GET PAYLOAD /////////////////////////////////////////////////////////////////
        if ((bytes = read(pack->core->sock, buffer, sizeof(buffer))) != sizeof(buffer)) //
        {                                                                               //
            debug_output("Invalid payload read, rval: [%d]!\n", bytes);                 //
            continue;                                                                   //
        }                                                                               //
                                                                                        //
        message_initialize(&message);                                                   //
        memcpy(message.message_string, buffer, 256);                                    //
                                                                                        //
        // Decrypt message (AES256)                                                     //
        AES_init_ctx_iv(&context, (const uint8_t *) key, (const uint8_t *) iv);         //
        AES_CBC_decrypt_buffer(&context, (uint8_t *) message.message_string, 256);      //
                                                                                        //
        // Generate message struct from message                                         //
        message_unpack(&message);                                                       //
        //////////////////////////////////////////////////////////////////////////////////

        pthread_mutex_lock(pack->lock);

        debug_output("Looking for channel [%s]!\n", channel);

        // Invoke callback function for the received channel
        for (int i = 0; i < pack->size; ++i)
        {
            if (strcmp(pack->subs[i].channel, channel) == 0)
            {
                found = 1;

                pack->subs[i].callback(message.payload);

                break;
            }
        }

        if (!found)
        {
            // TODO: Unsubscribe from channel, this device is not actually subscribed
        }

        pthread_mutex_unlock(pack->lock);
    }

    return NULL;
}

unsigned long get_interface()
{
    struct ifaddrs *if_addr, *ifa;
    struct sockaddr_in *address, *subnet, broadcast;
    char a_name[16], s_name[16];
    int i = 0, j = 0;

    debug_output("Found interfaces:\n");

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
    debug_output("Chosen interface: %5s    Broadcast: %s\n", ifa->ifa_name, inet_ntoa(broadcast.sin_addr));

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
        debug_output("Could not set socket options!\n");
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
            debug_output("Failed to broadcast message!\n");
            break;
        }
    }

    close(sock);
    return 1;
}

static void * _network_traverse(void * v_key, void * v_value)
{
    char * key = (char *) v_key;
    channel_t * array = (channel_t *) v_value;

    debug_output("Channel [%s] devices:\n", key);
    for (int i = 0; i < array->size; ++i)
    {
        debug_output("%d. %x \t", i + 1, array->nodes[i].node_id);
        if ((i + 1) % 5 == 0)
        {
            debug_output("\n");
        }
    }
    debug_output("\n");

    return NULL;
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
        debug_output("Could not set socket options!\n");
        close(sock);
        return 1;
    }

    if (bind(sock, (struct sockaddr *) &client_addr, sizeof(client_addr)) < 0)
    {
        debug_output("Could not bind to %d:%d!\n", ntohl(client_addr.sin_addr.s_addr), ntohs(port));
        return 1;
    }

    while ((bytes = recvfrom(sock, message, BUFFER_DEPTH, 0, (struct sockaddr *) &server_address, &address_length)) > 0)
    {
        message[bytes] = '\0';
        debug_output("%s\n", message);
    }

    close(sock);

    return 0;
}

/*static void * _aggregate_fds(void * key, void * value)
{
    static fds rval;
    static char clean = 1;

    channel_t * channel = (channel_t *) value;
    int fd;

    if (clean)
    {
        rval.max_fd = 0;
        FD_ZERO(&rval.set);
        clean = 0;
    }

    if (key && value) // Both parameters set
    {
        for (int i = 0; i < channel->size; ++i)
        {
            fd = channel->nodes[i].sock;

            FD_SET(fd, &rval.set);
            if (fd > rval.max_fd)
            {
                rval.max_fd = fd;
            }
        }

        return NULL;
    }
    else if (!key && !value) // Both parameters NULL
    {
        clean = 1;

        return &rval;
    }

    return NULL;
}*/

int start_core_server(int port)
{
    int rval;

    hash_table_t table;
    ht_construct(&table, TABLE_SIZE, 250,       sizeof(channel_t), &_hash_channel, &_compare_channel);
    //           table   10          key size   value size         hash function   compare function

    message_t message;
    hash_data_t search;
    channel_t  * channel_target;
    //fds * fd_list;
    int max_fd;
    fd_set active_fds;
    fd_set read_fds;

    struct AES_ctx context;
    const char * key = "01234567012345670123456701234567";  // Test key (32 bytes)
    const char * iv = "0123456701234567";   // Test IV (16 bytes)

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int handle = 0;
    int client_size = sizeof(struct sockaddr);

    char buffer[256];
    char desbuf[256];
    int bytes = 0;

    char channel[250];

    char mode;
    char found;

    int yes = 1;

    struct sockaddr_in server_addr, client_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t) port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    // Ignore SIGPIPE signals
    signal(SIGPIPE, SIG_IGN);

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
    {
        debug_output("Could not set socket options!\n");
        return 1;
    }

    // Bind server socket to the given port
    if (bind(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    {
        debug_output("Could not bind to %d:%d!\n", ntohl(server_addr.sin_addr.s_addr), ntohs(port));
        close(sock);
        return 1;
    }

    // Start listening on the provided port
    if (listen(sock, LISTEN_QUEUE) < 0)
    {
        debug_output("Could not listen for incoming connections!\n");
        close(sock);
        return 1;
    }

    FD_ZERO(&active_fds);
    FD_SET(sock, &active_fds);
    max_fd = sock;

    debug_output("\nCore initialized, awaiting connections...\n");

    while(1)
    {
        // Clear buffers
        memset(buffer, 0, sizeof(buffer));
        memset(desbuf, 0, sizeof(desbuf));
        memset(channel, 0, sizeof(channel));

        debug_output("\n");

        // Acquire all file descriptors of currently subscribed devices
        /*_aggregate_fds(NULL, NULL);
        ht_traverse(&table, &_aggregate_fds);
        fd_list = _aggregate_fds(NULL, NULL);

        FD_SET(sock, &fd_list->set);
        if (sock > fd_list->max_fd)
        {
            fd_list->max_fd = sock;
        }*/

        //if (select(fd_list->max_fd + 1, &fd_list->set, NULL, NULL, NULL) < 0)

        // Wait for incoming connections
        read_fds = active_fds;
        if ((rval = select(max_fd + 1, &read_fds, NULL, NULL, NULL)) < 0)
        {
            debug_output("Failed to select incoming IO!: [%d]\n", errno);
            continue;
        }

        //for (int s = 0; s < fd_list->max_fd + 1; ++s)
        for (int s = 0; s < max_fd + 1; ++s)
        {
            //if (FD_ISSET(s, &fd_list->set))
            if (FD_ISSET(s, &read_fds))
            {
                if (s == sock)
                // Incoming new connection
                {
                    if ((handle = accept(sock, (struct sockaddr *) &client_addr, (socklen_t *) &client_size)) < 0)
                    {
                        debug_output("Failed to accept incoming connection!\n");
                        continue;
                    }
                    else
                    {
                        FD_SET(handle, &active_fds);
                        if (handle > max_fd)
                        {
                            max_fd = handle;
                        }
                        debug_output("Acquired new connection!\n");
                    }
                }
                else
                {
                    debug_output("Acquired old connection!\n");
                    handle = s;
                }

                // Read incoming message
                if ((bytes = read(handle, buffer, sizeof(buffer))) != sizeof(buffer))
                {
                    if (bytes)
                    {
                        debug_output("Invalid initial read, rval: [%d]!\n", bytes);
                    }
                    else
                    {
                        // Read 0 bytes; Node terminated connection
                        debug_output("Node terminated connection!\n");

                        FD_CLR(handle, &active_fds);
                        close(handle);
                    }

                    continue;
                }

                message_initialize(&message);
                memcpy(message.message_string, buffer, 256);

                // Decrypt message (AES256)
                AES_init_ctx_iv(&context, (const uint8_t *) key, (const uint8_t *) iv);
                AES_CBC_decrypt_buffer(&context, (uint8_t *) message.message_string, 256);

                // Generate message struct from message
                message_unpack(&message);

                strcpy(desbuf, buffer);
                strcpy(channel, message.payload);

                switch (message.source_id)
                {
                /*
                 *  PUBLISH
                 */
                case 0:
                // Message is a "Publish" message

                    debug_output("Publish message received!\n");

                    // Read payload message
                    if ((bytes = read(handle, buffer, sizeof(buffer))) != sizeof(buffer))
                    {
                        debug_output("Invalid payload read, rval: [%d]!\n", bytes);
                        continue;
                    }

                    message_initialize(&message);
                    memcpy(message.message_string, buffer, 256);

                    // Decrypt message (AES256)
                    AES_init_ctx_iv(&context, (const uint8_t *) key, (const uint8_t *) iv);
                    AES_CBC_decrypt_buffer(&context, (uint8_t *) message.message_string, 256);

                    // Generate message struct from message
                    message_unpack(&message);

                    debug_output("Publishing message [%s] to channel [%s]!\n", message.payload, channel);

                    // Find channel in table
                    if (ht_search(&table, &search, channel) == HT_DNE)
                    // Channel hasn't been created yet; no devices are subscribed to the target channel
                    {
                        debug_output("No devices are subscribed to channel [%s]!\n", channel);
                    }
                    else
                    // Relay message to all devices subscribed to the target channel
                    {
                        channel_target = (channel_t *) search.value;
                        found = 0;

                        debug_output("Relaying message from channel [%s] to [%d] devices!\n", channel, channel_target->size);

                        for (int i = 0; i < channel_target->size && !found; ++i)
                        {
                            if (_send_to_node(&(channel_target->nodes[i]), desbuf, 256)
                            ||  _send_to_node(&(channel_target->nodes[i]), buffer, 256))
                            // Message failed to send
                            {
                                debug_output("Failed to relay message from channel [%s] to device [%x]!\n", channel, channel_target->nodes[i].node_id);

                                // Remove device from array
                                if (channel_target->size == 1)
                                // Device is the only subscribed device
                                {
                                    free(channel_target->nodes[0].addr);
                                    free(channel_target->nodes);
                                    ht_remove(&table, channel);

                                    found = 1;

                                    debug_output("Channel [%s] has no subscribers. Removed!\n", channel);
                                }
                                else
                                // Device is not the only subscribed device
                                {
                                    // Free Node elements
                                    free(channel_target->nodes[i].addr);

                                    // Patch array
                                    for (int j = i; j < channel_target->size - 1; ++j)
                                    {
                                        channel_target->nodes[i] = channel_target->nodes[i + 1];
                                    }

                                    // Free element
                                    channel_target->nodes = realloc(channel_target->nodes, (channel_target->size - 1) * sizeof(node_t));

                                    channel_target->size -= 1;
                                }
                                ht_traverse(&table, &_network_traverse);
                            }
                            else
                            {
                                debug_output("Message published to channel [%s] relayed to device [%x]!\n", channel, channel_target->nodes[i].node_id);
                            }
                        }
                    }
                    break;

                /*
                 *  SUBSCRIBE
                 */
                default:
                // Message is a "Subscribe" message

                    debug_output("Subscribe message received!\n");

                    mode = (message.source_id & 0x7FFF) >> 15; // Subscribe = 0, unsubscribe = 1
                    message.source_id &= 0x7FFF; // Ignore MSB of source_id field

                    if ((rval = ht_search(&table, &search, channel)) == HT_DNE)
                    // Channel doesn't yet exist in table
                    {
                        if (!mode)
                        // Subscribe
                        {
                            channel_target = calloc(1, sizeof(channel_t));
                            channel_target->size = 1;

                            // TODO: Free the following allocated memory
                            channel_target->nodes = calloc(1, sizeof(node_t));

                            channel_target->nodes[0].addr = calloc(1, sizeof(struct sockaddr_in));
                            *(channel_target->nodes[0].addr) = client_addr;
                            channel_target->nodes[0].sock = handle;
                            channel_target->nodes[0].node_id = message.source_id;

                            ht_insert(&table, channel, channel_target);
                            free(channel_target);

                            debug_output("Channel [%s] created and device [%x] subscribed!\n", channel, message.source_id);
                            ht_traverse(&table, &_network_traverse);
                        }
                        else
                        // Unsubscribe
                        {
                            debug_output("Device [%x] cannot unsubscribe from channel [%s], channel does not exist!\n", message.source_id, channel);
                        }
                    }
                    else if (rval == SUCCESS)
                    // Channel exists in table
                    {
                        if(!mode)
                        // Subscribe
                        {
                            channel_target = (channel_t *) search.value;
                            found = 0;

                            // If device already exists, update address
                            for (int i = 0; i < channel_target->size; ++i)
                            {
                                if (channel_target->nodes[i].node_id == message.source_id)
                                {
                                    found = 1;

                                    *(channel_target->nodes[i].addr) = client_addr;
                                    channel_target->nodes[i].sock = handle;

                                    debug_output("Device [%x] subscription to channel [%s] updated!\n", message.source_id, channel);
                                    ht_traverse(&table, &_network_traverse);
                                    break;
                                }
                            }

                            if (!found)
                            // Device does not yet exist in array of subscribers
                            {
                                channel_target->nodes = realloc(channel_target->nodes, (channel_target->size + 1) * sizeof(node_t));

                                channel_target->nodes[channel_target->size].addr = calloc(1, sizeof(struct sockaddr_in));
                                *(channel_target->nodes[channel_target->size].addr) = client_addr;
                                channel_target->nodes[channel_target->size].sock = handle;
                                channel_target->nodes[channel_target->size].node_id = message.source_id;

                                channel_target->size += 1;

                                debug_output("Device [%x] subscribed to channel [%s]!\n", message.source_id, channel);
                                ht_traverse(&table, &_network_traverse);
                            }
                        }
                        else
                        // Unsubscribe
                        {
                            channel_target = (channel_t *) search.value;

                            // Find index of subscribed device and remove it from array
                            for (int i = 0; i < channel_target->size; ++i)
                            {
                                if (channel_target->nodes[i].node_id == message.source_id)
                                {
                                    if (channel_target->size == 1)
                                    // Device is the only subscribed device
                                    {
                                        free(channel_target->nodes[0].addr);
                                        free(channel_target->nodes);
                                        ht_remove(&table, channel);

                                        debug_output("Channel [%s] has no subscribers. Removed!\n", channel);
                                    }
                                    else
                                    // Device is not the only subscribed device
                                    {
                                        // Free Node elements
                                        free(channel_target->nodes[i].addr);

                                        // Patch array
                                        for (int j = i; j < channel_target->size - 1; ++j)
                                        {
                                            channel_target->nodes[i] = channel_target->nodes[i + 1];
                                        }

                                        // Free element
                                        channel_target->nodes = realloc(channel_target->nodes, (channel_target->size - 1) * sizeof(node_t));

                                        channel_target->size -= 1;
                                    }

                                    break;
                                }
                            }
                            debug_output("Device [%x] unsubscribed to channel [%s]!\n", message.source_id, channel);
                        }
                    }
                    else
                    {
                        debug_output("An unknown error occurred when handling Subscription message: [%d]!\n", rval);
                        break;
                    }
                    break;
                }
            }
        }
    }

    return 0;
}

int start_node_client(core_t * core, unsigned int id, char * ip, int port)
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
            debug_output("Could not connect to server!\n");
            return 1;
        }
        else
        {
            // Connection succeeded
            core->addr = calloc(1, sizeof(server_addr));
            *(core->addr) = server_addr;
            core->sock = sock;
            core->node_id = id;
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
            debug_output("Cannot publish to channel name of length 250 or greater!\n");
            return 1;
        }

        if (strlen(payload) >= 250)
        {
            // TODO: Allow arbitrary message size
            debug_output("Cannot publish message of length 250 or greater!\n");
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
        //fprintf(stderr, "%x %x %s\n", message.bytes_remaining, message.source_id, message.payload);

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

        //message_debug_hex(message.message_string);

        // Encrypt message (AES256)
        AES_init_ctx_iv(&context, (const uint8_t *) key, (const uint8_t *) iv);
        AES_CBC_encrypt_buffer(&context, (uint8_t *) message.message_string, 256);

        //fprintf(stderr, "\n");
        //message_debug_hex(message.message_string);

        // Send message
        if (_send_to_core(core, message.message_string, 256))
        {
            debug_output("Message could not be sent to Core!\n");
        }
        else
        {
            debug_output("Message sent to Core!\n");
        }


    }
    else
    {
        // Invalid parameters
        return 1;
    }

    return 0;
}

int subscribe(core_t * core, char * channel, void (*callback)(char *))
{
    message_t message;

    struct AES_ctx context;
    const char * key = "01234567012345670123456701234567";  // 32 bytes
    const char * iv = "0123456701234567";   // 16 bytes

    static char init = 0;
    static subpack_t pack;

    pthread_t listener;

    if (core && channel && callback)
    {
        if (strlen(channel) >= 250)
        {
            debug_output("Cannot subscribe to channel name of length 250 or greater!\n");
            return 1;
        }

        /*
         * Set up listener server
         */

        if (!init)
        {
            pack.core = core;
            pack.size = 1;
            pack.subs = calloc(1, sizeof(subscription_t));
            pack.lock = calloc(1, sizeof(pthread_mutex_t));

            strcpy(pack.subs[0].channel, channel);
            pack.subs[0].callback = callback;

            pthread_mutex_init(pack.lock, NULL);

            if (pthread_create(&listener, NULL, &_subscription_listener, (void *) &pack))
            {
                debug_output("Could not create listener thread!\n");
                return 1;
            }

            init = 1;
        }
        else
        {
            pthread_mutex_lock(pack.lock);

            pack.subs = realloc(pack.subs, (pack.size + 1) * sizeof(subscription_t));
            strcpy(pack.subs[pack.size].channel, channel);
            pack.subs[pack.size].callback = callback;
            pack.size += 1;

            pthread_mutex_unlock(pack.lock);
        }

        /*
         * Send channel subscription message
         */

        // Set up message
        message_initialize(&message);
        message.bytes_remaining = strlen(channel);  // Bytes Remaining
        message.source_id = core->node_id;  // Node device ID
        message.source_id &= 0x7FFF;    // Force MSB of ID to 0
        strcpy(message.payload, channel);   // Payload

        // Serialize message
        message_pack(&message);
        //fprintf(stderr, "%x %x %s\n", message.bytes_remaining, message.source_id, message.payload);

        // Encrypt message (AES256)
        AES_init_ctx_iv(&context, (const uint8_t *) key, (const uint8_t *) iv);
        AES_CBC_encrypt_buffer(&context, (uint8_t *) message.message_string, 256);

        // Send message
        if(_send_to_core(core, message.message_string, 256))
        {
            debug_output("Message could not be sent to Core!\n");
        }
        else
        {
            debug_output("Message sent to Core!\n");
        }
    }
    else
    {
        // Invalid parameters
        return 1;
    }

    return 0;
}
