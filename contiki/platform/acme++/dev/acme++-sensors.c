#include "contiki.h"
#include "dev/relay-button-sensor.h"

#include <string.h>

/** \brief Exports a global symbol to be used by the sensor API */
SENSORS(&relay_button_sensor);
