/*
 *	Created On:	January 6, 2014
 *		Author: Sam DeBruin
 */

#include "contiki.h"
#include "ade7753.h"

/*---------------------------------------------------------------------------*/
PROCESS(ade_test_process, "ADE7753 Test Process");
AUTOSTART_PROCESSES(&ade_test_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ade_test_process, ev, data) {

	unsigned int i;

	PROCESS_BEGIN();

	ade_init();

	for(i = 0; i < 1000; i++) {
		ade_readReg(ADEREG_AENERGY, 3);
	}

	PROCESS_END();
}