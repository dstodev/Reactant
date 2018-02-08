/*******************************************************************************
 * Author:	Daniel J. Stotts
 * Purpose: Includes functions to interact with peripherals
 * Revision Date: 2/8/2018
 ******************************************************************************/

#include <stdio.h>

#include <bcm2835.h>

int peripheral_init();
int peripheral_term();
int peripheral_spi_init();
int peripheral_spi_term();

int mcp3008_read_channel(int channel);
