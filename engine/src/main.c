#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "common.h"
#include "sniffer.h"
#include "analyzer.h"
#include "utils.h"

// --- GLOBAL VARIABLES ---

// Implementation of the extern declared in common.h
volatile bool keep_running = true;

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

// --- MAIN ---

int main()
{
    printf("=== Sentinel AI Engine Starting ===\n");

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

    // Initialize Synchronization Primitives
    // These must be init's before any thread touches them
    if (pthread_mutex_init(&rb->mutex, NULL) != 0)
    {
        fprintf(stderr, "[MAIN] CRITICAL: Mutex init failed\n");
        free(rb);
        return EXIT_FAILURE;
    }

    if (pthread_cond_init(&rb->not_empty, NULL) != 0 || pthread_cond_init(&rb->not_full, NULL) != 0)
    {
        fprintf(stderr, "[MAIN] CRITICAL: CondVar init failed\n");
        free(rb);
        return EXIT_FAILURE;
    }

    // Initialize buffer indices
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;

    LOG("[MAIN] Memory initialized. Buffer Size: %lu MB\n", sizeof(RingBuffer) / 1024 / 1024);

    // Start Threads
    pthread_t sniffer_tid, analyzer_tid;

    LOG("[MAIN] Launching Sniffer Thread...\n");
    if (pthread_create(&sniffer_tid, NULL, sniffer_thread, (void *)rb) != 0)
    {
        fprintf(stderr, "[MAIN] Error creating Sniffer thread\n");
        free(rb);
        return EXIT_FAILURE;
    }

    LOG("[MAIN] Launching Analyzer Thread...\n");
    if (pthread_create(&analyzer_tid, NULL, analyzer_thread, (void *)rb) != 0)
    {
        fprintf(stderr, "[MAIN] Error creating Analyzer thread\n");
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
    pthread_mutex_destroy(&rb->mutex);
    pthread_cond_destroy(&rb->not_empty);
    pthread_cond_destroy(&rb->not_full);
    free(rb);

    printf("=== Sentinel AI Engine Shutdown Complete ===\n");
    return EXIT_SUCCESS;
}