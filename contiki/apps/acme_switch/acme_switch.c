/**
 * \addtogroup cc2538
 * @{
 *
 *
 * @{
 *
 * \file
 *     Example demonstrating the cc2538dk platform
 */
#include "coilcube_ip.h"
#include "contiki-net.h"
#include "contiki.h"
#include "cpu.h"
#include "sys/etimer.h"
#include "sys/rtimer.h"
#include "dev/leds.h"
#include "dev/uart.h"
//#include "dev/relay-button-sensor.h"
#include "dev/watchdog.h"
#include "dev/serial-line.h"
#include "dev/sys-ctrl.h"
#include "net/rime/broadcast.h"
#include "net/ip/uip.h"
#include "net/ip/uip-udp-packet.h"
#include "net/ip/uiplib.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include <stdio.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
#define LOOP_INTERVAL       CLOCK_SECOND
#define LEDS_OFF_HYSTERISIS (RTIMER_SECOND >> 1)
#define LEDS_PERIODIC       LEDS_YELLOW
#define LEDS_BUTTON         LEDS_RED
#define LEDS_SERIAL_IN      LEDS_ORANGE
#define LEDS_REBOOT         LEDS_ALL
#define LEDS_RF_RX          (LEDS_YELLOW | LEDS_ORANGE)
#define BROADCAST_CHANNEL   129
/*---------------------------------------------------------------------------*/

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",lladdr->addr[0], lladdr->addr[1], lladdr->addr[2], lladdr->addr[3],lladdr->addr[4], lladdr->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#endif


static uip_ipaddr_t dest_addr;
/*static uip_ipaddr_t bcast_ipaddr;
static uip_lladdr_t bcast_lladdr = {{0, 0, 0, 0, 0, 0, 0, 0}};
static struct uip_udp_conn *client_conn;*/

//pkt_data_t pkt_data = {PROFILE_ID, SEHNSOR_VERSION, 0, 0};

static struct etimer periodic_timer;

static struct simple_udp_connection udp_conn;

PROCESS(acme_switch, "ACme Switch");
AUTOSTART_PROCESSES(&acme_switch);




// Turn the plugged in load on
static void load_on () {
  leds_on(LEDS_BLUE);
  GPIO_SET_PIN(RELAY_CTRL_BASE, RELAY_CTRL_MASK);
}

// Turn the plugged in load off
static void load_off () {
  leds_off(LEDS_BLUE);
  GPIO_CLR_PIN(RELAY_CTRL_BASE, RELAY_CTRL_MASK);
}

static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen) {

  uint8_t command;

  if (datalen == 0) return;

  command = data[0];

  simple_udp_sendto_port(&udp_conn,
    (uint8_t*) "aaabbbcccdddeeefffggghhfdsafdafdsafdbfdabffdsafdsafdsafdsa", 1,
    &dest_addr, UDP_REMOTE_PORT);

  switch (command) {
    case ACME_CMD_POWER_ON:
      load_on();
      break;

    case ACME_CMD_POWER_OFF:
      load_off();
      break;

    default:
      break;
  }
}


static void periodic () {
  leds_toggle(LEDS_RED);

  simple_udp_sendto_port(&udp_conn,
    (uint8_t*) "aaabbbcccdddeeefffggghhfdsafdafdsafdbfdabffdsafdsafdsafdsa", 25,
    &dest_addr, UDP_REMOTE_PORT);
}


/*---------------------------------------------------------------------------*/
/*static void
send_handler(process_event_t ev, process_data_t data) {
  pkt_data.counter++;
  pkt_data.seq_no++;

  leds_toggle(LEDS_BLUE);



  // control relay
  if (pkt_data.seq_no & 0x1) {
    GPIO_CLR_PIN(RELAY_CTRL_BASE, RELAY_CTRL_MASK);
    leds_off(LEDS_RED);
  } else {
    GPIO_SET_PIN(RELAY_CTRL_BASE, RELAY_CTRL_MASK);
    leds_on(LEDS_RED);
  }

  simple_udp_send(&udp_conn, (uint8_t*) &pkt_data, sizeof(pkt_data_t));
}*/



PROCESS_THREAD(acme_switch, ev, data) {


  PROCESS_BEGIN();

  leds_off(LEDS_GREEN);
  leds_on(LEDS_RED);
  leds_off(LEDS_BLUE);


  GPIO_SET_OUTPUT(RELAY_CTRL_BASE, RELAY_CTRL_MASK);

  // Setup the destination address
  uiplib_ipaddrconv(RECEIVER_ADDR, &dest_addr);

  // Register a simple UDP socket
  simple_udp_register(&udp_conn, UDP_LISTEN_PORT, NULL, 0, receiver);

  etimer_set(&periodic_timer, 10*CLOCK_SECOND);

  while(1) {
    PROCESS_YIELD();

    if (etimer_expired(&periodic_timer)) {
      periodic();
      etimer_restart(&periodic_timer);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/