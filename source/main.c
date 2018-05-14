#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>

#include "reactant_peripherals.h"
#include "reactant_network.h"
#include "reactant_util.h"
#include "reactant_ui.h"
#include "exsrc_minIni.h"


//char reverse_byte(char byte);

//void core_test();
//void node_test();

void core_integration_test();
void node_integration_test();

//void spi_test();
//void i2c_test();
//void ui_test();
//void queue_test();
//void message_test();

//int configure_callback(WINDOW * PH(window));
//int test_callback(WINDOW * window);

void humidity_callback(char * message);
void light_callback(char * message);
void pressure_callback(char * message);
void temperature_callback(char * message);
void generic_callback(char *message);

typedef struct _gencfg_t {
        char ip[16];
        short port;
} gencfg_t;

static int _gencfg_handler(const mTCHAR *section, const mTCHAR *key, const mTCHAR *value, void *user) {
    gencfg_t *gencfg = (gencfg_t *)user;

    debug_output("[%s] %s=%s\n", section, key, value);

    #define MATCH(s, k) (strcmp(s, section) == 0 && strcmp(k, key) == 0)
    if (MATCH("general", "core-ip")) {
        strcpy(gencfg->ip, value);
    } else if (MATCH("general", "port")) {
        gencfg->port = (short) atoi(value);
    }
    return 1;
}

int main() {
    debug_control(DISABLE);

    //core_integration_test();
    node_integration_test();

    return 0;
}

void core_integration_test() {
    gencfg_t config;
    if (ini_browse(&_gencfg_handler, &config, "cfg.ini") < 0) {
        debug_output("Failed to load configuration settings!\n");
        return;
    }

    start_core_server(config.port);
}

void node_integration_test() {
#ifdef __arm__

    core_t core;
    int rval = 0;
    float temperature = 0;
    char buffer[250];
    short ch0, ch1;

    gencfg_t config;
    if (ini_browse(&_gencfg_handler, &config, "cfg.ini") < 0) {
        debug_output("Failed to load configuration settings!\n");
        return;
    }

    debug_output("%x\n", config.ip);

    if(!peripheral_init()) {
        // Setup light sensor timing register (402ms integration time, 16x gain)
        smbus_write_byte(TSL2561_TIMING, TSL2561_INTEGRATION_402 | TSL2561_GAIN_16);

        // Ensure valid light sensor ID register
        if ((rval = smbus_read_byte(TSL2561_ID)) != 0x50) {
            fprintf(stderr, "Returned invalid ID register read!: 0x%x\n", rval);
        }
        rval = 0;

        // Enable light sensor
        tsl2561_enable();

        if (!start_node_client(&core, 0x741, config.ip, config.port)) { // 192.168.1.105
            subscribe(&core, "Humidity-1", &humidity_callback);
            subscribe(&core, "Light-1", &light_callback);
            subscribe(&core, "Pressure-1", &pressure_callback);
            subscribe(&core, "Temperature-1", &temperature_callback);

            subscribe(&core, "General-1", &generic_callback);
            subscribe(&core, "General-2", &generic_callback);
            subscribe(&core, "General-3", &generic_callback);
            subscribe(&core, "General-4", &generic_callback);

            while (rval < 900) {
                fprintf(stderr, "\n");

                // Humidity
                snprintf(buffer, sizeof(buffer), "%d", mcp3008_read_channel(2));
                publish(&core, "Humidity-1", buffer);

                // Light
                usleep(0.403 * 1000000);
                ch0 = smbus_read_word(TSL2561_WORD | TSL2561_DATA0LOW);
                ch1 = smbus_read_word(TSL2561_WORD | TSL2561_DATA1LOW);
                snprintf(buffer, sizeof(buffer), "Ch0 (broadband): %d \tCh1 (IR): %d", ch0, ch1);
                publish(&core, "Light-1", buffer);

                // Pressure
                rval = mcp3008_read_channel(0);
                snprintf(buffer, sizeof(buffer), "%d", rval);
                publish(&core, "Pressure-1", buffer);

                // Temperature
                temperature = mcp3008_read_channel(1);
                temperature = (225.0 * temperature) / 256.0 - 58.0;
                snprintf(buffer, sizeof(buffer), "%d", (int) temperature);
                publish(&core, "Temperature-1", buffer);

                if (rval >= 100 && rval < 300) {
                    publish(&core, "General-1", "General-1 publish!");
                } else if (rval >= 300 && rval < 500) {
                    publish(&core, "General-2", "General-2 publish!");
                } else if (rval >= 500 && rval < 700) {
                    publish(&core, "General-3", "General-3 publish!");
                } else if (rval >= 700 && rval < 900) {
                    publish(&core, "General-4", "General-4 publish!");
                }
                bcm2835_delay(1000);
            }
            stop_node_client(&core);
        }
        // Disable light sensor
        tsl2561_disable();

        peripheral_term();
    } else {
        debug_output("node_integration_test() failed! Could not initialize peripherals!\n");
    }
#endif
}

