#include "message.h"
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

static uint64_t get_timestamp_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
}

pubsub_error_t pubsub_message_create(
    pubsub_message_t** msg,
    const char* topic,
    const uint8_t* payload,
    size_t payload_size,
    const char* key) {
    
    if (!msg || !topic || !payload) {
        return PUBSUB_ERROR_NULL_PARAM;
    }
    
    if (strlen(topic) >= PUBSUB_MAX_TOPIC_LENGTH) {
        return PUBSUB_ERROR_INVALID_TOPIC;
    }
    
    if (payload_size > PUBSUB_MAX_PAYLOAD_SIZE) {
        return PUBSUB_ERROR_ALLOCATION;
    }
    
    pubsub_message_t* new_msg = (pubsub_message_t*)calloc(1, sizeof(pubsub_message_t));
    if (!new_msg) {
        return PUBSUB_ERROR_ALLOCATION;
    }
    
    /* Copy topic */
    strncpy(new_msg->topic, topic, PUBSUB_MAX_TOPIC_LENGTH - 1);
    new_msg->topic[PUBSUB_MAX_TOPIC_LENGTH - 1] = '\0';
    
    /* Copy key if provided */
    if (key) {
        strncpy(new_msg->key, key, PUBSUB_MAX_KEY_LENGTH - 1);
        new_msg->key[PUBSUB_MAX_KEY_LENGTH - 1] = '\0';
    } else {
        new_msg->key[0] = '\0';
    }
    
    /* Allocate and copy payload */
    new_msg->payload = (uint8_t*)malloc(payload_size);
    if (!new_msg->payload) {
        free(new_msg);
        return PUBSUB_ERROR_ALLOCATION;
    }
    
    memcpy(new_msg->payload, payload, payload_size);
    new_msg->payload_size = payload_size;
    new_msg->timestamp_ms = get_timestamp_ms();
    new_msg->partition_id = 0;
    
    *msg = new_msg;
    return PUBSUB_SUCCESS;
}

pubsub_error_t pubsub_message_create_string(
    pubsub_message_t** msg,
    const char* topic,
    const char* payload_str) {
    
    if (!payload_str) {
        return PUBSUB_ERROR_NULL_PARAM;
    }
    
    size_t len = strlen(payload_str);
    return pubsub_message_create(msg, topic, (const uint8_t*)payload_str, 
                                len, NULL);
}

pubsub_error_t pubsub_message_clone(
    const pubsub_message_t* src,
    pubsub_message_t** dst) {
    
    if (!src || !dst) {
        return PUBSUB_ERROR_NULL_PARAM;
    }
    
    return pubsub_message_create(dst, src->topic, src->payload,
                                src->payload_size, src->key);
}

void pubsub_message_destroy(pubsub_message_t* msg) {
    if (msg) {
        if (msg->payload) {
            free(msg->payload);
            msg->payload = NULL;
        }
        free(msg);
    }
}
