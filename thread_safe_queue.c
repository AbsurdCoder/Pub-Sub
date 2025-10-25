#include "thread_safe_queue.h"
#include <stdlib.h>
#include <errno.h>

pubsub_error_t tsqueue_create(thread_safe_queue_t** queue, size_t max_size) {
    if (!queue) {
        return PUBSUB_ERROR_NULL_PARAM;
    }
    
    thread_safe_queue_t* q = (thread_safe_queue_t*)calloc(1, sizeof(thread_safe_queue_t));
    if (!q) {
        return PUBSUB_ERROR_ALLOCATION;
    }
    
    /* Initialize mutex */
    if (pthread_mutex_init(&q->mutex, NULL) != 0) {
        free(q);
        return PUBSUB_ERROR_MUTEX_INIT;
    }
    
    /* Initialize condition variables */
    if (pthread_cond_init(&q->cond_not_empty, NULL) != 0) {
        pthread_mutex_destroy(&q->mutex);
        free(q);
        return PUBSUB_ERROR_COND_INIT;
    }
    
    if (pthread_cond_init(&q->cond_not_full, NULL) != 0) {
        pthread_cond_destroy(&q->cond_not_empty);
        pthread_mutex_destroy(&q->mutex);
        free(q);
        return PUBSUB_ERROR_COND_INIT;
    }
    
    /* Create dummy head node for easier queue management */
    q->head = (queue_node_t*)calloc(1, sizeof(queue_node_t));
    if (!q->head) {
        pthread_cond_destroy(&q->cond_not_full);
        pthread_cond_destroy(&q->cond_not_empty);
        pthread_mutex_destroy(&q->mutex);
        free(q);
        return PUBSUB_ERROR_ALLOCATION;
    }
    
    q->tail = q->head;
    q->size = 0;
    q->max_size = max_size;
    q->shutdown = false;
    
    *queue = q;
    return PUBSUB_SUCCESS;
}

void tsqueue_destroy(thread_safe_queue_t* queue) {
    if (!queue) {
        return;
    }
    
    tsqueue_shutdown(queue);
    
    pthread_mutex_lock(&queue->mutex);
    
    /* Free all nodes */
    queue_node_t* current = queue->head;
    while (current) {
        queue_node_t* next = current->next;
        if (current->message) {
            pubsub_message_destroy(current->message);
        }
        free(current);
        current = next;
    }
    
    pthread_mutex_unlock(&queue->mutex);
    
    pthread_cond_destroy(&queue->cond_not_full);
    pthread_cond_destroy(&queue->cond_not_empty);
    pthread_mutex_destroy(&queue->mutex);
    
    free(queue);
}

pubsub_error_t tsqueue_push(thread_safe_queue_t* queue, 
                           pubsub_message_t* msg) {
    if (!queue || !msg) {
        return PUBSUB_ERROR_NULL_PARAM;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    /* Wait while queue is full */
    while (queue->size >= queue->max_size && !queue->shutdown) {
        pthread_cond_wait(&queue->cond_not_full, &queue->mutex);
    }
    
    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return PUBSUB_ERROR_SHUTDOWN;
    }
    
    /* Create new node */
    queue_node_t* node = (queue_node_t*)malloc(sizeof(queue_node_t));
    if (!node) {
        pthread_mutex_unlock(&queue->mutex);
        return PUBSUB_ERROR_ALLOCATION;
    }
    
    node->message = msg;
    node->next = NULL;
    
    /* Add to tail */
    queue->tail->next = node;
    queue->tail = node;
    queue->size++;
    
    pthread_cond_signal(&queue->cond_not_empty);
    pthread_mutex_unlock(&queue->mutex);
    
    return PUBSUB_SUCCESS;
}

pubsub_error_t tsqueue_push_timeout(thread_safe_queue_t* queue,
                                   pubsub_message_t* msg,
                                   uint32_t timeout_ms) {
    if (!queue || !msg) {
        return PUBSUB_ERROR_NULL_PARAM;
    }
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    /* Wait while queue is full or timeout */
    while (queue->size >= queue->max_size && !queue->shutdown) {
        int ret = pthread_cond_timedwait(&queue->cond_not_full, &queue->mutex, &ts);
        if (ret == ETIMEDOUT) {
            pthread_mutex_unlock(&queue->mutex);
            return PUBSUB_ERROR_TIMEOUT;
        }
    }
    
    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return PUBSUB_ERROR_SHUTDOWN;
    }
    
    /* Create new node */
    queue_node_t* node = (queue_node_t*)malloc(sizeof(queue_node_t));
    if (!node) {
        pthread_mutex_unlock(&queue->mutex);
        return PUBSUB_ERROR_ALLOCATION;
    }
    
    node->message = msg;
    node->next = NULL;
    
    /* Add to tail */
    queue->tail->next = node;
    queue->tail = node;
    queue->size++;
    
    pthread_cond_signal(&queue->cond_not_empty);
    pthread_mutex_unlock(&queue->mutex);
    
    return PUBSUB_SUCCESS;
}

