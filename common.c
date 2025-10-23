#include "common.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static const char* log_level_strings[] = {
    "DEBUG", "INFO", "WARN", "ERROR"
};

void pubsub_log(log_level_t level, const char* file, int line,
                const char* fmt, ...) {
    va_list args;
    time_t now;
    struct tm* timeinfo;
    char time_buffer[64];
    
    time(&now);
    timeinfo = localtime(&now);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    fprintf(stderr, "[%s] [%s] %s:%d - ", time_buffer,
            log_level_strings[level], file, line);
    
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    fprintf(stderr, "\n");
    fflush(stderr);
}

const char* pubsub_error_string(pubsub_error_t error) {
    switch (error) {
        case PUBSUB_SUCCESS: return "Success";
        case PUBSUB_ERROR_NULL_PARAM: return "NULL parameter";
        case PUBSUB_ERROR_ALLOCATION: return "Memory allocation failed";
        case PUBSUB_ERROR_QUEUE_FULL: return "Queue is full";
        case PUBSUB_ERROR_QUEUE_EMPTY: return "Queue is empty";
        case PUBSUB_ERROR_SHUTDOWN: return "System is shutting down";
        case PUBSUB_ERROR_TIMEOUT: return "Operation timed out";
        case PUBSUB_ERROR_NOT_FOUND: return "Item not found";
        case PUBSUB_ERROR_INVALID_TOPIC: return "Invalid topic name";
        case PUBSUB_ERROR_THREAD_CREATE: return "Failed to create thread";
        case PUBSUB_ERROR_MUTEX_INIT: return "Failed to initialize mutex";
        case PUBSUB_ERROR_COND_INIT: return "Failed to initialize condition variable";
        default: return "Unknown error";
    }
}
