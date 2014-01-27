#include "contiki.h"
#include "dev/nvic.h"
#include "dev/ioc.h"
#include "dev/gpio.h"
#include "dev/relay-button-sensor.h"
#include "sys/timer.h"

#include <stdint.h>
#include <string.h>

#define RELAY_BUTTON_PORT_BASE  GPIO_PORT_TO_BASE(RELAY_BUTTON_PORT)
#define RELAY_BUTTON_PIN_MASK   GPIO_PIN_MASK(RELAY_BUTTON_PIN)
/*---------------------------------------------------------------------------*/
static struct timer debouncetimer;
/*---------------------------------------------------------------------------*/
/**
 * \brief Common initialiser for all buttons
 * \param port_base GPIO port's register offset
 * \param pin_mask Pin mask corresponding to the button's pin
 */
static void
config(uint32_t port_base, uint32_t pin_mask)
{
  /* Software controlled */
  GPIO_SOFTWARE_CONTROL(port_base, pin_mask);

  /* Set pin to input */
  GPIO_SET_INPUT(port_base, pin_mask);

  /* Enable edge detection */
  GPIO_DETECT_EDGE(port_base, pin_mask);

  /* Single edge */
  GPIO_TRIGGER_SINGLE_EDGE(port_base, pin_mask);

  /* Trigger interrupt on Falling edge */
  GPIO_DETECT_RISING(port_base, pin_mask);

  GPIO_ENABLE_INTERRUPT(port_base, pin_mask);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Callback registered with the GPIO module. Gets fired with a button
 * port/pin generates an interrupt
 * \param port The port number that generated the interrupt
 * \param pin The pin number that generated the interrupt. This is the pin
 * absolute number (i.e. 0, 1, ..., 7), not a mask
 */
static void
btn_callback(uint8_t port, uint8_t pin)
{
  /* Check if the debounce timer is still active. If so, skip this button
     press. */
  if (!timer_expired(&debouncetimer)) {
    return;
  }

  /* Start the timer to ignore any upcoming bounces */
  timer_set(&debouncetimer, CLOCK_SECOND / 8);

  /* Make sure that we should notify upper level applications */
  if (port == RELAY_BUTTON_PORT && pin == RELAY_BUTTON_PIN) {
    sensors_changed(&relay_button_sensor);
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Init function for the select button.
 *
 * Parameters are ignored. They have been included because the prototype is
 * dictated by the core sensor api. The return value is also not required by
 * the API but otherwise ignored.
 *
 * \param type ignored
 * \param value ignored
 * \return ignored
 */
static int
config_relay_button(int type, int value)
{
  config(RELAY_BUTTON_PORT_BASE, RELAY_BUTTON_PIN_MASK);

  ioc_set_over(RELAY_BUTTON_PORT, RELAY_BUTTON_PIN, IOC_OVERRIDE_PUE);

  nvic_interrupt_enable(RELAY_BUTTON_VECTOR);

  gpio_register_callback(btn_callback, RELAY_BUTTON_PORT, RELAY_BUTTON_PIN);
  return 1;
}
/*---------------------------------------------------------------------------*/
void
relay_button_sensor_init()
{
  timer_set(&debouncetimer, 0);
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(relay_button_sensor, BUTTON_SENSOR, NULL, config_relay_button, NULL);

/** @} */
