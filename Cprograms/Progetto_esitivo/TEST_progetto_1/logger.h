#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h> 
#include <stdbool.h>

typedef enum{
    LOG_EVENT_FILE_PARSING,
    LOG_EVENT_MESSAGE_QUEUE,
    LOG_EVENT_EMERGENCY_STATUS,
    LOG_EVENT_RESCUER_STATUS,
    LOG_EVENT_ASSIGNMENT,
    LOG_EVENT_TIMEOUT,
    LOG_EVENT_GENERAL_INFO,
    LOG_EVENT_GENERAL_ERROR
}log_event_type_t;

bool init_logger(const char* log_filename);

void close_logger(void);

void log_message(log_event_type_t event_type, const char* id, const char* message);

const char* log_event_type_string(log_event_type_t event_type);

#endif