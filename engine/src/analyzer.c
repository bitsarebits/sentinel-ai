#include <stdio.h>
#include <stdio.h>

#include "analyzer.h"
#include "utils.h"
#include "parser.h"

// --- ANALYZER THREAD ---

void *analyzer_thread(void *ring_buffer)
{
    printf("[ANALYZER] Thread started\n");

    // Cast dell'argomento
    RingBuffer *rb = (RingBuffer *)ring_buffer;

    while (keep_running)
    {
        // Lock the mutex
        pthread_mutex_lock(&rb->mutex);

        // Wait if the buffer is empty
        while (rb->count == 0 && keep_running)
            pthread_cond_wait(&rb->not_empty, &rb->mutex);

        // Check for shut downs during the waiting
        if (!keep_running)
        {
            pthread_mutex_unlock(&rb->mutex);
            break;
        }

        // Read from the buffer (stack copy)
        PacketSlot slot = rb->buffer[rb->tail];

        // Increment the pointer and decrement the counter
        rb->tail = (rb->tail + 1) % QUEUE_SIZE; // ring
        rb->count--;

        // Save the counter for later print
        int current_count = rb->count;

        // Wake up the producer and unluck the mutex
        pthread_cond_signal(&rb->not_full);
        pthread_mutex_unlock(&rb->mutex);

        // Process the packet
        LOG("[ANALYZER] Processing packet. Size: %d bytes. Count in buffer: %d\n", slot.length, current_count);
        analyze_packet(slot.data, slot.length);
    }

    // SHUTDOWN SEQUENCE: Wake up the producer just in case
    pthread_mutex_lock(&rb->mutex);
    pthread_cond_broadcast(&rb->not_full);
    pthread_mutex_unlock(&rb->mutex);
    printf("[ANALYZER] Thread exiting...\n");
    return NULL;
}