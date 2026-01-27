/**
 * ringbuffer.h
 *
 * Thread-safe Circular Buffer Interface.
 * Encapsulates the synchronization logic (Mutex/CondVars) for the
 * Producer-Consumer pattern between Sniffer and Analyzer.
 */

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include "common.h" // Definitions of RingBuffer, PacketSlot, keep_running

/**
 * @brief Initializes the Ring Buffer resources.
 * Sets indices to 0 and initializes mutex and condition variables.
 *
 * @param rb Pointer to the RingBuffer to initialize.
 */
void ring_buffer_init(RingBuffer *rb);

/**
 * @brief Cleans up Ring Buffer resources.
 * Destroys mutex and condition variables.
 *
 * @param rb Pointer to the RingBuffer to clean up.
 */
void ring_buffer_cleanup(RingBuffer *rb);

/**
 * @brief Pushes a packet into the buffer (Producer).
 * Blocks if the buffer is full. Handles thread shutdown signals.
 *
 * @param rb Pointer to the RingBuffer.
 * @param slot Pointer to the packet data to copy.
 */
void ring_buffer_push(RingBuffer *rb, const PacketSlot *slot);

/**
 * @brief Pops a packet from the buffer (Consumer).
 * Blocks if the buffer is empty. Handles thread shutdown signals.
 *
 * @param rb Pointer to the RingBuffer.
 * @param out_slot Pointer to the struct where data will be copied.
 * @return int 1 on success, 0 if the thread is shutting down and buffer is empty.
 */
int ring_buffer_pop(RingBuffer *rb, PacketSlot *out_slot);

#endif