void humidity_callback(char *message) {
    fprintf(stderr, "Humidity reading: %s\n", message);
}

void light_callback(char *message) {
    fprintf(stderr, "Light reading: %s\n", message);
}

void pressure_callback(char *message) {
    fprintf(stderr, "Pressure reading: %s\n", message);
}

void temperature_callback(char *message) {
    fprintf(stderr, "Temperature reading: %s F\n", message);
}

void generic_callback(char *message) {
    fprintf(stderr, "Received relayed: %s\n", message);
}

/*void core_test()
{
    start_core_server(10112);
}*/

/*void _node_callback(char * message)
{
    fprintf(stderr, "Received relayed: %s\n", message);
}*/

/*void node_test()
{
    core_t core;

    if (!start_node_client(&core, 0x741, "192.168.1.105", 10112)) // 192.168.1.105
    {
        publish(&core, "chat1", "this is a test");

        subscribe(&core, "chat1", &_node_callback);
        subscribe(&core, "chat3", &_node_callback);
        subscribe(&core, "chat4", &_node_callback);

        publish(&core, "chat1", "this is a test");


        publish(&core, "chat2", "this is a test2");

        subscribe(&core, "chat2", &_node_callback);

        publish(&core, "chat2", "this is a test2");


        while(1);

        stop_node_client(&core);
    }
}*/

/*char reverse_byte(char byte)
{
    char rval = 0;

    for (int i = 0; i < 4; ++i)
    {
        rval |= ((byte & (1 << i)) << (8 - i * 2 - 1));
    }
    for (int i = 3; i >= 0; --i)
    {
        rval |= ((byte & (0x80 >> i)) >> (8 - i * 2 - 1));
    }

    return rval;
}*/

/*void spi_test()
{
#ifdef __arm__
    if(!peripheral_init())
    {
        int rval = 0;
        float temperature = 0;

        while(rval < 850)
        {
            temperature = mcp3008_read_channel(1);

            fprintf(stderr, "Ch %d: %d \tCh %d: %d\n", 1, rval = mcp3008_read_channel(0), 2, (int) temperature);

            temperature = (225.0 * temperature) / 256.0 - 58.0;
            fprintf(stderr, "Temperature: %f\n\n", temperature);

            bcm2835_delay(100);
        }

        peripheral_term();
    }
    else
    {
        fprintf(stderr, "%s\n", "spi_test() failed! Could not initialize peripherals!");
    }
#endif
}*/

/*void i2c_test()
{
#ifdef __arm__
    char rval;
    short ch0, ch1;

    // Initialize peripherals
    if(!peripheral_init())
    {
        // Setup timing register (402ms integration time, 16x gain)
        smbus_write_byte(TSL2561_TIMING, TSL2561_INTEGRATION_402 | TSL2561_GAIN_16);

        // Ensure valid ID register
        if ((rval = smbus_read_byte(TSL2561_ID)) != 0x50)
        {
            fprintf(stderr, "%s: 0x%x\n", "Returned invalid ID register read!", rval);
        }

        // Enable light sensor
        tsl2561_enable();

        do
        {
            // Wait 403 ms (because of the 402ms integration time)
            usleep(0.403 * 1000000);

            // Read channel 0 (broadband)
            ch0 = smbus_read_word(TSL2561_WORD | TSL2561_DATA0LOW);

            // Read channel 1 (IR)
            ch1 = smbus_read_word(TSL2561_WORD | TSL2561_DATA1LOW);

            fprintf(stderr, "Ch0 (broadband): %d \tCh1 (IR): %d\n", ch0, ch1);

        } while (ch0 > 2000);

        // Disable light sensor
        tsl2561_disable();

        // Terminate peripherals
        peripheral_term();
    }
    else
    {
        fprintf(stderr, "%s\n", "i2c_test() failed! Could not initialize peripherals!");
    }
#endif
}*/

