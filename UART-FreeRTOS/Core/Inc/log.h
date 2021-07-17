/**
 * @file log.h
 * @author Timothy Nguyen
 * @brief Console logging module.
 * @version 0.1
 * @date 2021-07-16
 * 
 * Currently supports logging messages over UART with timestamps and configurable global log level.
 * 
 * In each module that uses logging functionality, define a TAG variable like so: 
 *
 * static const char* TAG = "MyModule";
 *
 * Then use one of logging macros to produce output, e.g: 
 * 
 * LOGW(TAG, "Baud rate error %.1f%%. Requested: %d baud, actual: %d baud", error * 100, baud_req, baud_real);
 */

#ifndef _LOG_H_
#define _LOG_H_

#include <stdbool.h>

#include "stm32l4xx_hal.h"
#include "common.h"

/* Configuration parameters */
#define LOG_TOGGLE_CHAR '\t' // Press LOG_TOGGLE_CHAR to toggle logging on and off.

/**
 * @brief Logging levels.
 */
typedef enum {
    LOG_NONE,    // No log output.
    LOG_ERROR,   // Critical errors. Software module can not recover on its own.
    LOG_WARNING, // Errors in which recovery measures have been taken by module.
    LOG_INFO,    // Information messages which describe normal flow of events.
    LOG_DEBUG,   // Extra information which is not necessary for normal use (values, pointers, sizes, etc).
    LOG_VERBOSE, // Bigger chunks of debugging information, or frequent messages which can potentially flood the output.

    LOG_DEFAULT = LOG_INFO // Default log level.
} log_level_t;

/**
 * @brief Initialize log module.
 *
 * @return MOD_OK if successful, otherwise a "MOD_ERR" value.
 */
mod_err_t log_init(void);

/**
 * @brief Toggle data logging.
 * 
 * @return true Logging is now active.
 *         false Logging is now inactive.
 */
bool log_toggle(void);

/**
 * @brief Check if logging is active or inactive.
 * 
 * @return true Logging is active.
 *         false Logging is inactive.
 */
bool log_is_active(void);

/** 
 * @brief Base "printf" style function for logging.
 *
 * @param fmt Format string.
 *
 * This function is not intended to be used directly. Instead, use one of 
 * LOGE, LOGW, LOGI, LOGD, LOGV macros below.
 */
void log_printf(const char* fmt, ...);

/* Private variables for logging macros. Do not modify. */
extern bool _log_active;
extern int32_t _global_log_level;

#define LOGE(tag, fmt, ...) do { \
    if (_log_active && _global_log_level >= LOG_ERROR)   { log_printf("E " fmt, ##__VA_ARGS__); } \
    } while (0)

#define LOGW(tag, fmt, ...) do {\
    if (_log_active && _global_log_level >= LOG_WARNING) { log_printf("W " fmt, ##__VA_ARGS__); } \
    } while (0)

#define LOGI(tag, fmt, ...) do { \
    if (_log_active && _global_log_level >= LOG_INFO)    { log_printf("I " fmt, ##__VA_ARGS__); } \
    } while (0)

#define LOGD(tag, fmt, ...) do { \
    if (_log_active && _global_log_level >= LOG_DEBUG)   { log_printf("D " fmt, ##__VA_ARGS__); } \
    } while (0)
            
#define LOGV(tag, fmt, ...) do { \
    if (_log_active && _global_log_level >= LOG_VERBOSE)   {log_printf("V ", fmt, ##__VA_ARGS__); } \
    } while (0)

#endif // _LOG_H_
