#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include "protocol_defs.h"
#include "collector.h"

// --- Public Interface for the Analyzer ---

/**
 * @brief Main entry point for packet analysis.
 * Parses the Ethernet header (Layer 2) and dispatches the payload to the appropriate
 * upper-layer parser (e.g., IPv4) based on the EtherType.
 *
 * @param data Pointer to the raw packet data captured by pcap.
 * @param len  Total length of the captured packet in bytes.
 * @return 0 on success, -1 if the packet was malformed or unrecognizable.
 */
int analyze_packet(const uint8_t *data, uint16_t len, PacketFeatures *features);

/**
 * @brief Extract and log IPv4 details.
 * @param packet_body Pointer to the start of the IP header (immediately after the Ethernet header).
 * @param remaining_len Safety check! How many bytes are left in the buffer?
 * @return 0 on success, -1 on failure (e.g. invalid version).
 */
int parse_ipv4(const uint8_t *packet_body, uint16_t remaining_len, PacketFeatures *features);

/**
 * @brief Extract and log IPv6 details.
 * @param packet_body Pointer to the start of the IP header (immediately after the Ethernet header).
 * @param remaining_len Safety check! How many bytes are left in the buffer?
 * @return 0 on success, -1 on failure (e.g. invalid version).
 */
int parse_ipv6(const uint8_t *packet_body, uint16_t remaining_len, PacketFeatures *features);

/**
 * @brief Extract and log TCP details (Ports, Flags, Seq).
 * @param segment Pointer to the start of the transport header (immediately after the IP header).
 * @param remaining_len Safety check! How many bytes are left in the buffer?
 * @return 0 on success, -1 on failure (e.g. invalid version).
 */
int parse_tcp(const uint8_t *segment, uint16_t remaining_len, PacketFeatures *features);

/**
 * @brief Extract and log UDP details.
 * @param segment Pointer to the start of the transport header (immediately after the IP header).
 * @param remaining_len Safety check! How many bytes are left in the buffer?
 * @return 0 on success, -1 on failure (e.g. invalid version).
 */
int parse_udp(const uint8_t *segment, uint16_t remaining_len, PacketFeatures *features);

#endif