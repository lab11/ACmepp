/*
*	Created On:	January 6, 2014
*		Author: Sam DeBruin
*/

#ifndef ADE7753_H
#define ADE7753_H

// Register addresses
#define ADEREG_WAVEFORM		0x01
#define ADEREG_AENERGY		0X02
#define ADEREG_RAENERGY		0x03
#define ADEREG_LAENERGY		0x04
#define ADEREG_VAENERGY		0x05
#define ADEREG_RVAENERGY	0x06
#define ADEREG_LVAENERGY	0x07
#define ADEREG_LVARENERGY	0x08
#define ADEREG_MODE			0x09
#define ADEREG_IRQEN		0x0A
#define ADEREG_STATUS		0x0B
#define ADEREG_RSTSTATUS	0x0C
#define ADEREG_CH1OS		0x0D
#define ADEREG_CH2OS		0x0E
#define ADEREG_GAIN			0x0F
#define ADEREG_PHCAL		0x10
#define ADEREG_APOS			0x11
#define ADEREG_WGAIN		0x12
#define ADEREG_WDIV			0x13
#define ADEREG_CFNUM		0x14
#define ADEREG_CFDEN		0x15
#define ADEREG_IRMS			0x16
#define ADEREG_VRMS			0x17
#define ADEREG_IRMSOS		0x18
#define ADEREG_VRMSOS		0x19
#define ADEREG_VAGAIN		0x1A
#define ADEREG_VADIV		0x1B
#define ADEREG_LINECYC		0x1C
#define	ADEREG_ZXTOUT		0x1D
#define ADEREG_SAGCYC		0x1E
#define ADEREG_SAGLVL		0x1F
#define ADEREG_IPKLVL		0x20
#define ADEREG_VPKLVL		0x21
#define ADEREG_IPEAK		0x22
#define ADEREG_RSTIPEAK		0x23
#define ADEREG_VPEAK		0x24
#define ADEREG_RSTVPEAK		0x25
#define ADEREG_TEMP			0x26
#define ADEREG_PERIOD		0x27
#define ADEREG_TMODE		0x3D
#define ADEREG_CHKSUM		0x3E
#define ADEREG_DIEREV		0x3F

// ADE7753 Mode Register
#define ADEMODE_DISHPF		0x0001
#define ADEMODE_DISLPF2		0x0002
#define ADEMODE_DISCF		0x0004
#define ADEMODE_DISSAG		0x0008
#define ADEMODE_ASUSPEND	0x0010
#define ADEMODE_TEMPSEL		0x0020
#define ADEMODE_SWRST		0x0040
#define ADEMODE_CYCMODE		0x0080
#define ADEMODE_DISCH1		0x0100
#define ADEMODE_DISCH2		0x0200
#define ADEMODE_SWAP		0x0400
#define ADEMODE_DTRT_0		0x0000
#define ADEMODE_DTRT_1		0x0800
#define ADEMODE_DTRT_2		0x1000
#define ADEMODE_DTRT_3		0x1800

// ADE7753 Interrupt Registers
#define ADEINT_AEHF			0x0001
#define ADEINT_SAG			0x0002
#define ADEINT_CYCEND		0x0004
#define ADEINT_WSMP			0x0008
#define ADEINT_ZX			0x0010
#define ADEINT_TEMP			0x0020
#define ADEINT_RESET		0x0040
#define ADEINT_AEOF			0x0080
#define ADEINT_PKV			0x0100
#define ADEINT_PKI			0x0200
#define ADEINT_VAEHF		0x0400
#define ADEINT_VAEOF		0x0800
#define ADEINT_ZXTO			0x1000
#define ADEINT_PPOS			0x2000
#define ADEINT_PNEG			0x4000

void ade7753_init();
void ade7753_configure_mode();
uint32_t ade7753_readReg(uint8_t addr);
void ade7753_setReg(uint8_t addr, uint32_t val);

uint32_t ade7753_getMaxVoltage();
uint32_t ade7753_getActiveEnergy();


#endif /* ADE7753_H */