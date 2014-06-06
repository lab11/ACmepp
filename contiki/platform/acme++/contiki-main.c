/*
 * Copyright (c) 2012, Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * \addtogroup platform
 * @{
 *
 * \defgroup cc2538 The cc2538 Development Kit platform
 *
 * The cc2538DK is the new platform by Texas Instruments, based on the
 * cc2530 SoC with an ARM Cortex-M3 core.
 * @{
 *
 * \file
 *   Main module for the cc2538dk platform
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/leds.h"
#include "dev/sys-ctrl.h"
#include "dev/scb.h"
#include "dev/nvic.h"
#include "dev/uart.h"
#include "dev/watchdog.h"
#include "dev/ioc.h"
#include "dev/serial-line.h"
#include "dev/slip.h"
#include "dev/cc2538-rf.h"
#include "dev/udma.h"
#include "usb/usb-serial.h"
#include "lib/random.h"
#include "net/netstack.h"
#include "net/queuebuf.h"
#include "net/ip/uip.h"
#include "net/ip/tcpip.h"
#include "net/mac/frame802154.h"
#include "cpu.h"
#include "reg.h"
#include "ieee-addr.h"
#include "lpm.h"
#include "spi-arch.h"
#include "spi.h"
#include "fm25lb.h"
#include "ade7753.h"
#include "relay-button-sensor.h"
#include "mac_timer.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/
static void
set_rf_params(void)
{
    uint16_t short_addr;
    uint8_t ext_addr[8];

    ieee_addr_cpy_to(ext_addr, 8);

    short_addr = ext_addr[7];
    short_addr |= ext_addr[6] << 8;

    /* Populate linkaddr_node_addr. Maintain endianness */
    memcpy(&linkaddr_node_addr, &ext_addr[8 - LINKADDR_SIZE], LINKADDR_SIZE);

#if STARTUP_CONF_VERBOSE
    {
        int i;
        printf("Rime configured with address ");
        for(i = 0; i < LINKADDR_SIZE - 1; i++) {
            printf("%02x:", linkaddr_node_addr.u8[i]);
        }
        printf("%02x\n", linkaddr_node_addr.u8[i]);
    }
#endif
    
    NETSTACK_RADIO.set_value(RADIO_PARAM_PAN_ID, IEEE802154_PANID);
    NETSTACK_RADIO.set_value(RADIO_PARAM_16BIT_ADDR, short_addr);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, CC2538_RF_CHANNEL);
    NETSTACK_RADIO.set_object(RADIO_PARAM_64BIT_ADDR, ext_addr, 8);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Main routine for the acme++ platform
 */
int
main(void)
{
  nvic_init();
  ioc_init();
  sys_ctrl_init();
  clock_init();
  lpm_init();
  rtimer_init();
  gpio_init();

  leds_init();
  leds_on(LEDS_ALL);

  process_init();

  watchdog_init();

  relay_button_sensor_init();

  spi_init();
  fm25lb_init();
  ade7753_init();

/*
#if UART_CONF_ENABLE
  uart_init();
  uart_set_input(serial_line_input_byte);
#endif

#if USB_SERIAL_CONF_ENABLE
  usb_serial_init();
  usb_serial_set_input(serial_line_input_byte);
#endif
*/
  serial_line_init();


  INTERRUPTS_ENABLE();

  /* Initialise the H/W RNG engine. */
  random_init(0);

  udma_init();

  process_start(&etimer_process, NULL);
  ctimer_init();

  set_rf_params();
  netstack_init();

#if UIP_CONF_IPV6
  memcpy(&uip_lladdr.addr, &linkaddr_node_addr, sizeof(uip_lladdr.addr));
  queuebuf_init();
  process_start(&tcpip_process, NULL);
#endif /* UIP_CONF_IPV6 */


  mac_timer_init();

  process_start(&sensors_process, NULL);

//  energest_init();
//  ENERGEST_ON(ENERGEST_TYPE_CPU);

  autostart_start(autostart_processes);

  watchdog_start();

  while(1) {
    uint8_t r;
    do {
      /* Reset watchdog and handle polls and events */
      watchdog_periodic();

      r = process_run();
    } while(r > 0);

    /* We have serviced all pending events. Enter a Low-Power mode. */
    lpm_enter();
  }
}
/*---------------------------------------------------------------------------*/

/**
 * @}
 * @}
 */
