#include "acme_coap.h"
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
#include "rest-engine.h"
#include "er-coap-engine.h"

#include "fm25lb.h"
#include "ade7753.h"
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

// History
// 1.0: CoAP added
// 2.0: Power metering and the ability to send message on state change
// 2.1: Change threshold for state change detection
#define SW_VERSION "2.1"
#define HW_VERSION "B"

static struct etimer periodic_power_sample;
coap_packet_t request[1];

static uip_ipaddr_t gatd_ip;
static struct simple_udp_connection udp_conn;

fram_config_t config;

// If not using the relay, track if the device is drawing power or not
uint8_t  load_energy_onoff = 2;
uint32_t load_energy;


PROCESS(acme, "ACme++ With CoAP Support");
PROCESS(acme_coap_client, "Issue CoAP POST");
AUTOSTART_PROCESSES(&acme);


static void write_config ();
static void load_on ();
static void load_off ();
static void load_set (uint8_t on_off);


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
}

// Turn the plugged in load off
static void load_off () {
  leds_off(LEDS_BLUE);
  GPIO_CLR_PIN(RELAY_CTRL_BASE, RELAY_CTRL_MASK);
  config.power_state = RELAY_OFF;
  write_config();
}

// Turn the plugged load to the given power state
static void load_set (uint8_t on_off) {
  if (on_off == RELAY_ON) load_on();
  else if (on_off == RELAY_OFF) load_off();
}

int
coap_parse_uint (void* request, uint32_t* u)
{
  int length;
  const char* payload = NULL;

  length = REST.get_request_payload(request, (const uint8_t**) &payload);

  if (length > 0) {
    *u = strtol(payload, NULL, 10);
    return 0;
  }

  return -1;
}

void
client_chunk_handler(void *response)
{

}




static void
state_changed (struct pt* process_pt, process_event_t ev) {

  leds_toggle(LEDS_RED);

  // Check that we have someone to tell
  if (config.notify_statechange_ipaddr.u8[0] != 0 &&
      config.notify_statechange_url[0] != 0) {

    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, config.notify_statechange_url);

    if (load_energy_onoff) {
      coap_set_payload(request, (uint8_t*) "true", 4);
    } else {
      coap_set_payload(request, (uint8_t*) "false", 5);
    }

    process_start(&acme_coap_client, NULL);

  }

}

// Pass this magic `process_pt` around everywhere. It comes from the contiki
// macros and you just have to trust it will be there.
static void
periodic_sample (struct pt* process_pt, process_event_t ev) {

  load_energy = ade7753_getActiveEnergy();

  // Check for a state change
  uint8_t current_state = 0;
  if (load_energy > config.notify_statechange_threshold) {
    current_state = 1;
  }

  if (load_energy_onoff == 2) {
    // unknown at startup
    load_energy_onoff = current_state;
    state_changed(process_pt, ev);
  } else if (load_energy_onoff == 1) {
    load_energy_onoff = current_state;
    if (current_state == 0) {
      state_changed(process_pt, ev);
    }
  } else {
    load_energy_onoff = current_state;
    if (current_state == 1) {
      state_changed(process_pt, ev);
    }
  }

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

#define TO_CHAR(X) (((X) < 10) ? ('0' + (X)) : ('a' + ((X) - 10)))
int inet_ntop6 (const uip_ipaddr_t *addr, char *buf, int cnt) {
  uint16_t block, block2;
  char *end = buf + cnt;
  int i, j, compressed = 0;

  for (j = 0; j < 8; j++) {
    if (buf > end - 8)
      goto done;

    //block = ntohs(addr->s6_addr16[j]);
    block = (addr->u8[j*2] << 8) + addr->u8[(j*2) + 1];
    block2 = block;
    for (i = 4; i <= 16; i+=4) {
      if (block > (0xffff >> i) || (compressed == 2 && i == 16)) {
        *buf++ = TO_CHAR((block >> (16 - i)) & 0xf);
      }
    }
    if (block2 == 0 && compressed == 0) {
      *buf++ = ':';
      compressed++;
    }
    if (block2 != 0 && compressed == 1) compressed++;

    if (j < 7 && compressed != 1) *buf++ = ':';
  }
  if (compressed == 1)
    *buf++ = ':';
 done:
  *buf++ = '\0';
  return buf - (end - cnt);
}



/*******************************************************************************
 * onoff
 ******************************************************************************/

static void
onoff_get_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, "Power=%s", (config.power_state)?"true":"false");

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

static void
onoff_post_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;
  const char* Power = NULL;

  length = REST.get_post_variable(request, "Power", &Power);
  if (length > 0) {
    if (strncmp(Power, "true", length) == 0) {
      load_on();
    } else if (strncmp(Power, "false", length) == 0) {
      load_off();
    } else {
      REST.set_response_status(response, REST.status.BAD_REQUEST);
    }
  } else {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  }
}

