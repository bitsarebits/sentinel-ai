/*
 * analyzer.h
 *
 * The "Consumer" module of the system.
 * Responsible for processing data retrieved from the RingBuffer.
 *
 * Primary responsibilities:
 * - Wait for data availability (wait on 'not_empty' condition).
 * - Read packet data from the RingBuffer.
 * - Parse protocols (Ethernet -> IP -> Transport Layer).
 * - (Future) Run ONNX inference for anomaly detection.
 * - (Future) Serialize results to JSON for the frontend.
 */

#ifndef ANALYZER_H
#define ANALYZER_H

#include "common.h"

/**
 * @brief Main entry point for the Analyzer Thread (Consumer).
 *
 * This function is responsible for the "Processing" phase of the pipeline.
 * It processes packets decoupled from the capture rate to avoid packet drops.
 *
 * Workflow:
 * 1. Enters a continuous loop checking the global 'keep_running' flag.
 * 2. Locks the RingBuffer mutex.
 * 3. Waits on the 'not_empty' condition variable if the buffer is empty.
 * 4. Reads the packet data from the 'tail' slot.
 * 5. Updates the 'tail' index and 'count'.
 * 6. Signals the 'not_full' condition variable (optional, for flow control).
 * 7. Unlocks the mutex.
 * 8. Processes the local copy of the packet:
 * - Parses Ethernet, IP, and TCP/UDP headers.
 * - (TODO) Passes features to the AI inference engine.
 * - (TODO) Serializes output to JSON for the Frontend.
 *
 * @param arg Pointer to the shared 'RingBuffer' structure (casted to void*).
 * @return void* NULL when the thread terminates.
 */
void *analyzer_thread(void *ring_buffer);

#endif