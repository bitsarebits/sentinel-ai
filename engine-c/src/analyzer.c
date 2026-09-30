#include <stdio.h>
#include <stdio.h>

#include "analyzer.h"
#include "utils.h"
#include "parser.h"
#include "collector.h"
#include "ringBuffer.h"
#include "ai_engine.h"

// --- GLOBAL VARIABLES ---
// Hardcoded from training
// OLD: const float ANOMALY_THRESHOLD = 0.008596;
// NEW: Tuned based on live observation (0.018 was the highest 'normal' noise)
const float ANOMALY_THRESHOLD = 0.025;

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

            // Route based on Mode
            if (global_config.mode == MODE_TRAINING)
            {
                // Feature needs timestamp!
                collector_record(&features);
            }
            else if (global_config.mode == MODE_INFERENCE)
            {
                // AI inference

                // If protocol is 0, it means we didn't parse a Transport Layer (ARP, ICMP, etc.)
                // Don't waste CPU cycles or confuse the AI with this.
                if (features.protocol == 0)
                {
                    continue;
                }

                // Analyze the packet with AI
                float anomaly_score = ai_engine_predict(&features);

                // Detect anomalies
                if (anomaly_score < 0)
                {
                    LOG("[ANALYZER] AI Prediction Error.\n");
                }
                else if (anomaly_score > ANOMALY_THRESHOLD)
                {
                    // Use RED
                    printf("\033[1;31m");
                    printf("\n[!!!] ANOMALY DETECTED (Score: %.6f) [!!!]\n", anomaly_score);
                    printf("---------------------------------------------\n");
                    printf("Timestamp:  %.6f\n", features.timestamp); // Fixed format
                    printf("Protocol:   %d\n", features.protocol);
                    printf("Src Port:   %d\n", features.src_port);
                    printf("Dst Port:   %d\n", features.dest_port);
                    printf("Length:     %d bytes\n", features.packet_len);
                    printf("Flags:      0x%02X\n", features.tcp_flags);
                    printf("---------------------------------------------\n");
                    printf("\033[0m");
                }
                else
                {
                    // Print "." for every normal packet to show life
                    printf(".");
                    fflush(stdout);
                }
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