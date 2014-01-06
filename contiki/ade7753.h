/*
*	Created On:	January 6, 2014
*		Author: Sam DeBruin
*/

#ifndef ADE7753_H
#define ADE7753_H

// Register addresses
#define WAVEFORM			0x01
#define AENERGY				0X02
#define RAENERGY			0x03
#define LAENERGY			0x04
#define VAENERGY			0x05
#define RVAENERGY			0x06
#define LVAENERGY			0x07
#define LVARENERGY			0x08
#define MODE				0x09
#define IRQEN				0x0A
#define STATUS				0x0B
#define RSTSTATUS			0x0C
#define CH1OS				0x0D
#define CH2OS				0x0E
#define GAIN				0x0F
#define PHCAL				0x10
#define APOS				0x11
#define WGAIN				0x12
#define WDIV				0x13
#define CFNUM				0x14
#define CFDEN				0x15
#define IRMS				0x16
#define VRMS				0x17
#define IRMSOS				0x18
#define VRMSOS				0x19
#define VAGAIN				0x1A
#define VADIV				0x1B
#define LINECYC				0x1C
#define	ZXTOUT				0x1D
#define SAGCYC				0x1E
#define SAGLVL				0x1F
#define IPKLVL				0x20
#define VPKLVL				0x21
#define IPEAK				0x22
#define RSTIPEAK			0x23
#define VPEAK				0x24
#define RSTVPEAK			0x25
#define TEMP				0x26
#define PERIOD				0x27
#define TMODE				0x3D
#define CHKSUM				0x3E
#define DIEREV				0x3F

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

extern unsigned char * ade_readReg(uint8_t addr, uint8_t len);
extern void ade_setReg(uint8_t addr, uint32_t val, uint8_t len);
extern void ade_init();

#endif /* ADE7753_H */ 