/*void ui_test()
{
    // ################
    // # Run Reactant #
    // ################
    curses_init();

    // Menu items
    panel_t * panels[4];

    panels[0] = create_panel("Operation", 2, 3); // 2, 3
    add_panel_button(panels[0], create_button("Start Reactant Core server", test_callback));
    add_panel_button(panels[0], create_button("Configure", configure_callback));
    add_panel_button(panels[0], create_button("Exit", NULL));

    panels[1] = create_panel("Other", 2, 40);
    add_panel_button(panels[1], create_button("About", test_callback));
    add_panel_button(panels[1], create_button("Test Reactant Node device", test_callback));

    panels[2] = create_panel("TESTPANEL1", 7, 3);
    add_panel_button(panels[2], create_button("TESTBUTTON1", test_callback));
    add_panel_button(panels[2], create_button("TESTBUTTON2", test_callback));
    add_panel_button(panels[2], create_button("TESTBUTTON3", test_callback));

    panels[3] = create_panel("TESTPANEL2", 7, 40);
    add_panel_button(panels[3], create_button("TESTBUTTON4", test_callback));
    add_panel_button(panels[3], create_button("TESTBUTTON5", test_callback));
    add_panel_button(panels[3], create_button("TESTBUTTON6", test_callback));

    panels[0]->selected = 1;
    panels[0]->items[0]->selected = 1;

    menu_t * menu = create_menu("Reactant Primary Control");
    add_menu_panel(menu, panels[0]);
    add_menu_panel(menu, panels[1]);
    add_menu_panel(menu, panels[2]);
    add_menu_panel(menu, panels[3]);

    operate_menu(menu);

    //    int status = start_server(10801);
    //    if (status > 0)
    //    {
    //        printw("Failed to establish server!\n");
    //        return 1;
    //    }

    free_menu(menu);
    curses_term();
}*/

/*void queue_test()
{
    queue_t q;

    int * items[10];
    int * rval = 0;

    debug_control(ENABLE);

    queue_construct(&q, 10);

    for(int i = 0; i < q.capacity; ++i)
    {
        items[i] = malloc(sizeof(int));
        *items[i] = i;
        queue_check(enqueue(&q, items[i]));
    }

    printf("Capacity: %d, Size: %d\n", (int) q.capacity, (int) q.size);

    printf("test\n");
    queue_check(enqueue(&q, items[0]));

    for(int i = 0; i < 5; ++i)
    {
        queue_check(dequeue(&q, (void **) &rval));
        printf("%d\n", *rval);
    }

    for(int i = 0; i < 5; ++i)
    {
        queue_check(enqueue_blocking(&q, items[7]));
    }

    for(int i = 0; i < q.capacity; ++i)
    {
        queue_check(dequeue_blocking(&q, (void **) &rval));
        printf("%d\n", *rval);
    }

    printf("test\n");

    queue_check(dequeue(&q, (void **) &rval));
    printf("%d\n", *rval);

    for (int i = 0; i < q.capacity; ++i)
    {
        free(items[i]);
    }

    queue_destruct(&q);
}*/

/*void message_test()
{
    message_t message1, message2;
    message_initialize(&message1);
    message_initialize(&message2);

    message1.bytes_remaining = 0x1234;
    message1.source_id = 0x567890AB;
    strcpy(message1.payload, "Hello!");

    message_pack(&message1);

    strcpy(message2.message_string, message1.message_string);

    message_unpack(&message2);

    fprintf(stderr, "%x %x %s\n", message2.bytes_remaining, message2.source_id, message2.payload);
}*/

/*int configure_callback(WINDOW * PH(window))
{
    int rval = 0;

    panel_t * panel = create_panel("Operation", 2, 3);
    add_panel_button(panel, create_button("Back", NULL));

    menu_t * menu = create_menu("Reactant Configuration");
    add_menu_panel(menu, panel);

    rval = operate_menu(menu);

    free_menu(menu);

    if (rval == UI_QUIT)
    {
        return UI_QUIT;
    }

    return rval;
}*/

/*int test_callback(WINDOW * window)
{
    int y, x;
    getmaxyx(stdscr, y, x);
    mvwprintw(window, y / 2, x / 2 - 7, "This is a test!");

    return 0;
}*/
