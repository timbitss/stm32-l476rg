/**
 * @file command.h
 * @author Timothy Nguyen
 * @brief Delegate commands received from serial and transmit information to host.
 * @version 0.1
 * @date 2021-07-12
 * 
 *       Includes menu interface.
 */

#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "stm32l4xx_hal.h"
#include "cmsis_os.h"

/* Configuration parameters */
#define MAX_MSG_COUNT 5    // Maximum number of messages allowed in queue at a time to UART_TX_Thread.
#define UART_BUF_SZ 100    // Maximum number of bytes in UART buffer.

typedef uint8_t (*Item_handler_t)(void); // Command function pointer alias.

/**
 * @brief Menu option.
 */
typedef struct
{
    const char *name;  // Name of menu option to be displayed to user.
    Item_handler_t cb; // Callback if menu option is selected.
} Command_menu_option;


/**
 * @brief Initialize UART threads for reception and transmission with menu interface.
 * 
 * @note OS scheduler is not called within initialization function.
 * @param huart Pointer to UART configuration structure.
 * @param _menu_options Array of Command_menu_option objects.
 * @param _num_of_options Number of options in _menu_options (<= 9)
 * @return uint8_t 0: Threads were successfully created.
 *                 1: Insufficient memory for threads or FreeRTOS objects.
 */
uint8_t Command_Init(UART_HandleTypeDef *huart, const Command_menu_option *_menu_options, uint8_t _num_of_options);

/**
 * @brief Transmit message over serial.
 * 
 * @param msg String buffer to be transmitted over serial.
 * @param timeout_period Timeout period in ms.
 * @return osStatus_t osOK: Message placed into queue.
 *                    osErrorTimeout: Message could not be placed in queue in given timeout_period.
 */
osStatus_t Command_Transmit(const char *msg, uint32_t timeout_period);

/**
 * @brief Get uint32 from user in blocking mode.
 * 
 *        Useful for options that require additional inputs.
 * 
 * @param prompt User prompt.
 * @return uint32_t 32-bit unsigned int received from user.
 */
uint32_t Command_get_uint32(const char *prompt);

#endif
