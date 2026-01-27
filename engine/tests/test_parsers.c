#include <arpa/inet.h>
#include <string.h>

#include "test_framework.h"
#include "../include/protocol_defs.h"
#include "../include/parser.h"
#include "../include/collector.h"

// ==========================================
//   ETHERNET TESTS
// ==========================================

void test_ethernet_struct_mapping()
{
    // Mock Data: Ethernet Frame Header (14 bytes)
    // Dest MAC: AA:BB:CC:DD:EE:FF
    // Src MAC:  11:22:33:44:55:66
    // Ethertype: 0x0800 (IPv4)
    uint8_t raw_eth_bytes[] = {
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, // Dest
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, // Src
        0x08, 0x00                          // Ethertype
    };

    EthernetHeader *eth = (EthernetHeader *)raw_eth_bytes;

    // Verify MAC Address bytes directly
    ASSERT_INT_EQ(0xAA, eth->dest_mac[0], "Ethernet Dest MAC first byte matches");
    ASSERT_INT_EQ(0xFF, eth->dest_mac[5], "Ethernet Dest MAC last byte matches");

    ASSERT_INT_EQ(0x11, eth->src_mac[0], "Ethernet Src MAC first byte matches");

    // Verify Ethertype (Network to Host)
    ASSERT_INT_EQ(0x0800, ntohs(eth->ethertype), "Ethernet Ethertype should be IPv4 (0x0800)");
}

// ==========================================
//   IPv4 TESTS
// ==========================================

void test_ipv4_struct_mapping()
{
    // Mock Data: A standard IPv4 Header (20 bytes)
    // 0x45 = Version 4, IHL 5
    // 0x0032 = Total Length 50
    // 0x06 = Protocol TCP
    uint8_t raw_bytes[] = {
        0x45, 0x00, 0x00, 0x32, // Vers+IHL, TOS, Len
        0x00, 0x01, 0x00, 0x00, // ID, Flags
        0x40, 0x06, 0x00, 0x00, // TTL=64, Proto=TCP, Checksum (0)
        0x01, 0x02, 0x03, 0x04, // Src IP: 1.2.3.4
        0x05, 0x06, 0x07, 0x08  // Dst IP: 5.6.7.8
    };

    IPv4Header *ip = (IPv4Header *)raw_bytes;

    ASSERT_INT_EQ(4, ip->version_ihl >> 4, "IPv4 Version should be 4");
    ASSERT_INT_EQ(5, ip->version_ihl & 0x0F, "IPv4 IHL should be 5");
    ASSERT_INT_EQ(6, ip->protocol, "IPv4 Protocol should be TCP (6)");

    // Check Endianness conversion (Big Endian Network -> Host)
    ASSERT_INT_EQ(50, ntohs(ip->total_length), "IPv4 Total length should be 50");
}

// ==========================================
//   IPv6 TESTS
// ==========================================

void test_ipv6_struct_mapping()
{
    // Mock Data: IPv6 Header (40 bytes)
    // Version: 6 (High 4 bits of first byte) -> 0x60...
    // Next Header: 17 (UDP) -> 0x11
    // Hop Limit: 64 -> 0x40
    // Src IP: fe80::1 (Link Local)
    // Dst IP: ff02::1 (All Nodes Multicast)

    uint8_t raw_ipv6_bytes[40] = {
        0x60, 0x00, 0x00, 0x00, // Bytes 0-3: Ver(6) + Class + Flow
        0x00, 0x14,             // Bytes 4-5: Payload Len (20)
        0x11,                   // Byte 6:    Next Header (UDP)
        0x40,                   // Byte 7:    Hop Limit (64)

        // Src IP (fe80::1) - Bytes 8-23 (16 bytes total)
        // 0xfe, 0x80 (2 bytes) + 13 zeros + 0x01 (1 byte) = 16 bytes
        0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01,

        // Dst IP (ff02::1) - Bytes 24-39 (16 bytes total)
        // 0xff, 0x02 (2 bytes) + 13 zeros + 0x01 (1 byte) = 16 bytes
        0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01};

    IPv6Header *ip6 = (IPv6Header *)raw_ipv6_bytes;

    // 1. Check Version
    // The field 'ver_tc_flow' is 32 bits. In network order it is 0x60000000.
    // We must convert to host order first, then shift right by 28 bits.
    uint32_t ver_val = ntohl(ip6->ver_tc_flow);
    uint8_t version = (ver_val >> 28);
    ASSERT_INT_EQ(6, version, "IPv6 Version should be 6");

    // 2. Check Next Header
    ASSERT_INT_EQ(17, ip6->next_header, "IPv6 Next Header should be 17 (UDP)");

    // 3. Check Payload Length
    ASSERT_INT_EQ(20, ntohs(ip6->payload_len), "IPv6 Payload Len should be 20");

    // 4. Check IP Address String Conversion
    // We verify that inet_ntop correctly reads our raw bytes into a readable string
    char src_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, ip6->src_ip, src_str, INET6_ADDRSTRLEN);

    // strcmp returns 0 if strings are identical
    if (strcmp(src_str, "fe80::1") != 0)
    {
        printf(RED "[FAIL] IPv6 Src IP mismatch. Expected 'fe80::1', got '%s'\n" RESET, src_str);
        tests_failed++;
        return;
    }
    else
    {
        printf(GREEN "[PASS] IPv6 Source IP parsed correctly as fe80::1\n" RESET);
        tests_run++;
    }
}

