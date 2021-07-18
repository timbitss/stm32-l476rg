/**
 * @file log.h
 * @author Timothy Nguyen
 * @brief Console logging module.
 * @version 0.1
 * @date 2021-07-16
 */

#include <stdarg.h>
#include <string.h>

#include "log.h"
#include "common.h"
#include "stm32l4xx_hal.h"
#include "printf.h"
#include "cmd.h"

////////////////////////////////////////////////////////////////////////////////
// Common macros
////////////////////////////////////////////////////////////////////////////////

/* Logging names */
#define LOG_LEVEL_NAMES "OFF, ERROR, WARNING, INFO, DEBUG, VERBOSE"
#define LOG_LEVEL_NAMES_CSV "OFF", "ERROR", "WARNING", "INFO", "DEBUG", "VERBOSE"

////////////////////////////////////////////////////////////////////////////////
// Type definitions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Private (static) function declarations
////////////////////////////////////////////////////////////////////////////////

/* Command callback functions */
static uint32_t cmd_log_get(uint32_t argc, const char **argv); // Get current global log level.
static uint32_t cmd_log_set(uint32_t argc, const char **argv); // Set current global log level.

static const char *log_level_str(int32_t level); // Convert log level from integer to string.
static int32_t log_level_int(const char *level_name); // Convert log level from string to integer.

////////////////////////////////////////////////////////////////////////////////
// Private (static) variables
////////////////////////////////////////////////////////////////////////////////

/* Log level names */
static const char* log_level_names[] = {
    LOG_LEVEL_NAMES_CSV
};

/* Data structure with log commands */
static cmd_cmd_info log_cmds[] = {
    {
        .cmd_name = "get",
        .cb = cmd_log_get,
        .help = "Display current log level.\r\nPossible log levels: " LOG_LEVEL_NAMES
    },
    {
        .cmd_name = "set",
        .cb = cmd_log_set,
        .help = "Set global log level, usage: log level <level>.\r\nPossible log levels: " LOG_LEVEL_NAMES
    }
};

/* Log module client info */
static cmd_client_info log_client_info =
{
		 .client_name = "log",
		 .num_cmds = 2,
		 .cmds = log_cmds,
		 .num_u16_pms = 0,
		 .u16_pms = NULL,
		 .u16_pm_names = NULL
};

/* Unique tag for logging module */
static const char* TAG = "LOG";

////////////////////////////////////////////////////////////////////////////////
// Public (global) variables and externs
////////////////////////////////////////////////////////////////////////////////

/* Logging state */
bool _log_active = true;

/* Logging level */
int32_t _global_log_level = LOG_DEFAULT;

////////////////////////////////////////////////////////////////////////////////
// Public (global) functions
////////////////////////////////////////////////////////////////////////////////

mod_err_t log_init(void)
{
	LOGI(TAG, "Initialized log module");
	return cmd_register(&log_client_info);
}

bool log_toggle(void)
{
    _log_active = _log_active ? false : true;
    return _log_active;
}

bool log_is_active(void)
{
    return _log_active;
}

void log_printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

/**
 * @brief Convert integer log level to a string.
 *
 * @param level The log level as an integer.
 *
 * @return Log level as a string.
 */
static const char *log_level_str(int32_t level)
{
	return log_level_names[level];
}
/**
 * @brief Convert log level string to an int.
 *
 * @param level_name The log level as a string.
 *
 * @return Log level as an int, or -1 on error.
 */
static int32_t log_level_int(const char *level_name)
{
    for (uint8_t level = 0; level < ARRAY_SIZE(log_level_names); level++)
    {
        if (strcasecmp(level_name, log_level_names[level]) == 0)
        {
            return level;
        }
    }

    return -1; // Log level not found.
}

////////////////////////////////////////////////////////////////////////////////
// Private (static) functions
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Display log level to user.
 *
 * @param argc Number of arguments.
 * @param argv Argument values.
 *
 * @return 0 if successful, 1 otherwise.
 */
uint32_t cmd_log_get(uint32_t argc, const char **argv)
{
	LOGI(TAG, "Current log level: %s",  log_level_str(_global_log_level));
	return 0;
}

/**
 * @brief Set log level.
 *
 * @param argc Number of arguments.
 * @param argv Argument values.
 *
 * @return 0 if successful, 1 otherwise.
 */
uint32_t cmd_log_set(uint32_t argc, const char **argv)
{
	if(argc != 1)
	{
        LOGW(TAG, "Missing log level argument");
		return 1; // Should include only 1 argument.
	}
	else
	{
		int32_t new_log_level = log_level_int(argv[0]);
		if(new_log_level == -1)
		{
			LOGW(TAG, "Log level (%s) not recognized", argv[0]);
			return 1;
		}
		else
		{
			_global_log_level = new_log_level;
			LOGI(TAG, "Global log level set to %s", log_level_str(_global_log_level));
			return 0;
		}
	}
}


