#include "common.h"
#include "ringBuffer.h"
#include <string.h>
#include <stdio.h>

int ring_buffer_init(RingBuffer *rb)
{
    if (!rb)
        return -1; // Safety check

    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;

    // Initialize Mutex
    if (pthread_mutex_init(&rb->mutex, NULL) != 0)
    {
        return -1;
    }

    // Initialize Condition Variables
    if (pthread_cond_init(&rb->not_empty, NULL) != 0)
    {
        // Cleanup the mutex we just created before failing
        pthread_mutex_destroy(&rb->mutex);
        return -1;
    }

    if (pthread_cond_init(&rb->not_full, NULL) != 0)
    {
        // Cleanup everything created so far
        pthread_cond_destroy(&rb->not_empty);
        pthread_mutex_destroy(&rb->mutex);
        return -1;
    }

    return 0; // Success
}

void ring_buffer_cleanup(RingBuffer *rb)
{
    pthread_mutex_destroy(&rb->mutex);
    pthread_cond_destroy(&rb->not_empty);
    pthread_cond_destroy(&rb->not_full);
}

void ring_buffer_push(RingBuffer *rb, const PacketSlot *slot)
{
    pthread_mutex_lock(&rb->mutex);

    // Wait while buffer is full
    while (rb->count == QUEUE_SIZE && keep_running)
    {
        pthread_cond_wait(&rb->not_full, &rb->mutex);
    }

    if (!keep_running)
    {
        pthread_mutex_unlock(&rb->mutex);
        return;
    }

    // Write data
    rb->buffer[rb->head] = *slot; // Copy struct
    rb->head = (rb->head + 1) % QUEUE_SIZE;
    rb->count++;

    pthread_cond_signal(&rb->not_empty);
    pthread_mutex_unlock(&rb->mutex);
}

int ring_buffer_pop(RingBuffer *rb, PacketSlot *out_slot)
{
    pthread_mutex_lock(&rb->mutex);

    // Wait while buffer is empty
    while (rb->count == 0 && keep_running)
    {
        pthread_cond_wait(&rb->not_empty, &rb->mutex);
    }

    if (!keep_running && rb->count == 0)
    {
        pthread_mutex_unlock(&rb->mutex);
        return 0; // Shutdown
    }

    // Read data
    *out_slot = rb->buffer[rb->tail]; // Copy struct
    rb->tail = (rb->tail + 1) % QUEUE_SIZE;
    rb->count--;

    pthread_cond_signal(&rb->not_full);
    pthread_mutex_unlock(&rb->mutex);
    return 1; // Success
}