// ==========================================
//   TCP TESTS
// ==========================================

void test_tcp_struct_mapping()
{
    // Mock Data: TCP Header (20 bytes standard)
    // Src Port: 80 (HTTP) -> 0x0050
    // Dst Port: 12345     -> 0x3039
    // Seq Num: 1000       -> 0x000003E8
    // Data Offset + Flags: 0x5012
    //    -> Data Offset: 5 (5 * 4 = 20 bytes) -> High 4 bits (0x5...)
    //    -> Flags: SYN (0x02) + ACK (0x10) = 0x12 -> Low bits (...012)

    uint8_t raw_tcp_bytes[] = {
        0x00, 0x50,             // Src Port
        0x30, 0x39,             // Dst Port
        0x00, 0x00, 0x03, 0xE8, // Seq Num (1000)
        0x00, 0x00, 0x00, 0x00, // Ack Num (0)
        0x50, 0x12,             // Data Offset (5) + Flags (SYN+ACK)
        0x00, 0x00,             // Window
        0x00, 0x00,             // Checksum
        0x00, 0x00              // Urgent Ptr
    };

    TCPHeader *tcp = (TCPHeader *)raw_tcp_bytes;

    // 1. Check Ports
    ASSERT_INT_EQ(80, ntohs(tcp->src_port), "TCP Source Port should be 80");
    ASSERT_INT_EQ(12345, ntohs(tcp->dest_port), "TCP Dest Port should be 12345");

    // 2. Check Sequence Number (32-bit conversion)
    ASSERT_INT_EQ(1000, ntohl(tcp->seq_num), "TCP Seq Num should be 1000");

    // 3. Check Data Offset (The critical bit manipulation)
    // Read 16 bits -> Host Order -> Shift Right 12 bits
    uint16_t raw_flags = ntohs(tcp->data_offset_flags);
    uint8_t data_offset = raw_flags >> 12;

    ASSERT_INT_EQ(5, data_offset, "TCP Data Offset should be 5");
    ASSERT_INT_EQ(20, data_offset * 4, "TCP Header Length should be 20 bytes");

    // 4. Check Flags (Masking the lower 9 bits)
    // 0x5012 & 0x01FF = 0x0012
    // 0x12 (hex) = 18 (decimal) = SYN (2) + ACK (16)
    uint16_t flags = raw_flags & 0x01FF;
    ASSERT_INT_EQ(0x12, flags, "TCP Flags should match SYN+ACK (0x12)");
}

// ==========================================
//   UDP TESTS
// ==========================================

