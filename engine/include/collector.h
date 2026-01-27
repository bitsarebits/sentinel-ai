/**
 * collector.h
 *
 * module: Data Collector
 *
 * Responsible for exporting packet features to persistent storage (CSV).
 * Used for building the training dataset for the AI model.
 */

#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <stdint.h>

/**
 * @brief The Feature Vector extracted from a raw packet.
 * This structure corresponds to a single row in the CSV dataset.
 */
typedef struct PacketFeatures
{
    double timestamp;    // Time of capture (Epoch seconds.microseconds)
    uint8_t protocol;    // Transport Protocol (6=TCP, 17=UDP)
    uint16_t src_port;   // Source Port (Host Byte Order)
    uint16_t dest_port;  // Destination Port (Host Byte Order)
    uint16_t packet_len; // Total size of the packet (Header + Payload)
    uint16_t tcp_flags;  // Bitmask of TCP Flags (SYN, ACK, FIN, etc.) - 0 for UDP
} PacketFeatures;

/**
 * @brief Initializes the CSV file for writing.
 * Opens the file and writes the header row if the file is new.
 *
 * @param filename Path to the CSV file (e.g., "data/training_set.csv").
 * @return int 0 on success, -1 on failure.
 */
int collector_init(const char *filename);

/**
 * @brief Appends a single packet record to the CSV file.
 * This function uses buffered I/O, so it is non-blocking in most cases.
 *
 * @param features Pointer to the struct containing extracted data.
 * @return int 0 on success, -1 on write error.
 */
int collector_record(const PacketFeatures *features);

/**
 * @brief Flushes buffers and closes the file handle.
 */
void collector_close(void);

#endif