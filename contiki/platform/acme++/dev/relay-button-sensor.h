
#ifndef RELAY_BUTTON_SENSOR_H_
#define RELAY_BUTTON_SENSOR_H_

#include "lib/sensors.h"
#include "dev/gpio.h"

#define BUTTON_SENSOR "RelayButton"

//#define button_sensor button_select_sensor
extern const struct sensors_sensor relay_button_sensor;
/*---------------------------------------------------------------------------*/

/** \brief Common initialiser for all buttons */
void relay_button_sensor_init();

#endif