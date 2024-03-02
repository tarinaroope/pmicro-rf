/**
 * @file debug_logging.h
 * @brief Header file for debug logging functionality.
 */
#ifndef DEBUG_LOGGING_H
#define DEBUG_LOGGING_H

#pragma once
#define ENABLE_LOGGING

#ifdef ENABLE_LOGGING
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"

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

#define INITLOG(max_log_size) log_initlog(max_log_size);
#define LOGENTRY(signal, customer) log_logentry(signal, customer);
#define PRINTLOG log_printlog();

typedef struct Logging_data
{
    uint64_t    timestamp;
    int8_t      signal;
    uint8_t     custom;
}Logging_data;

static uint16_t logindex;
static Logging_data* logger_data;

static void log_logentry(int8_t signal, uint8_t custom)
{
    logger_data[logindex].timestamp = to_us_since_boot(get_absolute_time());
    logger_data[logindex].signal = signal;
    logger_data[logindex].custom = custom;
    logindex++;
}

static void log_printlog()
{
    for (int i = 0; i < logindex; i++)
    {
        printf("t:%llu,s:%d,c:%u ",logger_data[i].timestamp,logger_data[i].signal,logger_data[i].custom);
        if (i != 0 && i % 5 == 0)
        {
            printf("\n");
        }
    }
}

static void log_initlog(uint16_t max_log_size)
{
    logger_data = (Logging_data*) malloc (sizeof(Logging_data)* max_log_size);
    logger_data = 0;
}

#else
/**
 * @def TRACE
 * @brief Empty macro when logging is disabled.
 *
 * This macro is empty when logging is disabled, allowing for easy removal
 * of debug log statements from the code.
 */
#define TRACE(fmt, ...) do {} while (0)

#define INITLOG(max_log_size) log_initlog(max_log_size);
#define LOGENTRY(signal, customer) log_logentry(signal, customer);
#define PRINTLOG log_printlog();
#endif
#endif