#ifndef PUBSUB_MESSAGE_H
#define PUBSUB_MESSAGE_H

#include "common.h"

typedef struct pubsub_message {
    char topic[PUBSUB_MAX_TOPIC_LENGTH];
    char key[PUBSUB_MAX_KEY_LENGTH];
    uint8_t* payload;
    size_t payload_size;
    uint64_t timestamp_ms;
    uint32_t partition_id;  /* For future use */
} pubsub_message_t;

/* Message lifecycle functions */
pubsub_error_t pubsub_message_create(
    pubsub_message_t** msg,
    const char* topic,
    const uint8_t* payload,
    size_t payload_size,
    const char* key
);

pubsub_error_t pubsub_message_create_string(
    pubsub_message_t** msg,
    const char* topic,
    const char* payload_str
);

pubsub_error_t pubsub_message_clone(
    const pubsub_message_t* src,
    pubsub_message_t** dst
);

void pubsub_message_destroy(pubsub_message_t* msg);

#endif /* PUBSUB_MESSAGE_H */
