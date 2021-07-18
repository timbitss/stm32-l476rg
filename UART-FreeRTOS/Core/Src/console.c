/**
 * @file console.c
 * @author Command line interface.
 * @brief Timothy Nguyen
 * @version 0.1
 * @date 2021-07-15
 */

#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "console.h"
#include "common.h"
#include "uart.h"
#include "cmd.h"
#include "log.h"
#include "printf.h"

////////////////////////////////////////////////////////////////////////////////
// Common macros
////////////////////////////////////////////////////////////////////////////////

#define PROMPT "> "

////////////////////////////////////////////////////////////////////////////////
// Type definitions
////////////////////////////////////////////////////////////////////////////////

/* Console structure */
typedef struct
{
    char cmd_buf[CONSOLE_CMD_BUF_SIZE]; // Hold command characters as they are entered by user over serial.
    uint16_t num_cmd_buf_chars;         // Holds number of characters currently in command buffer.
    bool first_run_done;                // First run, print PROMPT before checking for command characters.
} Console_t;

////////////////////////////////////////////////////////////////////////////////
// Private (static) function declarations
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Private (static) variables
////////////////////////////////////////////////////////////////////////////////

/* Console_t object */
static Console_t console;

/* Unique tag for logging module */
static const char * TAG = "CONSOLE";

////////////////////////////////////////////////////////////////////////////////
// Public (global) variables and externs
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Public (global) functions
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Initialize console module instance.
 *
 * @return MOD_OK for success.
 */
mod_err_t console_init(void)
{
    memset(&console, 0, sizeof(console));
    LOGI(TAG, "Initialized console.");
    return MOD_OK;
}


mod_err_t console_run(void)
{
    char c;
    if (!console.first_run_done)
    {
        console.first_run_done = true;
        LOG(PROMPT);
    }

    /* Process all characters in UART's receive buffer. */
    while (uart_getc(&c))
    {
        /* Execute command once Enter key is pressed. */
        if (c == '\n' || c == '\r')
        {
            console.cmd_buf[console.num_cmd_buf_chars] = '\0'; // Signal end of command string.
            LOG("\r\n");
            cmd_execute(console.cmd_buf); // Execute command's callback function.
            console.num_cmd_buf_chars = 0;
            LOG(PROMPT);
            continue;
        }
        /* Delete a character when Backspace key is pressed. */
        if (c == '\b' || c == '\x7f')
        {
            if (console.num_cmd_buf_chars > 0)
            {
                LOG("\x7f");
                console.num_cmd_buf_chars--; // "Overwrite" last character.
            }
            continue;
        }
        /* Toggle logging on and off LOG_TOGGLE_CHAR key is pressed. */
        if (c == LOG_TOGGLE_CHAR)
        {
            bool log_active = log_toggle();
            LOG("\r\n<Logging %s>\r\n", log_active ? "on" : "off");
            LOG(PROMPT);
            continue;
        }
        /* Echo the character back. */
        if (isprint(c))
        {
            if (console.num_cmd_buf_chars < (CONSOLE_CMD_BUF_SIZE - 1))
            {
                console.cmd_buf[console.num_cmd_buf_chars++] = c;
                LOG("%c", c);
            }
            else
            {
                /* No space in buffer, so ring terminal bell. */
                LOGW(TAG, "No more space in command buffer.");
                LOG("\a");
            }
            continue;
        }
    }

    return MOD_OK;
}

////////////////////////////////////////////////////////////////////////////////
// Private (static) functions
////////////////////////////////////////////////////////////////////////////////
