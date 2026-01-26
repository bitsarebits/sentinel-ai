/*
 * common.h
 *
 * Core shared definitions for Sentinel AI.
 * This file contains data structures and constants accessed by multiple threads.
 *
 * Key components:
 * 1. Configuration constants (Queue size, Max packet size).
 * 2. The 'PacketSlot' structure (container for a raw captured packet).
 * 3. The 'RingBuffer' structure (Circular Queue for thread-safe data exchange).
 * 4. Synchronization primitives (Mutex and Condition Variables).
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

// --- CONFIGURATION ---

#define QUEUE_SIZE 2048       // Max number of packets the buffer can hold
#define MAX_PACKET_SIZE 65535 // Max size of a standard TCP/IP packet (64KB)

// --- DATA STRUCTURES ---

/**
 * @brief Represents a single slot in the ring buffer.
 * Holds the raw data of one network packet.
 */
typedef struct PacketSlot
{
    uint8_t data[MAX_PACKET_SIZE];
    uint16_t length; // Actual length of the captured data
} PacketSlot;

/**
 * @brief Thread-safe Circular Buffer (Ring Buffer).
 * Acts as the shared memory channel between the Sniffer (Producer) and Analyzer (Consumer).
 */
typedef struct RingBuffer
{
    PacketSlot buffer[QUEUE_SIZE]; // Static array storage (no malloc needed at runtime)
    int head;                      // Write index (Producer inserts here)
    int tail;                      // Read index (Consumer removes here)
    int count;                     // Current number of items in the buffer
    // -- Synchronization Primitives --
    pthread_mutex_t mutex;    // Locks access to head/tail/count
    pthread_cond_t not_empty; // Signal sent when data is added (wakes Consumer)
    pthread_cond_t not_full;  // Signal sent when space is freed (wakes Producer)
} RingBuffer;

// --- GLOBALS ---

// Flag to control the main loop of all threads.
// Defined in main.c, accessed here via extern.
// volatile (not cached by the compiler)
extern volatile bool keep_running;

#endif