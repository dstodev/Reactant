#include <curses.h>
#include "reactant_network.h"
#include "reactant_peripherals.h"
#include "reactant_ui.h"
#include "reactant_util.h"

#include <openssl/crypto.h>
#include <openssl/sha.h>
#include "exsrc_aes.h"

#define GREEN   "\x1B[32m"
#define RED     "\x1B[31m"
#define DEFAULT "\x1B[0m"


void print_result(const char * label, unsigned char rval, int width);

int test_all(WINDOW *window);
int test_spi_cb(WINDOW *window);
int test_i2c_cb(WINDOW *window);
int test_humidity_cb(WINDOW *window);
int test_light_cb(WINDOW *window);
int test_pressure_cb(WINDOW *window);
int test_temperature_cb(WINDOW *window);
int test_aes_cb(WINDOW *window);
int test_sha_cb(WINDOW *window);
int test_message_cb(WINDOW *window);
int test_channels_cb(WINDOW *window);

int test_spi();
int test_i2c();
int test_humidity();
int test_light();
int test_pressure();
int test_temperature();
int test_aes();
int test_sha();
int test_message();
int test_channels();

void spi_test();
void i2c_test();
void ui_test();
void queue_test();
void message_test();