RESOURCE(coap_onoff,
         "title=\"onoffdevice\";rt=\"AC Relay\"",
         onoff_get_handler,
         onoff_post_handler,
         onoff_post_handler,
         NULL);


/*******************************************************************************
 * onoff/Power
 ******************************************************************************/

static void
onoff_power_get_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, "%s", (config.power_state)?"true":"false");

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

static void
onoff_power_post_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;
  const char* payload = NULL;

  length = REST.get_request_payload(request, (const uint8_t**) &payload);
  if (length > 0) {
    if (strncmp(payload, "true", length) == 0) {
      load_on();
    } else if (strncmp(payload, "false", length) == 0) {
      load_off();
    } else {
      REST.set_response_status(response, REST.status.BAD_REQUEST);
    }
  } else {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  }
}

RESOURCE(coap_onoff_power,
         "title=\"onoffdevice/Power\";rt=\"AC Relay\"",
         onoff_power_get_handler,
         onoff_power_post_handler,
         onoff_power_post_handler,
         NULL);

/*******************************************************************************
 * led
 ******************************************************************************/

static void
led_get_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, "Power=%s",
    ((leds_get()&LEDS_RED)==LEDS_RED)?"true":"false");

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

static void
led_post_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;
  const char* Power = NULL;

  length = REST.get_post_variable(request, "Power", &Power);
  if (length > 0) {
    if (strncmp(Power, "true", length) == 0) {
      leds_on(LEDS_RED);
    } else if (strncmp(Power, "false", length) == 0) {
      leds_off(LEDS_RED);
    } else {
      REST.set_response_status(response, REST.status.BAD_REQUEST);
    }
  } else {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  }
}

RESOURCE(coap_led,
         "title=\"onoffdevice\";rt=\"AC Relay\"",
         led_get_handler,
         led_post_handler,
         led_post_handler,
         NULL);



/*******************************************************************************
 * led/Power
 ******************************************************************************/

static void
led_power_get_handler(void *request,
                       void *response,
                       uint8_t *buffer,
                       uint16_t preferred_size,
                       int32_t *offset) {
  int length;

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, "%s",
    ((leds_get()&LEDS_RED)==LEDS_RED)?"true":"false");

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

// set the relay
static void
led_power_post_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;
  const char* payload = NULL;

  length = REST.get_request_payload(request, (const uint8_t**) &payload);
  if (length > 0) {
    if (strncmp(payload, "true", length) == 0) {
      leds_on(LEDS_RED);
    } else if (strncmp(payload, "false", length) == 0) {
      leds_off(LEDS_RED);
    } else {
      REST.set_response_status(response, REST.status.BAD_REQUEST);
    }
  } else {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  }
}

/* A simple actuator example. Toggles the red led */
RESOURCE(coap_led_power,
         "title=\"Red LED\";rt=\"Control\"",
         led_power_get_handler,
         led_power_post_handler,
         led_power_post_handler,
         NULL);

/*******************************************************************************
 * powermeter
 ******************************************************************************/

static void
powermeter_get_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;
  char res[] = "Power=%i&Voltage=%u&Period=%u";

  int32_t  power   = 0;
  uint32_t voltage = ade7753_getMaxVoltage();
  uint32_t period  = ade7753_readReg(ADEREG_PERIOD);

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, res, power, voltage, period);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

RESOURCE(coap_powermeter,
         "title=\"powermeter\"",
         powermeter_get_handler,
         NULL,
         NULL,
         NULL);

/*******************************************************************************
 * powermeter/Voltage
 ******************************************************************************/

static void
powermeter_voltage_get_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset)
{
  int length;
  char res[] = "%u";

  uint32_t voltage = ade7753_getMaxVoltage();

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, res, voltage);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

RESOURCE(coap_powermeter_voltage,
         "title=\"powermeter/Voltage\"",
         powermeter_voltage_get_handler,
         NULL,
         NULL,
         NULL);


/*******************************************************************************
 * powermeter/Power
 ******************************************************************************/

static void
powermeter_power_get_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset)
{
  int length;
  char res[] = "%i";

  int32_t power = load_energy;

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, res, power);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

RESOURCE(coap_powermeter_power,
         "title=\"powermeter/Power\"",
         powermeter_power_get_handler,
         NULL,
         NULL,
         NULL);

/*******************************************************************************
 * powermeter/Period
 ******************************************************************************/

static void
powermeter_period_get_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset)
{
  int length;
  char res[] = "%u";

  uint32_t period  = ade7753_readReg(ADEREG_PERIOD);

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, res, period);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

RESOURCE(coap_powermeter_period,
         "title=\"powermeter/Period\"",
         powermeter_period_get_handler,
         NULL,
         NULL,
         NULL);



