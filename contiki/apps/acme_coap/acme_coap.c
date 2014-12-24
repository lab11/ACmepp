/**
 * App for making ACme++ a wireless switch with an auto-turn-back-on function
 */
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

#define SW_VERSION "1.0"
#define HW_VERSION "B"

static uip_ipaddr_t gatd_ip;
static struct simple_udp_connection udp_conn;

fram_config_t config;


PROCESS(acme, "ACme++ With CoAP Support");
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
 * onoffdevice
 ******************************************************************************/

static void
onoffdevice_get_handler(void *request,
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
onoffdevice_post_handler(void *request,
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

RESOURCE(coap_onoffdevice,
         "title=\"onoffdevice\";rt=\"AC Relay\"",
         onoffdevice_get_handler,
         onoffdevice_post_handler,
         onoffdevice_post_handler,
         NULL);


/*******************************************************************************
 * onoffdevice/Power
 ******************************************************************************/

static void
onoffdevice_power_get_handler(void *request,
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
onoffdevice_power_post_handler(void *request,
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

RESOURCE(coap_onoffdevice_power,
         "title=\"onoffdevice/Power\";rt=\"AC Relay\"",
         onoffdevice_power_get_handler,
         onoffdevice_power_post_handler,
         onoffdevice_power_post_handler,
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
  char res[] = "Voltage=%u";

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
  char res[] = "Power=%i";

  int32_t  power   = 0;

  length = snprintf((char*) buffer, REST_MAX_CHUNK_SIZE, res, power);

  REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
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
  char res[] = "Period=%u";

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

  rest_activate_resource(&coap_onoffdevice,        "onoffdevice");
  rest_activate_resource(&coap_onoffdevice_power,  "onoffdevice/Power");

  rest_activate_resource(&coap_led,                "led");
  rest_activate_resource(&coap_led_power,          "led/Power");

  rest_activate_resource(&coap_powermeter,         "powermeter");
  rest_activate_resource(&coap_powermeter_voltage, "powermeter/Voltage");
  rest_activate_resource(&coap_powermeter_power,   "powermeter/Power");
  rest_activate_resource(&coap_powermeter_period,  "powermeter/Period");

  rest_activate_resource(&coap_device,             "device");
  rest_activate_resource(&coap_device_gatdip,      "device/GATD_IP");
  rest_activate_resource(&coap_device_software_version, "device/software/Version");
  rest_activate_resource(&coap_device_hardware_version, "device/hardware/Version");


  while (1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}
