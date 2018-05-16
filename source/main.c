#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>

#include "reactant_network.h"
#include "reactant_util.h"
#include "exsrc_minIni.h"
#include "tests.h"


void main_menu();
void core_integration_test();
void node_integration_test();

void humidity_callback(char * message);
void light_callback(char * message);
void pressure_callback(char * message);
void temperature_callback(char * message);
void generic_callback(char *message);

typedef struct _gencfg_t {
        char ip[16];
        short port;
        char key[33];
        char iv[17];
} gencfg_t;

static int _gencfg_handler(const mTCHAR *section, const mTCHAR *key, const mTCHAR *value, void *user) {
    gencfg_t *gencfg = (gencfg_t *)user;

    debug_output("[%s] %s=%s\n", section, key, value);

    #define MATCH(s, k) (strcmp(s, section) == 0 && strcmp(k, key) == 0)
    if (MATCH("general", "core-ip")) {
        strcpy(gencfg->ip, value);
    } else if (MATCH("general", "port")) {
        gencfg->port = (short) atoi(value);
    } else if (MATCH("security", "key")) {
        strcpy(gencfg->key, value);
    } else if (MATCH("security", "iv")) {
        strcpy(gencfg->iv, value);
    }
    return 1;
}

int main() {
    main_menu();

    return 0;
}

int _port_config(WINDOW *window) {
    char port[6] = { 0 };
    prompt(window, "Enter a new port number:", port, sizeof(port));
    ini_puts("general", "port", port, CONF_INI);
    return 0;
}

int _ip_config(WINDOW *window) {
    char ip[16] = { 0 };
    prompt(window, "Enter a new Core IP:", ip, sizeof(ip));
    ini_puts("general", "core-ip", ip, CONF_INI);
    return 0;
}

int _key_config(WINDOW *window) {
    char key[33] = { 0 };
    prompt(window, "Enter a new encryption key:", key, sizeof(key));
    ini_puts("security", "key", key, CONF_INI);
    return 0;
}

int _iv_config(WINDOW *window) {
    char iv[17] = { 0 };
    prompt(window, "Enter a new encryption IV:", iv, sizeof(iv));
    ini_puts("security", "iv", iv, CONF_INI);
    return 0;
}

int _configure_callback(WINDOW *window) {
    int rval = 0;
    panel_t * panels[2];

    panels[0] = create_panel("General", 2, 3);
    add_panel_button(panels[0], create_button("Port", _port_config));
    add_panel_button(panels[0], create_button("Core IP", _ip_config));

    panels[1] = create_panel("Security", 2, 40);
    add_panel_button(panels[1], create_button("Key", _key_config));
    add_panel_button(panels[1], create_button("IV", _iv_config));

    panels[0]->selected = 1;
    panels[0]->items[0]->selected = 1;

    menu_t * menu = create_menu("Reactant Configuration");
    add_menu_panel(menu, panels[0]);
    add_menu_panel(menu, panels[1]);

    rval = operate_menu(menu);

    free_menu(menu);
    if (rval == UI_QUIT) {
        return UI_QUIT;
    }
    return rval;
}

int _core_test_callback(WINDOW *window) {
    endwin();
    core_integration_test();
    return 0;
}

int _node_test_callback(WINDOW *window) {
    endwin();
    node_integration_test();
    return 0;
}

void main_menu() {
    curses_init();

    // Menu items
    panel_t * panels[3];

    panels[0] = create_panel("Operation", 2, 3); // 2, 3
    add_panel_button(panels[0], create_button("Start Reactant Core", NULL));
    add_panel_button(panels[0], create_button("Configure", _configure_callback));
    add_panel_button(panels[0], create_button("Exit", NULL));

    panels[1] = create_panel("Other", 2, 40);
    add_panel_button(panels[1], create_button("About", NULL));
    add_panel_button(panels[1], create_button("Test Reactant Core", _core_test_callback));
    add_panel_button(panels[1], create_button("Test Reactant Node", _node_test_callback));

    panels[2] = create_panel("Tests", 7, 3);
    add_panel_button(panels[2], create_button("All Tests", test_all));
    add_panel_button(panels[2], create_button("SPI", test_spi_cb));
    add_panel_button(panels[2], create_button("I2C", test_i2c_cb));
    add_panel_button(panels[2], create_button("Humidity", test_humidity_cb));
    add_panel_button(panels[2], create_button("Light", test_light_cb));
    add_panel_button(panels[2], create_button("Pressure", test_pressure_cb));
    add_panel_button(panels[2], create_button("Temperature", test_temperature_cb));
    add_panel_button(panels[2], create_button("AES256", test_aes_cb));
    add_panel_button(panels[2], create_button("SHA256", test_sha_cb));
    add_panel_button(panels[2], create_button("Message", test_message_cb));
    add_panel_button(panels[2], create_button("Channels", test_channels_cb));

    panels[0]->selected = 1;
    panels[0]->items[0]->selected = 1;

    menu_t * menu = create_menu("Reactant Primary Control");
    for (int i = 0; i < sizeof(panels) / sizeof(panel_t*); ++i) {
        add_menu_panel(menu, panels[i]);
    }

    operate_menu(menu);

    free_menu(menu);
    curses_term();
}


void core_integration_test() {
    debug_control(ENABLE);

    gencfg_t config;
    if (ini_browse(&_gencfg_handler, &config, CONF_INI) < 0) {
        debug_output("Failed to load configuration settings!\n");
        return;
    }

    start_core_server(config.port, config.key, config.iv);
}

void node_integration_test() {
#ifdef __arm__
    debug_control(DISABLE);

    core_t core;
    int rval = 0;
    float temperature = 0;
    char buffer[250];
    short ch0, ch1;

    gencfg_t config;
    if (ini_browse(&_gencfg_handler, &config, CONF_INI) < 0) {
        debug_output("Failed to load configuration settings!\n");
        return;
    }

    if (!peripheral_init()) {
        // Setup light sensor timing register (402ms integration time, 16x gain)
        smbus_write_byte(TSL2561_TIMING, TSL2561_INTEGRATION_402 | TSL2561_GAIN_16);

        // Ensure valid light sensor ID register
        if ((rval = smbus_read_byte(TSL2561_ID)) != 0x50) {
            fprintf(stderr, "Returned invalid ID register read!: 0x%x\n", rval);
        }
        rval = 0;

        // Enable light sensor
        tsl2561_enable();

        if (!start_node_client(&core, 0x741, config.ip, config.port, config.key, config.iv)) { // 192.168.1.105
            subscribe(&core, "Humidity-1", &humidity_callback);
            subscribe(&core, "Light-1", &light_callback);
            subscribe(&core, "Pressure-1", &pressure_callback);
            subscribe(&core, "Temperature-1", &temperature_callback);

            subscribe(&core, "General-1", &generic_callback);
            subscribe(&core, "General-2", &generic_callback);
            subscribe(&core, "General-3", &generic_callback);
            subscribe(&core, "General-4", &generic_callback);

            while (rval < 900) {
                debug_output("\n");

                // Humidity
                snprintf(buffer, sizeof(buffer), "%d", mcp3008_read_channel(2));
                publish(&core, "Humidity-1", buffer);

                // Light
                usleep(0.403 * S_MUL);
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
