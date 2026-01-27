#include <stdio.h>
#include <stdio.h>

#include "analyzer.h"
#include "utils.h"
#include "parser.h"
#include "collector.h"
#include "ringBuffer.h"

// --- ANALYZER THREAD ---

void *analyzer_thread(void *ring_buffer)
{
    printf("[ANALYZER] Thread started\n");

    // Cast dell'argomento
    RingBuffer *rb = (RingBuffer *)ring_buffer;

    // Prepare the variables
    PacketSlot slot;
    PacketFeatures features;

    while (keep_running)
    {
        // Pop is blocking and thread-safe
        if (ring_buffer_pop(rb, &slot) == 0)
        {
            break; // Shutdown signal received
        }

        // Process the packet
        LOG("[ANALYZER] Processing packet. Size: %d bytes.\n", slot.length);
        // Initialize the collector

        // Parse packet
        if (analyze_packet(slot.data, slot.length, &features) == 0)
        {
            features.timestamp = (double)slot.ts.tv_sec + (double)slot.ts.tv_usec / 1000000.0;
            features.packet_len = slot.length;

            // 2. Route based on Mode
            if (global_config.mode == MODE_TRAINING)
            {
                // Feature needs timestamp!
                collector_record(&features);
            }
            else if (global_config.mode == MODE_INFERENCE)
            {
                // Future AI call here
            }
        }
    }

    // SHUTDOWN SEQUENCE: Wake up the producer just in case
    pthread_mutex_lock(&rb->mutex);
    pthread_cond_broadcast(&rb->not_full);
    pthread_mutex_unlock(&rb->mutex);
    printf("[ANALYZER] Thread exiting...\n");
    return NULL;
}