pubsub_error_t tsqueue_pop(thread_safe_queue_t* queue,
                          pubsub_message_t** msg) {
    if (!queue || !msg) {
        return PUBSUB_ERROR_NULL_PARAM;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    /* Wait while queue is empty */
    while (queue->size == 0 && !queue->shutdown) {
        pthread_cond_wait(&queue->cond_not_empty, &queue->mutex);
    }
    
    if (queue->shutdown && queue->size == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return PUBSUB_ERROR_SHUTDOWN;
    }
    
    /* Remove from head */
    queue_node_t* node = queue->head->next;
    queue->head->next = node->next;
    
    if (queue->tail == node) {
        queue->tail = queue->head;
    }
    
    *msg = node->message;
    free(node);
    queue->size--;
    
    pthread_cond_signal(&queue->cond_not_full);
    pthread_mutex_unlock(&queue->mutex);
    
    return PUBSUB_SUCCESS;
}

pubsub_error_t tsqueue_pop_timeout(thread_safe_queue_t* queue,
                                  pubsub_message_t** msg,
                                  uint32_t timeout_ms) {
    if (!queue || !msg) {
        return PUBSUB_ERROR_NULL_PARAM;
    }
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    /* Wait while queue is empty or timeout */
    while (queue->size == 0 && !queue->shutdown) {
        int ret = pthread_cond_timedwait(&queue->cond_not_empty, &queue->mutex, &ts);
        if (ret == ETIMEDOUT) {
            pthread_mutex_unlock(&queue->mutex);
            return PUBSUB_ERROR_TIMEOUT;
        }
    }
    
    if (queue->shutdown && queue->size == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return PUBSUB_ERROR_SHUTDOWN;
    }
    
    /* Remove from head */
    queue_node_t* node = queue->head->next;
    queue->head->next = node->next;
    
    if (queue->tail == node) {
        queue->tail = queue->head;
    }
    
    *msg = node->message;
    free(node);
    queue->size--;
    
    pthread_cond_signal(&queue->cond_not_full);
    pthread_mutex_unlock(&queue->mutex);
    
    return PUBSUB_SUCCESS;
}

pubsub_error_t tsqueue_try_push(thread_safe_queue_t* queue,
                               pubsub_message_t* msg) {
    if (!queue || !msg) {
        return PUBSUB_ERROR_NULL_PARAM;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->size >= queue->max_size || queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return queue->shutdown ? PUBSUB_ERROR_SHUTDOWN : PUBSUB_ERROR_QUEUE_FULL;
    }
    
    /* Create new node */
    queue_node_t* node = (queue_node_t*)malloc(sizeof(queue_node_t));
    if (!node) {
        pthread_mutex_unlock(&queue->mutex);
        return PUBSUB_ERROR_ALLOCATION;
    }
    
    node->message = msg;
    node->next = NULL;
    
    /* Add to tail */
    queue->tail->next = node;
    queue->tail = node;
    queue->size++;
    
    pthread_cond_signal(&queue->cond_not_empty);
    pthread_mutex_unlock(&queue->mutex);
    
    return PUBSUB_SUCCESS;
}

pubsub_error_t tsqueue_try_pop(thread_safe_queue_t* queue,
                              pubsub_message_t** msg) {
    if (!queue || !msg) {
        return PUBSUB_ERROR_NULL_PARAM;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->size == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return PUBSUB_ERROR_QUEUE_EMPTY;
    }
    
    /* Remove from head */
    queue_node_t* node = queue->head->next;
    queue->head->next = node->next;
    
    if (queue->tail == node) {
        queue->tail = queue->head;
    }
    
    *msg = node->message;
    free(node);
    queue->size--;
    
    pthread_cond_signal(&queue->cond_not_full);
    pthread_mutex_unlock(&queue->mutex);
    
    return PUBSUB_SUCCESS;
}

size_t tsqueue_size(thread_safe_queue_t* queue) {
    if (!queue) {
        return 0;
    }
    
    pthread_mutex_lock(&queue->mutex);
    size_t size = queue->size;
    pthread_mutex_unlock(&queue->mutex);
    
    return size;
}

bool tsqueue_is_empty(thread_safe_queue_t* queue) {
    return tsqueue_size(queue) == 0;
}

void tsqueue_shutdown(thread_safe_queue_t* queue) {
    if (!queue) {
        return;
    }
    
    pthread_mutex_lock(&queue->mutex);
    queue->shutdown = true;
    pthread_cond_broadcast(&queue->cond_not_empty);
    pthread_cond_broadcast(&queue->cond_not_full);
    pthread_mutex_unlock(&queue->mutex);
}
