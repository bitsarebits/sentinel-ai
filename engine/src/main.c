#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>

#include "common.h"
#include "sniffer.h"
#include "analyzer.h"
#include "utils.h"
#include "ringBuffer.h"
#include "collector.h"

// --- GLOBAL VARIABLES ---

// Implementation of the extern declared in common.h
volatile bool keep_running = true;

// Define the global config
AppConfig global_config = {.mode = MODE_SNIFFER_ONLY}; // Default

/**
 * @brief Handles Ctrl+C (SIGINT) to allow graceful shutdown.
 * Instead of killing the process immediately, we set a flag.
 * The threads will read this flag and exit their loops.
 */
void handle_sigint(int sig)
{
    (void)sig; // Silence unused parameter warning
    LOG("\n[MAIN] Caught signal (Ctrl+C). Stopping Sentinel AI...\n");
    keep_running = false;

    // NOTE: In a complex system, we might need to wake up sleeping threads here
    // by broadcasting on the condition variables, otherwise they might stick
    // waiting forever if no new packets arrive.
}

void print_usage()
{
    printf("Usage: sudo ./sentinel-engine [OPTION]\n");
    printf("Options:\n");
    printf("  --train    Run in Data Collection Mode (Save to CSV)\n");
    printf("  --guard    Run in AI Inference Mode (Active Detection)\n");
    printf("  (none)     Run in Sniffer/Debug Mode\n");
}

// --- MAIN ---

int main(int argc, char *argv[])
{
    printf("=== Sentinel AI Engine Starting ===\n");

    // Argument Parsing
    if (argc > 1)
    {
        if (strcmp(argv[1], "--train") == 0)
        {
            global_config.mode = MODE_TRAINING;
            printf("[CONFIG] Mode: TRAINING (Recording data...)\n");
        }
        else if (strcmp(argv[1], "--guard") == 0)
        {
            global_config.mode = MODE_INFERENCE;
            printf("[CONFIG] Mode: GUARD (AI Active)\n");
        }
        else
        {
            print_usage();
            return EXIT_FAILURE;
        }
    }
    else
    {
        printf("[CONFIG] Mode: DEBUG (Sniffer Only)\n");
    }

    // Setup Signal Handling
    signal(SIGINT, handle_sigint);

    // Allocate Ring Buffer on HEAP (Avoid Stack Overflow)
    // calloc to allocate and zero-out the memory immediately
    RingBuffer *rb = (RingBuffer *)calloc(1, sizeof(RingBuffer));
    if (!rb)
    {
        fprintf(stderr, "[MAIN] CRITICAL: Failed to allocate RingBuffer (Out of Memory)\n");
        return EXIT_FAILURE;
    }

    // Initialization (Encapsulated & Checked)
    if (ring_buffer_init(rb) != 0)
    {
        fprintf(stderr, "[MAIN] CRITICAL: Failed to initialize RingBuffer sync primitives\n");
        free(rb);
        return EXIT_FAILURE;
    }

    // Initialize resources based on mode
    if (global_config.mode == MODE_TRAINING)
    {
        if (collector_init("training_data.csv") != 0)
        {
            fprintf(stderr, "[MAIN] Error initializing the data collector\n");
            free(rb);
            return EXIT_FAILURE;
        }
    }

    LOG("[MAIN] Memory initialized. Buffer Size: %lu MB\n", sizeof(RingBuffer) / 1024 / 1024);

    // Start Threads
    pthread_t sniffer_tid, analyzer_tid;

    LOG("[MAIN] Launching Sniffer Thread...\n");
    if (pthread_create(&sniffer_tid, NULL, sniffer_thread, (void *)rb) != 0)
    {
        fprintf(stderr, "[MAIN] Error creating Sniffer thread\n");
        if (global_config.mode == MODE_TRAINING)
        {
            collector_close();
        }
        free(rb);
        return EXIT_FAILURE;
    }

    LOG("[MAIN] Launching Analyzer Thread...\n");
    if (pthread_create(&analyzer_tid, NULL, analyzer_thread, (void *)rb) != 0)
    {
        fprintf(stderr, "[MAIN] Error creating Analyzer thread\n");
        if (global_config.mode == MODE_TRAINING)
        {
            collector_close();
        }
        keep_running = false; // Stop sniffer
        pthread_join(sniffer_tid, NULL);
        return EXIT_FAILURE;
    }

    // Wait for threads to finish
    // The main thread is now blocked here until keep_running becomes false
    // and the child threads exit.
    pthread_join(sniffer_tid, NULL);
    pthread_join(analyzer_tid, NULL);

    LOG("[MAIN] All threads stopped. Cleaning up...\n");

    // Cleanup resources
    if (global_config.mode == MODE_TRAINING)
    {
        collector_close();
    }
    ring_buffer_cleanup(rb);
    free(rb);

    printf("=== Sentinel AI Engine Shutdown Complete ===\n");
    return EXIT_SUCCESS;
}