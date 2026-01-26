/*
 * protocol_defs.h
 *
 * Network Protocol Data Structures.
 * Defines the memory layout of standard network headers (Ethernet, IPv4, TCP, UDP).
 *
 * NOTE: All structures must be packed (__attribute__((packed))) to prevent
 * compiler padding, ensuring strict alignment with raw packet bytes.
 *
 * Usage: Casting raw byte pointers (uint8_t*) to these structure types
 * allows easy access to protocol fields (e.g., src_ip, dst_port).
 */
#ifndef PROTOCOL_DEFS_H
#define PROTOCOL_DEFS_H

#include <stdint.h>

// --- ETHERNET HEADER (Layer 2) ---
typedef struct __attribute__((packed)) EthernetHeader
{
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype; // Remember to use ntohs()
} EthernetHeader;

// --- IPv4 HEADER (Layer 3) ---
typedef struct __attribute__((packed)) IPv4Header
{
    uint8_t version_ihl;        // 1 byte: Version (4 bit) + Internet Header Length (header length) (4 bit)
    uint8_t tos;                // 1 byte: Type of Service
    uint16_t total_length;      // 2 byte: Total length (Header + data) -> ntohs()
    uint16_t id;                // 2 byte: ID (unique for package)
    uint16_t flags_frag_offset; // 2 byte: Flags and Offset fragmentation
    uint8_t ttl;                // 1 byte: Time To Live
    uint8_t protocol;           // 1 byte: Protocol (1=ICMP, 6=TCP, 17=UDP)
    uint16_t checksum;          // 2 byte: Checksum for errors
    uint32_t src_ip;            // 4 byte: Source IP -> ntohl() o inet_ntoa
    uint32_t dest_ip;           // 4 byte: Destination IP
} IPv4Header;

// --- IPv6 HEADER (Layer 3) ---
typedef struct __attribute__((packed)) IPv6Header
{
    // First 4 bytes contain: Version (4b), Traffic Class (8b), Flow Label (20b)
    // We read this as a single 32-bit block and use bitwise ops to extract values.
    uint32_t ver_tc_flow;

    uint16_t payload_len; // Length of the payload (excluding this header)
    uint8_t next_header;  // Identifies the next protocol (like 'protocol' in IPv4)
    uint8_t hop_limit;    // Replaces TTL
    uint8_t src_ip[16];   // Source IP (128-bit)
    uint8_t dest_ip[16];  // Dest IP (128-bit)
} IPv6Header;

// --- TCP HEADER (Layer 4) ---
typedef struct __attribute__((packed)) TCPHeader
{
    uint16_t src_port;  // Source Port
    uint16_t dest_port; // Destination Port
    uint32_t seq_num;   // Sequence Number (for packet ordering)
    uint32_t ack_num;   // Acknowledgment Number (for confirmation)
    // This 16-bit field contains:
    // - Data Offset (4 bits): Header length in 32-bit words
    // - Reserved (3 bits): Must be zero
    // - Flags (9 bits): NS, CWR, ECE, URG, ACK, PSH, RST, SYN, FIN
    uint16_t data_offset_flags;
    uint16_t window_size; // Flow control window size
    uint16_t checksum;    // Error checking
    uint16_t urgent_ptr;  // Points to urgent data (if URG flag is set)
} TCPHeader;

// --- UDP HEADER (Layer 4) ---
typedef struct __attribute__((packed)) UDPHeader
{
    uint16_t src_port;  // Source Port
    uint16_t dest_port; // Destination Port
    uint16_t length;    // Length of header + data
    uint16_t checksum;  // Optional in IPv4, mandatory in IPv6
} UDPHeader;

#endif