/*******************************************************************************
 * notify/statechange/Threshold
 ******************************************************************************/

static void
notify_statechange_threshold_get_handler(void *request,
                       void *response,
                       uint8_t *buffer,
                       uint16_t preferred_size,
                       int32_t *offset) {
  int length;

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, "%u", (unsigned int) config.notify_statechange_threshold);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

static void
notify_statechange_threshold_post_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {

  uint32_t u;
  int ret;

  ret = coap_parse_uint(request, &u);

  if (ret < 0) {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  } else {
    config.notify_statechange_threshold = u;
  }
}

RESOURCE(coap_notify_statechange_threshold,
         "title=\"Note State Change\";rt=\"Notify\"",
         notify_statechange_threshold_get_handler,
         notify_statechange_threshold_post_handler,
         notify_statechange_threshold_post_handler,
         NULL);

/*******************************************************************************
 * notify/statechange/IPAddr
 ******************************************************************************/

static void
notify_statechange_ipaddr_get_handler(void *request,
                       void *response,
                       uint8_t *buffer,
                       uint16_t preferred_size,
                       int32_t *offset) {
  int length;
  char res[] = "%s";
  char ipbuf[40];

  inet_ntop6(&config.notify_statechange_ipaddr, ipbuf, 40);
  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, res, ipbuf);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

static void
notify_statechange_ipaddr_post_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;
  const char* payload = NULL;

  length = REST.get_request_payload(request, (const uint8_t**) &payload);
  if (length > 0) {
    uip_ip6addr_t new_ip;
    int ret;

    ret = uiplib_ip6addrconv(payload, &new_ip);

    if (ret) {
      memcpy(config.notify_statechange_ipaddr.u8, new_ip.u8, sizeof(uip_ip6addr_t));
      write_config();
    } else {
      REST.set_response_status(response, REST.status.BAD_REQUEST);
    }
  } else {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  }
}

RESOURCE(coap_notify_statechange_ipaddr,
         "title=\"Note State Change\";rt=\"Notify\"",
         notify_statechange_ipaddr_get_handler,
         notify_statechange_ipaddr_post_handler,
         notify_statechange_ipaddr_post_handler,
         NULL);


/*******************************************************************************
 * notify/statechange/URL
 ******************************************************************************/

static void
notify_statechange_url_get_handler(void *request,
                       void *response,
                       uint8_t *buffer,
                       uint16_t preferred_size,
                       int32_t *offset) {
  int length;

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, "%s", config.notify_statechange_url);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

static void
notify_statechange_url_post_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;
  const char* payload = NULL;

  length = REST.get_request_payload(request, (const uint8_t**) &payload);
  if (length > 0) {
    strncpy(config.notify_statechange_url, payload, 40);
    write_config();
  } else {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  }
}

RESOURCE(coap_notify_statechange_url,
         "title=\"Note State Change URL\";rt=\"Notify\"",
         notify_statechange_url_get_handler,
         notify_statechange_url_post_handler,
         notify_statechange_url_post_handler,
         NULL);



/*******************************************************************************
 * device
 ******************************************************************************/

static void
device_get_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;
  char res[] = "GATD_IP=%s&software/Version=%s&hardware/Version=%s";
  char gatd[40];

  inet_ntop6(&gatd_ip, gatd, 40);

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, res, gatd, SW_VERSION, HW_VERSION);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

static void
device_post_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;
  const char* GatdIp = NULL;

  length = REST.get_post_variable(request, "GATD_IP", &GatdIp);
  if (length > 0) {
    uip_ip6addr_t new_gatd_ip;
    int ret;

    ret = uiplib_ip6addrconv(GatdIp, &new_gatd_ip);

    if (ret) {
      memcpy(gatd_ip.u8, new_gatd_ip.u8, sizeof(uip_ip6addr_t));
    } else {
      REST.set_response_status(response, REST.status.BAD_REQUEST);
    }
  } else {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  }
}

RESOURCE(coap_device,
         "title=\"device\"",
         device_get_handler,
         device_post_handler,
         device_post_handler,
         NULL);


/*******************************************************************************
 * device/GATD_IP
 ******************************************************************************/

static void
device_gatdip_get_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;
  char res[] = "%s";
  char gatd[40];

  inet_ntop6(&gatd_ip, gatd, 40);

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, res, gatd);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

static void
device_gatdip_post_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;
  const char* payload = NULL;

  length = REST.get_request_payload(request, (const uint8_t**) &payload);
  if (length > 0) {
    uip_ip6addr_t new_gatd_ip;
    int ret;

    ret = uiplib_ip6addrconv(payload, &new_gatd_ip);

    if (ret) {
      memcpy(gatd_ip.u8, new_gatd_ip.u8, sizeof(uip_ip6addr_t));
    } else {
      REST.set_response_status(response, REST.status.BAD_REQUEST);
    }
  } else {
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  }
}

