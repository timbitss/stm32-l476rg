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
#include "active.h"

////////////////////////////////////////////////////////////////////////////////
// Common macros
////////////////////////////////////////////////////////////////////////////////

#define PROMPT "> "

////////////////////////////////////////////////////////////////////////////////
// Type definitions
////////////////////////////////////////////////////////////////////////////////

/* Command line buffer ID */
typedef enum
{
  BUF1_ID,
  BUF2_ID,
  NUM_OF_BUFFERS
} BUF_ID_t;

/* Console structure */
typedef struct
{
    char cmd_buf[NUM_OF_BUFFERS][CONSOLE_CMD_BUF_SIZE]; // Hold command characters as they are entered by user over serial.
    uint16_t num_cmd_buf_chars;                         // Holds number of characters currently in command buffer.
    bool first_run_done;                                // First run, print PROMPT before checking for command characters.
    BUF_ID_t buf_id;                                   // Buffer ID (BUF1 or BUF2).
} Console_t;

////////////////////////////////////////////////////////////////////////////////
// Private (static) function declarations
////////////////////////////////////////////////////////////////////////////////

static inline void switch_bufs(void); // Switch between command line buffers.

static void post_cmd_event(void); // Post command event to command active object.

////////////////////////////////////////////////////////////////////////////////
// Private (static) variables
////////////////////////////////////////////////////////////////////////////////

/* Console_t object */
static Console_t console;

/* Unique tag for logging module */
static const char * TAG = "CONSOLE";

/* Command event object to send when Enter key is pressed. */
static Cmd_Event cmd_evt = { .base = { CMD_RX_SIG }, .cmd_line = NULL };


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
            console.cmd_buf[console.buf_id][console.num_cmd_buf_chars] = '\0'; // Signal end of command string.
            LOG("\r\n");
            post_cmd_event();
            switch_bufs();
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
                console.cmd_buf[console.buf_id][console.num_cmd_buf_chars++] = c;
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

/**
 * @brief Post command event to command active object.
 */
static void post_cmd_event(void)
{
	cmd_evt.cmd_line = console.cmd_buf[console.buf_id];
	Active_post(cmd_base, (Event const*)&cmd_evt);
}

/**
 * @brief Switch between command line buffers.
 *
 * More than one command line buffer is used to reduce possibility of race conditions.
 */
static inline void switch_bufs(void)
{
	console.buf_id = console.buf_id == BUF1_ID ? BUF2_ID : BUF1_ID;
}

