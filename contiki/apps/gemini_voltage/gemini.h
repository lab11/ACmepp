#ifndef __GEMINI_H__
#define __GEMINI_H__

#include <stdint.h>

// Constant to keep track of which revision of coilcube sent the packet.
// This will probably be useful to determine what wakeups and packets mean.
//#define SEHNSOR_VERSION 2

#define ADDR_ALL_ROUTERS "ff02::2"

#define RECEIVER_ADDR "2001:470:1f10:1320::2"
#define RECEIVER_PORT 4001

/*
#define PROFILE_ID "7aiOPJapXF"

// Prefix for this node, just set it blank as we have no idea what the prefix
// will be.
#define IN6_PREFIX "::"

#define FRAM_ADDR_COUNT  0
#define FRAM_ADDR_SEQ_NO 1

typedef struct {
//  ieee_eui64_t id;
  uint8_t counter;
  uint8_t seq_no;
} __attribute__((packed)) fram_data_t;

typedef struct {
  char profile[10]; // GATD profile ID
  uint8_t version;  // version of the coilcube
  uint8_t counter;  // number of wakeups of the coilcube
  uint8_t seq_no;   // copy of the 15.4 sequence number as this will be lost (udp)
} __attribute__((packed)) pkt_data_t;

typedef enum {
  STATE_INITIAL_READ, // Get the starting seq no, counter and id from FRAM
  STATE_INITIAL_READ_DONE,
  STATE_CHECK_PKT_DELAY, // Check how long it has been since a packet
  STATE_CHECK_PKT_DELAY_DONE,
  STATE_SEND_PACKET,
  STATE_SEND_PACKET_DONE,
  STATE_DONE
} cc_state_e;


*/

#define MAGICID 0xBD30

#define FRAM_ADDR_CONFIG 0

typedef struct {
  uint32_t magic_id;
  uint8_t power_state;
} fram_config_t;

typedef struct {
  uint32_t vpeak;              // max voltage of AC waveform
  uint32_t ticks_since_rising; // time from rising zero-crossing of AC signal to SFD
  uint16_t chksum_balance;     // 16 bits to compensate for checksum
  uint8_t  ending;             // Magic byte to identify these packets
} __attribute__ ((__packed__)) voltage_data_t;


#define RELAY_OFF 0
#define RELAY_ON  1

#define ACME_CMD_POWER_ON  1
#define ACME_CMD_POWER_OFF 2




#define UDP_LISTEN_PORT 47652
#define UDP_REMOTE_PORT 47653

#define UDP_PORT_VIRT_VOLT 39888

#endif