void test_udp_struct_mapping()
{
    // Mock Data: A valid UDP Header (8 bytes)
    // Src Port: 12345 (0x3039)
    // Dst Port: 53    (0x0035 - DNS)
    // Length:   8     (0x0008)
    uint8_t raw_udp_bytes[] = {
        0x30, 0x39, // Src Port
        0x00, 0x35, // Dst Port
        0x00, 0x08, // Length
        0x00, 0x00  // Checksum (Ignored)
    };

    UDPHeader *udp = (UDPHeader *)raw_udp_bytes;

    // Use ntohs() because the raw bytes are in Network Order (Big Endian)
    ASSERT_INT_EQ(12345, ntohs(udp->src_port), "UDP Source Port should be 12345");
    ASSERT_INT_EQ(53, ntohs(udp->dest_port), "UDP Dest Port should be 53 (DNS)");
    ASSERT_INT_EQ(8, ntohs(udp->length), "UDP Length should be 8");
}

// ==========================================
//   REAL LOGIC TESTS
// ==========================================

void test_real_ipv4_logic()
{
    printf("Testing Real IPv4 Logic...\n");

    PacketFeatures features;
    memset(&features, 0, sizeof(PacketFeatures));

    // CASE 1: Valid IPv4 Packet
    // We explicitly size the array to 28 bytes to hold IP(20) + UDP(8)
    uint8_t valid_packet[28] = {
        0x45, 0x00, 0x00, 0x1C, // Total Len 28 (0x001C)
        0x00, 0x00, 0x00, 0x00,
        0x40, 0x11, 0x00, 0x00, // Proto 17 (UDP)
        0x7F, 0x00, 0x00, 0x01,
        0x7F, 0x00, 0x00, 0x01
        // Remaining 8 bytes are implicitly zeroed by C because we set size [28]
    };

    // We must ensure the dummy UDP header has a valid length field to pass check.
    // UDP Header format: [Src(2)][Dst(2)][Len(2)][Cks(2)]
    // Offset is 20 (end of IP). Len is at offset 24.
    valid_packet[24] = 0x00;
    valid_packet[25] = 0x08; // Length 8

    // Pass 28 bytes so parse_udp has data to read
    int result = parse_ipv4(valid_packet, 28, &features);
    ASSERT_INT_EQ(0, result, "Real IPv4 Parser should accept valid packet");

    // CASE 2: Invalid Version (6)
    uint8_t invalid_ver[] = {0x60, 0x00, 0x00, 0x00};
    result = parse_ipv4(invalid_ver, 4, &features);
    ASSERT_INT_EQ(-1, result, "Real IPv4 Parser should REJECT Version 6");

    // CASE 3: Truncated Header
    // We pass a valid header start, but claim we only have 10 bytes available
    result = parse_ipv4(valid_packet, 10, &features);
    ASSERT_INT_EQ(-1, result, "Real IPv4 Parser should REJECT truncated buffer");
}

void test_real_udp_logic()
{
    printf("Testing Real UDP Logic...\n");

    // Dummy PacketFeatures to avoid SegFault
    PacketFeatures features;
    memset(&features, 0, sizeof(PacketFeatures));

    // CASE 1: Valid UDP
    // Src=53, Dst=53, Len=8, Cks=0
    uint8_t valid_udp[] = {
        0x00, 0x35, 0x00, 0x35,
        0x00, 0x08, 0x00, 0x00};

    int result = parse_udp(valid_udp, 8, &features);
    ASSERT_INT_EQ(0, result, "Real UDP Parser should accept valid header");

    // CASE 2: Truncated Buffer
    // We pass the valid packet, but say we only have 4 bytes
    result = parse_udp(valid_udp, 4, &features);
    ASSERT_INT_EQ(-1, result, "Real UDP Parser should REJECT truncated buffer (<8 bytes)");

    // CASE 3: Logical Error (Declared Len < 8)
    uint8_t bad_len_udp[] = {
        0x00, 0x35, 0x00, 0x35,
        0x00, 0x04, 0x00, 0x00 // Declared Length is 4 (Impossible, header is 8)
    };
    result = parse_udp(bad_len_udp, 8, &features);
    ASSERT_INT_EQ(-1, result, "Real UDP Parser should REJECT impossible declared length");
}

