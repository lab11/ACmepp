#include <string.h>

#include "contiki.h"
#include "ade7753.h"
#include "spi.h"

unsigned char ade_data[3];

#define CS_INIT() {\
	P2DIR |= 0x04;	/* ADE CS on PB.2 */\
	P2OUT |= 0x04;						\	
}

#define CS_CLR() (P2OUT &= ~0x04)
#define CS_SET() (P2OUT |= 0x04)

unsigned char * ade_readReg(uint8_t addr, uint8_t len) {

	unsigned int i;

	// Drive CS low
	CS_CLR();

	// Send register address
	SPI_WRITE(addr); // MSB is 0 for read

	// Read data
	for(i = 0; i < len; i++) {
		SPI_READ(ade_data[i]);
	}

	// Drive CS high
	CS_SET();
}

void ade_setReg(uint8_t addr, uint32_t val, uint8_t len) {
	
	unsigned int i;

	// Drive CS low
	CS_CLR();

	// Send register address
	SPI_WRITE(0x80 | addr);	// MSB is 1 for write

	// Send data to be written
	for(i = 0; i < len; i++) {
		SPI_WRITE_FAST(val[i]);
	}

	// Drive CS high
	CS_SET();
}

void ade_init() {
	// Initialize SPI
	spi_init();
	CS_INIT();
}