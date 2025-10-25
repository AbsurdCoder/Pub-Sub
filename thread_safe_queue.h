#ifndef PUBSUB_THREAD_SAFE_QUEUE_H
#define PUBSUB_THREAD_SAFE_QUEUE_H

#include "common.h"
#include "message.h"
#include <pthread.h>

/* Queue node structure */
typedef struct queue_node {
    pubsub_message_t* message;
    struct queue_node* next;
} queue_node_t;

/* Thread-safe queue structure */
typedef struct thread_safe_queue {
    queue_node_t* head;
    queue_node_t* tail;
    size_t size;
    size_t max_size;
    pthread_mutex_t mutex;
    pthread_cond_t cond_not_empty;
    pthread_cond_t cond_not_full;
    bool shutdown;
} thread_safe_queue_t;

/* Queue operations */
pubsub_error_t tsqueue_create(thread_safe_queue_t** queue, size_t max_size);

void tsqueue_destroy(thread_safe_queue_t* queue);

pubsub_error_t tsqueue_push(thread_safe_queue_t* queue, 
                           pubsub_message_t* msg);

pubsub_error_t tsqueue_push_timeout(thread_safe_queue_t* queue,
                                   pubsub_message_t* msg,
                                   uint32_t timeout_ms);

pubsub_error_t tsqueue_pop(thread_safe_queue_t* queue,
                          pubsub_message_t** msg);

pubsub_error_t tsqueue_pop_timeout(thread_safe_queue_t* queue,
                                  pubsub_message_t** msg,
                                  uint32_t timeout_ms);

pubsub_error_t tsqueue_try_push(thread_safe_queue_t* queue,
                               pubsub_message_t* msg);

pubsub_error_t tsqueue_try_pop(thread_safe_queue_t* queue,
                              pubsub_message_t** msg);

size_t tsqueue_size(thread_safe_queue_t* queue);

bool tsqueue_is_empty(thread_safe_queue_t* queue);

void tsqueue_shutdown(thread_safe_queue_t* queue);

#endif /* PUBSUB_THREAD_SAFE_QUEUE_H */
