#ifndef PUBSUB_COMMON_H
#define PUBSUB_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>

/* Error codes */
typedef enum {
    PUBSUB_SUCCESS = 0,
    PUBSUB_ERROR_NULL_PARAM = -1,
    PUBSUB_ERROR_ALLOCATION = -2,
    PUBSUB_ERROR_QUEUE_FULL = -3,
    PUBSUB_ERROR_QUEUE_EMPTY = -4,
    PUBSUB_ERROR_SHUTDOWN = -5,
    PUBSUB_ERROR_TIMEOUT = -6,
    PUBSUB_ERROR_NOT_FOUND = -7,
    PUBSUB_ERROR_INVALID_TOPIC = -8,
    PUBSUB_ERROR_THREAD_CREATE = -9,
    PUBSUB_ERROR_MUTEX_INIT = -10,
    PUBSUB_ERROR_COND_INIT = -11
} pubsub_error_t;

/* Log levels */
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level_t;

/* Configuration constants */
#define PUBSUB_MAX_TOPIC_LENGTH 256
#define PUBSUB_MAX_KEY_LENGTH 128
#define PUBSUB_MAX_PAYLOAD_SIZE 65536
#define PUBSUB_DEFAULT_QUEUE_SIZE 10000
#define PUBSUB_HASH_TABLE_SIZE 1024

/* Logging macro */
#define PUBSUB_LOG(level, fmt, ...) \
    pubsub_log(level, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

/* Logging function */
void pubsub_log(log_level_t level, const char* file, int line, 
                const char* fmt, ...);

/* Error to string conversion */
const char* pubsub_error_string(pubsub_error_t error);

#endif /* PUBSUB_COMMON_H */