void test_real_tcp_logic()
{
    printf("Testing Real TCP Logic...\n");

    // Dummy PacketFeatures to avoid SegFault
    PacketFeatures features;
    memset(&features, 0, sizeof(PacketFeatures));

    // CASE 1: Valid TCP (Standard 20 bytes)
    // Offset = 5 (0x50...) -> 20 bytes
    uint8_t valid_tcp[20] = {0}; // Zero init
    valid_tcp[12] = 0x50;        // Data Offset = 5

    int result = parse_tcp(valid_tcp, 20, &features);
    ASSERT_INT_EQ(0, result, "Real TCP Parser should accept valid standard header");

    // CASE 2: Truncated Buffer
    result = parse_tcp(valid_tcp, 15, &features);
    ASSERT_INT_EQ(-1, result, "Real TCP Parser should REJECT truncated buffer");

    // CASE 3: Invalid Offset (Offset = 4 -> 16 bytes. TCP min is 20)
    uint8_t bad_offset_tcp[20] = {0};
    bad_offset_tcp[12] = 0x40; // Data Offset = 4

    result = parse_tcp(bad_offset_tcp, 20, &features);
    ASSERT_INT_EQ(-1, result, "Real TCP Parser should REJECT Data Offset < 5");
}

void test_real_ipv6_logic()
{
    printf("Testing Real IPv6 Logic...\n");

    // Dummy PacketFeatures to avoid SegFault
    PacketFeatures features;
    memset(&features, 0, sizeof(PacketFeatures));

    // CASE 1: Valid IPv6 Packet (Next Header = UDP)
    // 0x60000000 (Ver 6) | Payload Len 8 | Next Head 17 (UDP) | Hop 64
    // Src: fe80::1, Dst: ff02::1
    uint8_t valid_packet[40] = {
        0x60, 0x00, 0x00, 0x00,
        0x00, 0x08, 0x11, 0x40,
        0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01,
        0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01};

    // We simulate a packet that has the header (40) + UDP header (8) = 48 bytes
    // But parse_ipv6 only receives the IPv6 body part, so we pass 48 bytes available.
    // However, the function just needs to read the header first.
    // Let's pass 40 bytes just to test the header parsing itself.
    int result = parse_ipv6(valid_packet, 40, &features);

    // It might try to call parse_udp with 0 bytes remaining.
    // parse_udp checks length and returns -1 if < 8.
    // So parse_ipv6 will return -1 because the chained parser failed.
    // THIS IS GOOD! It means dispatch worked.
    // To make it return 0, we need to give it 8 extra bytes for the UDP dummy.

    uint8_t full_packet[48];
    memcpy(full_packet, valid_packet, 40);
    // Add dummy UDP header at the end (Src, Dst, Len=0, Cks)
    memset(full_packet + 40, 0, 8);
    // Fix UDP len to 8 to pass UDP check
    full_packet[44] = 0x00;
    full_packet[45] = 0x08;

    result = parse_ipv6(full_packet, 48, &features);
    ASSERT_INT_EQ(0, result, "Real IPv6 Parser should accept valid packet");

    // CASE 2: Invalid Version (4 instead of 6)
    uint8_t invalid_ver[40] = {0};
    invalid_ver[0] = 0x40; // Version 4
    result = parse_ipv6(invalid_ver, 40, &features);
    ASSERT_INT_EQ(-1, result, "Real IPv6 Parser should REJECT Version 4");

    // CASE 3: Truncated
    result = parse_ipv6(valid_packet, 20, &features); // Only 20 bytes available
    ASSERT_INT_EQ(-1, result, "Real IPv6 Parser should REJECT truncated buffer");
}

// ==========================================
//   MAIN RUNNER
// ==========================================

int main()
{
    printf("\n=== RUNNING PARSER TESTS ===\n");

    // --- STRUCT MAPPING TESTS (Data) ---
    test_ethernet_struct_mapping();
    test_ipv4_struct_mapping();
    test_ipv6_struct_mapping();
    test_udp_struct_mapping();
    test_tcp_struct_mapping();

    // --- REAL LOGIC TESTS (Code) ---
    test_real_ipv4_logic();
    test_real_ipv6_logic();
    test_real_udp_logic();
    test_real_tcp_logic();

    printf("\nTotal Tests Run: %d\n", tests_run);
    if (tests_failed == 0)
    {
        printf(GREEN "ALL TESTS PASSED\n" RESET);
        return 0;
    }
    else
    {
        printf(RED "TESTS FAILED: %d\n" RESET, tests_failed);
        return 1;
    }
}