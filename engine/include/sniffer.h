/*
 * sniffer.h
 *
 * The "Producer" module of the system.
 * Responsible for interfacing with the Network Interface Card (NIC) via libpcap.
 *
 * Primary responsibilities:
 * - Initialize pcap in Promiscuous Mode.
 * - Capture raw packets in a tight loop.
 * - Write packet data into the shared RingBuffer.
 * - Handle thread synchronization (signal 'not_empty' to the Analyzer).
 */

#ifndef SNIFFER_H
#define SNIFFER_H

#include "common.h"

/**
 * @brief Main entry point for the Sniffer Thread (Producer).
 *
 * This function is responsible for the "Acquisition" phase of the pipeline.
 * It operates in a tight loop to capture network traffic with minimal latency.
 *
 * Workflow:
 * 1. Initializes libpcap on the selected network interface.
 * 2. Compiles and applies any BPF filters (optional).
 * 3. Enters a capture loop (pcap_loop or pcap_next_ex).
 * 4. For every captured packet:
 * - Locks the RingBuffer mutex.
 * - Writes the raw packet data into the 'head' slot.
 * - Updates the 'head' index and 'count'.
 * - Signals the 'not_empty' condition variable to wake the Analyzer.
 * - Unlocks the mutex.
 *
 * @param ring_buffer Pointer to the shared 'RingBuffer' structure (casted to void*).
 * @return void* NULL when the thread terminates (usually via global shutdown flag).
 */
void *sniffer_thread(void *ring_buffer);

#endif