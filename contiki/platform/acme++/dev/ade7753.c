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
#include "spi-arch.h"
#include "spi.h"


uint8_t ADE7753_REG_LENGTHS[ADEREG_DIEREV+1] = { 0, 24, 24, 24, 24, 24, 24, 24,
                                                24, 16, 16, 16, 16,  8,  8,  8,
                                                 6, 16, 12,  8, 12, 12, 24, 24,
                                                12, 12, 12,  8, 16, 12,  8,  8,
                                                 8,  8, 24, 24, 24, 24,  8, 16,
                                                 0,  0,  0,  0,  0,  0,  0,  0,
                                                 0,  0,  0,  0,  0,  0,  0,  0,
                                                 0,  0,  0,  0,  0,  8,  6,  8};


/* Returns max voltage in millivolts */
uint32_t ade7753_getMaxVoltage() {
	uint32_t voltage_adc;
	uint64_t millivolts;


	voltage_adc = ade7753_readReg(ADEREG_RSTVPEAK);


	/* equation: ((((ADC/2)/MAX_ADC) * MAX_ADC_MV) * VOLTAGE_SCALER) / FILTER_ADJ
	 *           ((((ADC/2)/10322) * 500) * 500) / 0.919
	 *           (ADC * 62500000) / 4742959
	 */
	millivolts = (62500000 * (uint64_t) voltage_adc) / 4742959;

	return (uint32_t) millivolts;
}

uint32_t ade7753_getActiveEnergy() {
	uint32_t energy;

	energy = ade7753_readReg(ADEREG_RAENERGY);

	return energy;
}



void ade7753_configure_mode() {
  spi_set_mode(SSI_CR0_FRF_MOTOROLA, 0, SSI_CR0_SPH, 8);
}

uint32_t ade7753_readReg(uint8_t addr) {

	uint8_t i;
	uint8_t buf;
	uint8_t len;
	uint32_t ret = 0;

	ade7753_configure_mode();

	SPI_CS_CLR(ADE7753_CS_N_PORT_NUM, ADE7753_CS_N_PIN);

	if (addr > ADEREG_DIEREV) return 0;

	len = (ADE7753_REG_LENGTHS[addr]+7)/8;

	// Send register address
	SPI_WRITE(addr); // MSB is 0 for read
	SPI_FLUSH();

	for (i=0; i<len; i++) {
		SPI_READ(buf);
		ret |= (buf << ((len-i-1)*8));
	}

	SPI_CS_SET(ADE7753_CS_N_PORT_NUM, ADE7753_CS_N_PIN);

	return ret;
}

void ade7753_setReg(uint8_t addr, uint32_t val) {

	uint8_t i;
	uint8_t len;

	ade7753_configure_mode();

	SPI_CS_CLR(ADE7753_CS_N_PORT_NUM, ADE7753_CS_N_PIN);

	len = (ADE7753_REG_LENGTHS[addr]+7)/8;

	// Send register address
	SPI_WRITE(0x80 | addr);	// MSB is 1 for write

	// Send data to be written
	for(i = len; i > 0; i--) {
		SPI_WRITE(val >> (8*(i-1)));
	}

	SPI_CS_SET(ADE7753_CS_N_PORT_NUM, ADE7753_CS_N_PIN);


}

void ade7753_init() {

	ade7753_configure_mode();

	spi_cs_init(ADE7753_CS_N_PORT_NUM, ADE7753_CS_N_PIN);

	SPI_CS_CLR(ADE7753_CS_N_PORT_NUM, ADE7753_CS_N_PIN);

	// Set ADE Mode
	SPI_WRITE(0x80 | ADEREG_MODE);
	SPI_WRITE((ADEMODE_DISCF + ADEMODE_DISSAG + ADEMODE_TEMPSEL) >> 8);
	SPI_WRITE(0xFF & (ADEMODE_DISCF + ADEMODE_DISSAG + ADEMODE_TEMPSEL));



	// Set ADE Gains


	SPI_CS_SET(ADE7753_CS_N_PORT_NUM, ADE7753_CS_N_PIN);



}
