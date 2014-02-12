/*
 *	Created On:	January 6, 2014
 *		Author: Sam DeBruin
 */

#include "contiki.h"
#include "ade7753.h"
#include "sys/etimer.h"
#include "dev/leds.h"

static struct etimer periodic_timer;

/*---------------------------------------------------------------------------*/
PROCESS(ade_test_process, "ADE7753 Test Process");
AUTOSTART_PROCESSES(&ade_test_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ade_test_process, ev, data) {

	//unsigned int i;

	PROCESS_BEGIN();

	ade_init();

	etimer_set(&periodic_timer, CLOCK_SECOND);

	while(1) {
		PROCESS_YIELD();
		
		if (etimer_expired(&periodic_timer)) {
			ade_setReg(ADEREG_MODE, 0x000C, 2);
			ade_readReg(ADEREG_MODE, 2);
			ade_readReg(ADEREG_TEMP, 1);
			leds_toggle(LEDS_GREEN);
			etimer_restart(&periodic_timer);
		}
	}

	PROCESS_END();
}