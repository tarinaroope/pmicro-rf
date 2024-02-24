/**
 * @file debug_logging.h
 * @brief Header file for debug logging functionality.
 */

#pragma once
#define ENABLE_LOGGING

#ifdef ENABLE_LOGGING
#include <stdio.h>
/**
 * @def TRACE
 * @brief Macro for printing debug log messages.
 * @param fmt The format string for the log message.
 * @param ... The optional arguments to be formatted and printed.
 *
 * This macro is used to print debug log messages when logging is enabled.
 * It prints the log message along with the function name and line number.
 */
#define TRACE(fmt, ...) \
    do { \
        printf("[%s:%d] " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)
#else
/**
 * @def TRACE
 * @brief Empty macro when logging is disabled.
 *
 * This macro is empty when logging is disabled, allowing for easy removal
 * of debug log statements from the code.
 */
#define TRACE(fmt, ...) do {} while (0)
#endif