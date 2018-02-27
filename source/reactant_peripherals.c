/*******************************************************************************
 * Author:  Daniel J. Stotts
 * Purpose: Includes functions to interact with peripherals
 * Revision Date: 2/27/2018
 ******************************************************************************/

#include "reactant_peripherals.h"


// #############################################################################
// #                                                                           #
// #    Internal I2C Helpers                                                   #
// #                                                                           #
// #############################################################################
int _i2c_open(const char * device)
{
    static int i2c_device = 0;
    int file;

    if (device == NULL)
    {
        // Close and clear file
        if (i2c_device)
        {
            close(i2c_device);
        }
        i2c_device = 0;
    }
    else
    {
        // Open I2C device
        if (!i2c_device)
        {

            if ((file = open(device, O_RDWR)) < 0)
            {
                fprintf(stderr, "%s\n", "_i2c_open() failed to open I2C device!");
                return -1;
            }

            i2c_device = file;
        }
    }

    return i2c_device;
}

int _i2c_get()
{
    return _i2c_open("/dev/i2c-1");
}

int _i2c_close()
{
    return _i2c_open(NULL);
}

// #############################################################################
// #                                                                           #
// #    General                                                                #
// #                                                                           #
// #############################################################################
int peripheral_init()
{
    int rval = 0;

    if ((rval = peripheral_spi_init()) < 0)
    {
        fprintf(stderr, "%s\n", "peripheral_init() failed! Could not initialize SPI peripherals!");
    }
    else if ((rval = peripheral_i2c_init()) < 0)
    {
        fprintf(stderr, "%s\n", "peripheral_init() failed! Could not initialize I2C peripherals!");
    }

    return rval;
}

int peripheral_term()
{
    peripheral_spi_term();
    peripheral_i2c_term();

    return 0;
}

// #############################################################################
// #                                                                           #
// #    SPI                                                                    #
// #                                                                           #
// #############################################################################
int peripheral_spi_init()
{
    if (!bcm2835_init())
    {
        fprintf(stderr, "%s\n", "peripheral_spi_init() failed. Try running as root user!");
        return -1;
    }

    if (!bcm2835_spi_begin())
    {
        fprintf(stderr, "%s\n", "peripheral_spi_init() failed. Try running as root user!");
        return -1;
    }

    // MCP3008 settings
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_65536);
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

    return 0;
}

int peripheral_spi_term()
{
    bcm2835_spi_end();
    bcm2835_close();

    return 0;
}

// #############################################################################
// #                                                                           #
// #    I2C                                                                    #
// #                                                                           #
// #############################################################################
int peripheral_i2c_init()
{
    _i2c_open("/dev/i2c-1");

    // TSL2561 settings
    i2c_set_addr(0x39);

    return 0;
}

int peripheral_i2c_term()
{
    _i2c_close();

    return 0;
}

int i2c_set_addr(const char addr)
{
    int file = _i2c_get();

    // Set I2C address
    if (ioctl(file, I2C_SLAVE, addr) < 0)
    {
        fprintf(stderr, "%s\n", "_i2c_set_addr() failed to set I2C address!");
        _i2c_close();
        return -1;
    }

    return 0;
}

char smbus_read_byte(const char reg)
{
    char rval = 0;
    int file = _i2c_get();

    if ((rval = i2c_smbus_read_byte_data(file, reg)) < 0)
    {
        fprintf(stderr, "%s\n", "smbus_read_byte() failed to read!");
        _i2c_close();
    }

    return rval;
}

int smbus_write_byte(const char reg, const char data)
{
    int rval = 0;
    int file = _i2c_get();

    if ((rval = i2c_smbus_write_byte_data(file, reg, data)) < 0)
    {
        fprintf(stderr, "%s\n", "smbus_write_byte() failed to transmit!");
        _i2c_close();
    }

    return rval;
}

short smbus_read_word(const char reg)
{
    short rval = 0;
    int file = _i2c_get();

    if ((rval = i2c_smbus_read_word_data(file, reg)) < 0)
    {
        fprintf(stderr, "%s\n", "smbus_read_word() failed to read!");
        _i2c_close();
    }

    return rval;
}

int smbus_write_word(const char reg, const short data)
{
    int rval = 0;
    int file = _i2c_get();

    if ((rval = i2c_smbus_write_word_data(file, reg, data)) < 0)
    {
        fprintf(stderr, "%s\n", "smbus_write_word() failed to transmit!");
        _i2c_close();
    }

    return rval;
}

// #############################################################################
// #                                                                           #
// #    MCP3008                                                                #
// #                                                                           #
// #############################################################################
int mcp3008_read_channel(int channel)
{
    char data[3];
    int rval;

    if (channel < 0 || channel > 7)
    {
        fprintf(stderr, "%s\n", "mcp3008_read_channel() failed. Invalid channel parameter!");
        return 1;
    }


    data[0] = 0x01;
    data[1] = 0x80 | (char) channel << 4;
    data[2] = 0x00;

    bcm2835_spi_transfern(data, 3);

    rval = (data[1] & 0x03) << 0x08;
    rval |= data[2];

    return rval;
}

// #############################################################################
// #                                                                           #
// #    MCP3008                                                                #
// #                                                                           #
// #############################################################################
int tsl2561_enable()
{
    int rval = 0;
    char val = 0;

    if (smbus_read_byte(TSL2561_CONTROL) == 0x33)
    {
        fprintf(stderr, "%s\n", "tsl2561_enable() failed. Light sensor already enabled!");
        rval = -1;
    }
    else
    {
        // Enable sensor
        smbus_write_byte(TSL2561_CONTROL, 0x03);

        if ((val = smbus_read_byte(TSL2561_CONTROL)) != 0x03)
        {
            fprintf(stderr, "tsl2561_enable() failed. Returned: 0x%x\n", val);
            rval = -2;
        }
    }

    return rval;
}

int tsl2561_disable()
{
    int rval = 0;
    char val = 0;

    if (smbus_read_byte(TSL2561_CONTROL) == 0x00)
    {
        fprintf(stderr, "tsl2561_disable() failed. Light sensor already disabled!");
        rval = -1;
    }
    else
    {
        // Disable sensor
        smbus_write_byte(TSL2561_CONTROL, 0x00);

        if ((val = smbus_read_byte(TSL2561_CONTROL)) != 0x00)
        {
            fprintf(stderr, "tsl2561_disabled() failed. Returned: 0x%x\n", val);
            rval = -2;
        }
    }

    return rval;
}
