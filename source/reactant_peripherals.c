#include "reactant_peripherals.h"

int peripheral_init()
{
    if (!bcm2835_init())
    {
        fprintf(stderr, "bcm2835_init() failed. Try running as root user!\n");
        return 1;
    }

    return 0;
}

int peripheral_term()
{
    bcm2835_close();

    return 0;
}

int peripheral_spi_init()
{
    if (!bcm2835_spi_begin())
    {
        fprintf(stderr, "bcm2835_spi_begin() failed. Try running as root user!\n");
        return 1;
    }

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

    return 0;
}

int peripheral_i2c_init()
{
    if (!bcm2835_i2c_begin())
    {
        fprintf(stderr, "bcm2835_i2c_begin() failed. Try running as root user!\n");
        return 1;
    }

    return 0;
}

int peripheral_i2c_term()
{
    bcm2835_i2c_end();

    return 0;
}

int mcp3008_read_channel(int channel)
{
    char data[3];
    int rval;

    if (channel < 0 || channel > 7)
    {
        fprintf(stderr, "mcp3008_read_channel() failed. Invalid channel parameter!\n");
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

