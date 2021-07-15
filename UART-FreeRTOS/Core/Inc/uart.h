/**
 * @file uart.h
 * @author Timothy Nguyen
 * @brief Interface for UART peripheral with CMSIS-RTOSv2
 * @version 0.1
 * @date 2021-07-12
 *
 * Notes:
 * - A UART peripheral must be initialized prior to using this module.
 * - Do not enable UART interrupts within CubeMX software.
 */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "cmsis_os.h"
#include "common.h"

/* Configuration parameters */
#define MAX_MSG_COUNT 5    // Maximum number of messages allowed in queue at a time to UART_TX_Thread.
#define UART_BUF_SZ 100    // Maximum number of bytes in UART buffer.
#define TX_BUF_SIZE 100    // Maximum number of bytes in UART's transmit circular buffer. 
#define RX_BUF_SIZE 100    // Maximum number of bytes in UART's receive circular buffer. 

/* Type definitions */
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
 * @param huart Pointer to UART configuration structure.
 * @return mod_err_t MOD_OK if initialization was successful, appropriate error value otherwise.
 * 
 * @note OS scheduler is not called within initialization function.
 */
mod_err_t uart_init(UART_HandleTypeDef *huart);


/**
 * @brief Log message over serial.
 * 
 * @param msg String buffer to be transmitted over serial.
 * @param timeout_period Timeout period in ms.
 * @return osStatus_t osOK: Message placed into queue.
 *                    osErrorTimeout: Message could not be placed in queue in given timeout_period.
 */
osStatus_t Console_Log(const char *msg, uint32_t timeout_period);

/**
 * @brief Get uint32 from user in blocking mode.
 * 
 * @param prompt User prompt.
 * @return uint32_t 32-bit unsigned int received from user.
 * 
 *   Useful for options that require additional inputs.
 */
uint32_t Command_get_uint32(const char *prompt);

#endif
