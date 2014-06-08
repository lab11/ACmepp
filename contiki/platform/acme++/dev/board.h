/** \addtogroup cc2538
 * @{
 *
 * \defgroup acme++
 *
 * \note   Do not include this file directly. It gets included by contiki-conf
 *         after all relevant directives have been set.
 */
#ifndef BOARD_H_
#define BOARD_H_

#include "dev/gpio.h"
#include "dev/nvic.h"
/*---------------------------------------------------------------------------*/
/** \name SmartRF LED configuration
 *
 * LEDs on the SmartRF06 (EB and BB) are connected as follows:
 * - LED1 (Red)    -> PC0
 * - LED2 (Yellow) -> PC1
 * - LED3 (Green)  -> PC2
 * - LED4 (Orange) -> PC3
 *
 * LED1 shares the same pin with the USB pullup
 * @{
 */
/*---------------------------------------------------------------------------*/
/* Some files include leds.h before us, so we need to get rid of defaults in
 * leds.h before we provide correct definitions */
#undef LEDS_GREEN
#undef LEDS_YELLOW
#undef LEDS_RED
#undef LEDS_CONF_ALL

#define LEDS_BLUE                16 /**< PB4 */
#define LEDS_RED                 64 /**< PA6 */
#define LEDS_GREEN               128 /**< PA7 */
#define LEDS_CONF_ALL            208

/* Notify various examples that we have LEDs */
#define PLATFORM_HAS_LEDS        1

#define LED_BLUE_BASE            GPIO_B_BASE
#define LED_RED_BASE             GPIO_A_BASE
#define LED_GREEN_BASE           GPIO_A_BASE
#define LED_BLUE_MASK            GPIO_PIN_MASK(4)
#define LED_RED_MASK             GPIO_PIN_MASK(6)
#define LED_GREEN_MASK           GPIO_PIN_MASK(7)
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name ADE7753 configuration
 *
 * These values configure which CC2538 pins to use for the ADE chip.
 * @{
 */
#define ADE7753_IRQ_N_PORT_NUM GPIO_B_NUM
#define ADE7753_IRQ_N_PIN      1
#define ADE7753_ZX_PORT_NUM    GPIO_B_NUM
#define ADE7753_ZX_PIN         0
#define ADE7753_CF_PORT_NUM    GPIO_C_NUM
#define ADE7753_CF_PIN         7
#define ADE7753_CS_N_PORT_NUM  GPIO_B_NUM
#define ADE7753_CS_N_PIN       2
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name FM25LB configuration
 *
 * These values configure which CC2538 pins to use for the FRAM chip.
 * @{
 */
#define FM25LB_HOLD_N_PORT_NUM GPIO_C_NUM
#define FM25LB_HOLD_N_PIN      1
#define FM25LB_WP_N_PORT_NUM   GPIO_C_NUM
#define FM25LB_WP_N_PIN        6
#define FM25LB_CS_N_PORT_NUM   GPIO_C_NUM
#define FM25LB_CS_N_PIN        5
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name SPI configuration
 *
 * These values configure which CC2538 pins to use for the SPI lines.
 * @{
 */
#define SPI_CLK_PORT  GPIO_C_NUM
#define SPI_CLK_PIN   2
#define SPI_MOSI_PORT GPIO_C_NUM
#define SPI_MOSI_PIN  3
#define SPI_MISO_PORT GPIO_C_NUM
#define SPI_MISO_PIN  4
/** @} */
/*---------------------------------------------------------------------------*/
/** \name GPIO
 *
 */
#define RELAY_CTRL_BASE          GPIO_B_BASE
#define RELAY_CTRL_MASK          GPIO_PIN_MASK(5)

#define RELAY_BUTTON_PORT        GPIO_B_NUM
#define RELAY_BUTTON_PIN         3
#define RELAY_BUTTON_VECTOR      NVIC_INT_GPIO_PORT_B
/** @} */
/*---------------------------------------------------------------------------*/
/** \name USB configuration
 *
 * The USB pullup is driven by PC0 and is shared with LED1
 */
#define USB_PULLUP_PORT          GPIO_C_BASE
#define USB_PULLUP_PIN           0
#define USB_PULLUP_PIN_MASK      (1 << USB_PULLUP_PIN)
/** @} */
/*---------------------------------------------------------------------------*/
/** \name UART configuration
 *
 * On the SmartRF06EB, the UART (XDS back channel) is connected to the
 * following ports/pins
 * - RX:  PA0
 * - TX:  PA1
 * - CTS: PB0 (Can only be used with UART1)
 * - RTS: PD3 (Can only be used with UART1)
 *
 * We configure the port to use UART0. To use UART1, replace UART0_* with
 * UART1_* below.
 * @{
 */
#define UART0_RX_PORT            GPIO_A_NUM
#define UART0_RX_PIN             0

#define UART0_TX_PORT            GPIO_A_NUM
#define UART0_TX_PIN             1
// unused uart1
#define UART1_CTS_PORT           GPIO_C_NUM
#define UART1_CTS_PIN            1

#define UART1_RTS_PORT           GPIO_C_NUM
#define UART1_RTS_PIN            2
/** @} */
/*---------------------------------------------------------------------------*/
/* Notify various examples that we do not have buttons */
#define PLATFORM_HAS_BUTTON      0
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name Device string used on startup
 * @{
 */
#define BOARD_STRING "acme++"
/** @} */

#endif /* BOARD_H_ */

/**
 * @}
 * @}
 */
