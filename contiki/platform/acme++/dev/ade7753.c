/*
 *	ADE7753 Driver
 *
 *	Used for controlling the ADE7753 energy meter IC
 *
 *	Created On:	January 6, 2014
 *		Author: Sam DeBruin
 */

#include <string.h>

#include "contiki.h"
#include "ade7753.h"
#include "spi.h"
#include "spi-arch.h"

unsigned char ade_data[3];


void ade_configure_cs() {
	spi_configure_cs(ADE7753_CS_N_PORT_NUM, ADE7753_CS_N_PIN);
}

unsigned char* ade_readReg(uint8_t addr, uint8_t len) {

	unsigned int i;

	ade_configure_cs();

	// Send register address
	SPI_WRITE(addr); // MSB is 0 for read

	// Read data
	for(i = 0; i < len; i++) {
		SPI_READ(ade_data[i]);
	}

	return ade_data;


}

void ade_setReg(uint8_t addr, uint32_t val, uint8_t len) {

	unsigned int i;

	ade_configure_cs();

	// Send register address
	SPI_WRITE(0x80 | addr);	// MSB is 1 for write

	// Send data to be written
	for(i = len; i > 0; i--) {
		SPI_WRITE_FAST(val >> (8*(i-1)));
	}


}

void ade_init() {

	ade_configure_cs();

	// Set ADE Mode
	SPI_WRITE(0x80 | ADEREG_MODE);
	SPI_WRITE((ADEMODE_DISCF + ADEMODE_DISSAG) >> 8);
	SPI_WRITE(0xFF & (ADEMODE_DISCF + ADEMODE_DISSAG));

	// Set ADE Gains

}