RESOURCE(coap_device_gatdip,
         "title=\"device/GATD_IP\";rt=\"gatd\"",
         device_gatdip_get_handler,
         device_gatdip_post_handler,
         device_gatdip_post_handler,
         NULL);



/*******************************************************************************
 * device/software/Version
 ******************************************************************************/

static void
device_software_version_get_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;
  char res[] = "%s";

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, res, SW_VERSION);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

RESOURCE(coap_device_software_version,
         "title=\"device/software/Version\";rt=\"sw\"",
         device_software_version_get_handler,
         NULL,
         NULL,
         NULL);


/*******************************************************************************
 * device/hardware/Version
 ******************************************************************************/

static void
device_hardware_version_get_handler(void *request,
                  void *response,
                  uint8_t *buffer,
                  uint16_t preferred_size,
                  int32_t *offset) {
  int length;
  char res[] = "%s";

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, res, HW_VERSION);

  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  REST.set_response_payload(response, buffer, length);
}

RESOURCE(coap_device_hardware_version,
         "title=\"device/hardware/Version\";rt=\"hw\"",
         device_hardware_version_get_handler,
         NULL,
         NULL,
         NULL);



PROCESS_THREAD(acme, ev, data) {
  PROCESS_BEGIN();

  leds_on(LEDS_ALL);

  // Read the FRAM
  fm25lb_read(FRAM_ADDR_CONFIG, sizeof(fram_config_t), (uint8_t*) &config);

  // Check if this is the first boot or not
  if (config.magic_id != MAGICID) {
    // First boot
    config.magic_id = MAGICID;
    config.power_state = RELAY_OFF;
    memset(&config.notify_statechange_ipaddr, 0, sizeof(uip_ip6addr_t));
    memset(config.notify_statechange_url, 0, 40);
    config.notify_statechange_threshold = 2; // 2 watts by default
    write_config();
  }

  // Configure the pin that sets the relay to be an output pin
  GPIO_SET_OUTPUT(RELAY_CTRL_BASE, RELAY_CTRL_MASK);
  // Put the load into its previous state
  load_set(config.power_state);

  // Setup the destination address
  uiplib_ipaddrconv(GATD_ADDR, &gatd_ip);

  // Register a simple UDP socket
  //simple_udp_register(&udp_conn, UDP_LISTEN_PORT, NULL, 0, receiver);

  // CoAP + REST
  rest_init_engine();

  rest_activate_resource(&coap_onoff,                        "onoff");
  rest_activate_resource(&coap_onoff_power,                  "onoff/Power");

  rest_activate_resource(&coap_led,                          "led");
  rest_activate_resource(&coap_led_power,                    "led/Power");

  rest_activate_resource(&coap_powermeter,                   "powermeter");
  rest_activate_resource(&coap_powermeter_voltage,           "powermeter/Voltage");
  rest_activate_resource(&coap_powermeter_power,             "powermeter/Power");
  rest_activate_resource(&coap_powermeter_period,            "powermeter/Period");

  rest_activate_resource(&coap_notify_statechange_threshold, "notify/statechange/Threshold");
  rest_activate_resource(&coap_notify_statechange_ipaddr,    "notify/statechange/IPAddr");
  rest_activate_resource(&coap_notify_statechange_url,       "notify/statechange/URL");

  rest_activate_resource(&coap_device,                       "device");
  rest_activate_resource(&coap_device_gatdip,                "device/GATD_IP");
  rest_activate_resource(&coap_device_software_version,      "device/software/Version");
  rest_activate_resource(&coap_device_hardware_version,      "device/hardware/Version");

  // CoAP Client
  coap_init_engine();

  etimer_set(&periodic_power_sample, CLOCK_SECOND);

  while (1) {
    PROCESS_WAIT_EVENT();

    if (etimer_expired(&periodic_power_sample)) {
      periodic_sample(process_pt, ev);
      etimer_restart(&periodic_power_sample);
    }
  }

  PROCESS_END();
}


// Process / protothread / whatever for making coap requests
PROCESS_THREAD(acme_coap_client, ev, data) {
  PROCESS_BEGIN();

  // This HAS to be in a PROCESS_THREAD block.
  // You can ask Pat or Adam Dunkels why. It should be noted that Pat does
  // not support this, endorse this, or think this is even sort of a good idea.
  // But he understands what is happening.
  COAP_BLOCKING_REQUEST(&config.notify_statechange_ipaddr,
                        UIP_HTONS(COAP_DEFAULT_PORT),
                        request,
                        client_chunk_handler);

  PROCESS_END();
}




