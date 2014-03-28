
#include <stdint.h>

#include "reg.h"
#include "dev/rfcore-sfr.h"
#include "mac_timer.h"

#define RFCORE_SFR_MTMSEL_MTMSEL_MTtim         0x00000000
#define RFCORE_SFR_MTMSEL_MTMSEL_MTcap         0x00000001
#define RFCORE_SFR_MTMSEL_MTMSEL_MTper         0x00000002
#define RFCORE_SFR_MTMSEL_MTMSEL_MTcmp1        0x00000003
#define RFCORE_SFR_MTMSEL_MTMSEL_MTcmp2        0x00000004

#define RFCORE_SFR_MTMSEL_MTMOVFSEL_MTovf      0x00000000
#define RFCORE_SFR_MTMSEL_MTMOVFSEL_MTovf_cap  0x00000010
#define RFCORE_SFR_MTMSEL_MTMOVFSEL_MTovf_per  0x00000020
#define RFCORE_SFR_MTMSEL_MTMOVFSEL_MTovf_cmp1 0x00000030
#define RFCORE_SFR_MTMSEL_MTMOVFSEL_MTovf_cmp2 0x00000040

void mac_timer_init () {
  // Start the mac timer
  REG(RFCORE_SFR_MTCTRL) = RFCORE_SFR_MTCTRL_RUN | RFCORE_SFR_MTCTRL_LATCH_MODE;
}

uint64_t read_mac_timer () {
  uint64_t ret;
  ret  = (uint64_t)  ((uint64_t) (REG(RFCORE_SFR_MTM0) & 0xFF));
  ret |= (uint64_t) (((uint64_t) (REG(RFCORE_SFR_MTM1) & 0xFF)) << 8);
  ret |= (uint64_t) (((uint64_t) (REG(RFCORE_SFR_MTMOVF0) & 0xFF)) << 16);
  ret |= (uint64_t) (((uint64_t) (REG(RFCORE_SFR_MTMOVF1) & 0xFF)) << 24);
  ret |= (uint64_t) (((uint64_t) (REG(RFCORE_SFR_MTMOVF2) & 0xFF)) << 32);
  return ret;
}

uint64_t mac_timer_get () {
  // Configure the mac timer to read the timer value and latch overflow at
  // the same time.
  REG(RFCORE_SFR_MTMSEL)  = RFCORE_SFR_MTMSEL_MTMSEL_MTtim | RFCORE_SFR_MTMSEL_MTMOVFSEL_MTovf;
  REG(RFCORE_SFR_MTCTRL) |= RFCORE_SFR_MTCTRL_LATCH_MODE;

  return read_mac_timer();
}

uint64_t mac_timer_get_sfd () {
  // Configure mac timer to read the SFD capture register
  REG(RFCORE_SFR_MTMSEL) = (RFCORE_SFR_MTMSEL_MTMSEL_MTcap | RFCORE_SFR_MTMSEL_MTMOVFSEL_MTovf_cap);
  REG(RFCORE_SFR_MTCTRL) |= RFCORE_SFR_MTCTRL_LATCH_MODE;

  return read_mac_timer();
}

