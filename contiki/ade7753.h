#ifndef ADE7753_H
#define ADE7753_H

extern unsigned char * ade_readReg(uint8_t addr, uint8_t len);
extern void ade_setReg(uint8_t addr, uint32_t val, uint8_t len);
extern void ade_init();

#endif /* ADE7753_H */ 