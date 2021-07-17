/**
 * @file console.h
 * @author Command line interface.
 * @brief Timothy Nguyen
 * @version 0.1
 * @date 2021-07-15
 */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "common.h"

/* Configuration parameters */
#define CONSOLE_CMD_BUF_SIZE 40 // Size of buffer to hold command characters entered as they are entered by user.

/**
 * @brief Initialize console module instance.
 *
 * @return MOD_OK for success.
 */
mod_err_t console_init(void);

/**
 * @brief Run console instance.
 *
 * @return MOD_OK for success.

 * This function processes characters in UART's receive buffer in non-blocking mode.
 * If the Enter key is pressed, a command parser is invoked.
 */
mod_err_t console_run(void);


#endif
