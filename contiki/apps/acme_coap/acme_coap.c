/**
 * App for making ACme++ a wireless switch with an auto-turn-back-on function
 */
#include "settings.h"
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

#include "fm25lb.h"
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

// Maximum number of minutes to allow the load to be off.
#define FAILSAFE_TIME_MINUTES 1

#define ONE_MINUTE (60*CLOCK_SECOND) // Length of

static uip_ipaddr_t dest_addr;
static struct simple_udp_connection udp_conn;

// Timer that makes sure the ACme++ turns back on after a certain time.
static struct ctimer failsafe_timer;

fram_config_t config;


PROCESS(acme_switch, "ACme Failsafe");
AUTOSTART_PROCESSES(&acme_switch);


static void write_config ();
static void load_on ();
static void load_off ();
static void load_set (uint8_t on_off);


// When this is called we stopped receiving packets so turn the load
// back on!
static void failsafe_timer_callback (void *ptr) {
  static int failsafe_timer_counter = 0;

  // Increment the counter. Each counter is a minute.
  failsafe_timer_counter++;

  if (failsafe_timer_counter >= FAILSAFE_TIME_MINUTES) {
    // Ahh! Turn the load back on!
    load_on();
  } else {
    // Make the timer fire again
    ctimer_set(&failsafe_timer, ONE_MINUTE, failsafe_timer_callback, NULL);
  }
}


// Write the config struct back to the FRAM
static void write_config () {
  fm25lb_write(FRAM_ADDR_CONFIG, sizeof(fram_config_t), (uint8_t*) &config);
}

// Turn the plugged in load on
static void load_on () {
  leds_on(LEDS_BLUE);
  GPIO_SET_PIN(RELAY_CTRL_BASE, RELAY_CTRL_MASK);
  config.power_state = RELAY_ON;
  write_config();

  // Restart the watchdog on every command
  ctimer_restart(&failsafe_timer);
}

// Turn the plugged in load off
static void load_off () {
  leds_off(LEDS_BLUE);
  GPIO_CLR_PIN(RELAY_CTRL_BASE, RELAY_CTRL_MASK);
  config.power_state = RELAY_OFF;
  write_config();

  // Restart the watchdog on every command
  ctimer_restart(&failsafe_timer);
}

// Turn the plugged load to the given power state
static void load_set (uint8_t on_off) {
  if (on_off == RELAY_ON) load_on();
  else if (on_off == RELAY_OFF) load_off();
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

  leds_toggle(LEDS_GREEN);

  if (datalen == 0) return;

  command = data[0];

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


PROCESS_THREAD(acme_switch, ev, data) {
  PROCESS_BEGIN();

  leds_on(LEDS_ALL);

  // Read the FRAM
  fm25lb_read(FRAM_ADDR_CONFIG, sizeof(fram_config_t), (uint8_t*) &config);

  // Check if this is the first boot or not
  if (config.magic_id != MAGICID) {
    // First boot
    config.magic_id = MAGICID;
    config.power_state = RELAY_OFF;
    write_config();
  }

  // Configure the pin that sets the relay to be an output pin
  GPIO_SET_OUTPUT(RELAY_CTRL_BASE, RELAY_CTRL_MASK);
  // Put the load into its previous state
  load_set(config.power_state);

  // Setup the destination address
  uiplib_ipaddrconv(GATD_ADDR, &dest_addr);

  // Register a simple UDP socket
  simple_udp_register(&udp_conn, UDP_LISTEN_PORT, NULL, 0, receiver);

  // Start the watchdog
  ctimer_set(&failsafe_timer, ONE_MINUTE, failsafe_timer_callback, NULL);

  while (1) {
    PROCESS_YIELD();
  }

  PROCESS_END();
}
