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
#include "dev/nvic.h"
#include "dev/ioc.h"

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
static struct simple_udp_connection udp_conn_cc;





PROCESS(acme_switch, "NetworkTest");
AUTOSTART_PROCESSES(&acme_switch);




static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen) {


  if (datalen == 0) return;


  simple_udp_sendto_port(&udp_conn,
    (uint8_t*) "aaabbbcccdddeeefffggghhfdsafdafdsafdbfdabffdsafdsafdsafdsa", 1,
    sender_addr, UDP_PORT_VIRT_VOLT);


}

static void
cc_receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen) {


  leds_toggle(LEDS_BLUE);

  uip_lladdr_t new_neighbor_ll;
  uip_ipaddr_t s_addr;
  uip_ds6_route_t* r;

  memset(s_addr.u8, 0, 16);
  memcpy(s_addr.u8+8, sender_addr->u8+8, 8);

  // Add a "neighbor" for our custom route
  // Setup the default broadcast route
  //uiplib_ipaddrconv(ADDR_ALL_ROUTERS, &bcast_ipaddr);
  memcpy(new_neighbor_ll.addr, sender_addr->u8+8, 8);
  new_neighbor_ll.addr[0] ^= 0x02;
  uip_ds6_nbr_add(&s_addr, &new_neighbor_ll, 0, NBR_REACHABLE);
  r = uip_ds6_route_add(&s_addr, 128, &s_addr);

  if (r == NULL) {
    leds_toggle(LEDS_RED);
  }

  simple_udp_sendto_port(&udp_conn_cc,
    (uint8_t*) "abcdefg", 7,
    &s_addr, UDP_PORT_VIRT_VOLT);
}


static void periodic () {




  struct {
    uint32_t emtpy1;
    uint32_t emtpy2;
    uint32_t vpeak;              // max voltage of AC waveform
    uint32_t ticks_since_rising; // time from rising zero-crossing of AC signal to SFD
    uint16_t chksum_balance;     // 16 bits to compensate for checksum
    uint8_t  ending;             // Magic byte to identify these packets
  } __attribute__ ((__packed__)) pktdata;


 // leds_toggle(LEDS_RED);
//  simple_udp_sendto_port(&udp_conn,
//    (uint8_t*) &pktdata, sizeof(pktdata),
//    &dest_addr, 2333);
}




uint64_t get_zero_crossing () {
  return 0;
}


PROCESS_THREAD(acme_switch, ev, data) {


  PROCESS_BEGIN();

  leds_off(LEDS_ALL);
  leds_on(LEDS_RED);






  // Setup the destination address
  uiplib_ipaddrconv(RECEIVER_ADDR, &dest_addr);

  // Register a simple UDP socket
  simple_udp_register(&udp_conn, UDP_LISTEN_PORT, NULL, 0, receiver);

  // Register for UDP listener
  simple_udp_register(&udp_conn_cc, UDP_PORT_VIRT_VOLT, NULL, 0, cc_receiver);

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
