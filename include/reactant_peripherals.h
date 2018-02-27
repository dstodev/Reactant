/*******************************************************************************
 * Author:  Daniel J. Stotts
 * Purpose: Includes functions to interact with peripherals
 * Revision Date: 2/8/2018
 ******************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>

#include <bcm2835.h>


/*******************************************************************************
 *  Category:   General
 *  Description:    Defines elements used by other categories
 ******************************************************************************/
int peripheral_init();
int peripheral_term();


/*******************************************************************************
 *  Category:   SPI
 *  Description:    Defines elements used by SPI devices
 *                  (Tailored for MCP3008 ADC)
 ******************************************************************************/
int peripheral_spi_init();
int peripheral_spi_term();


/*******************************************************************************
 *  Category:   I2C
 *  Description:    Defines elements used by I2C devices
 *                  (Tailored for TSL2561 light sensor)
 ******************************************************************************/
int peripheral_i2c_init();
int peripheral_i2c_term();

int i2c_set_addr(char addr);
char smbus_read_byte(char reg);
short smbus_read_word(char reg);
int smbus_write_byte(char reg, char data);
int smbus_write_word(char reg, short data);


/*******************************************************************************
 *  Category:   MCP3008
 *  Description:    Defines elements used by the MCP3008 device (SPI)
 ******************************************************************************/
int mcp3008_read_channel(int channel);


/*******************************************************************************
 *  Category:   TSL2561
 *  Description:    Defines elements used by the TSL2561 device (I2C)
 ******************************************************************************/
// Register definitions
#define TSL2561_COMMAND (0x80)
#define TSL2561_WORD (0x20)

#define TSL2561_CONTROL (0x80)
#define TSL2561_TIMING  (0x81)
#define TSL2561_THRESHLOWLOW    (0x82)
#define TSL2561_THRESHLOWHIGH   (0x83)
#define TSL2561_THRESHHIGHLOW   (0x84)
#define TSL2561_THRESHHIGHHIGH  (0x85)
#define TSL2561_INTERRUPT   (0x86)
#define TSL2561_ID  (0x8A)
#define TSL2561_DATA0LOW    (0x8C)
#define TSL2561_DATA0HIGH   (0x8D)
#define TSL2561_DATA1LOW    (0x8E)
#define TSL2561_DATA1HIGH   (0x8F)

#define TSL2561_INTEGRATION_13 (0x00)
#define TSL2561_INTEGRATION_101 (0x01)
#define TSL2561_INTEGRATION_402 (0x02)

#define TSL2561_GAIN_1 (0x00)
#define TSL2561_GAIN_16 (0x10)

int tsl2561_enable();
int tsl2561_disable();



