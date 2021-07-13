/**
 * @file command.h
 * @author Timothy Nguyen
 * @brief Delegate commands received from serial and transmit information to host.
 * @version 0.1
 * @date 2021-07-12
 */

#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "stm32l4xx_hal.h"
#include "cmsis_os.h"

/**
 * @brief Initialize receive and transmit UART threads and various FreeRTOS objects.
 * 
 * @note Scheduler is not called within initialization function.
 * @param huart Pointer to UART configuration structure.
 * @return uint8_t 0: Threads were successfully created.
 *                 1: Insufficient memory for threads or FreeRTOS objects.
 */
uint8_t Command_Init(UART_HandleTypeDef *huart);

/**
 * @brief Transmit message over serial.
 * 
 * @param msg String buffer to be transmitted over serial.
 * @param timeout_period Timeout period in ms.
 * @return osStatus_t osOK: Message placed into queue.
 *                    osErrorTimeout: Message could not be placed in queue in given timeout_period.
 */
osStatus_t Command_Transmit(const char *msg, uint32_t timeout_period);

#endif