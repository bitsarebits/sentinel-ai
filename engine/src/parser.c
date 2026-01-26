#include <stdint.h>
#include <arpa/inet.h>

#include "parser.h"
#include "protocol_defs.h"
#include "utils.h"

int analyze_packet(const uint8_t *data, uint16_t len)
{
    // Safety check: the packet is bit enough to have an Ethernet header?
    if (len < sizeof(EthernetHeader))
    {
        LOG("[ANALYZER] Packet too short (runt).\n");
        return -1;
    }

    // Cast the header to the pointer
    EthernetHeader *eth = (EthernetHeader *)data;

    uint16_t ethertype = ntohs(eth->ethertype);

    LOG("[ANALYZER] Packet Len: %d | EtherType: 0x%04X | MAC Src: %02X:%02X:%02X:%02X:%02X:%02X\n",
        len, ethertype,
        eth->src_mac[0], eth->src_mac[1], eth->src_mac[2],
        eth->src_mac[3], eth->src_mac[4], eth->src_mac[5]);

    // Next layer
    if (ethertype == 0x0800)
    { // IPv4
        return parse_ipv4(data + sizeof(EthernetHeader), len - sizeof(EthernetHeader));
    }
    else if (ethertype == 0x0806)
    {
        LOG("  [ARP] Packet ignored for now.\n");
    }
    else if (ethertype == 0x86DD)
    {
        return parse_ipv6(data + sizeof(EthernetHeader), len - sizeof(EthernetHeader));
    }
    else if (ethertype == 0x88E1)
    {
        LOG("  [L2] HomePlug AV (Powerline) Control Packet.\n");
    }
    else
    {
        LOG("  [UNKNOWN] Unhandled EtherType: 0x%04X\n", ethertype);
    }

    return 0;
}

int parse_ipv4(const uint8_t *packet_body, uint16_t remaining_len)
{
    //  Minimum Standard Header Check
    if (remaining_len < sizeof(IPv4Header))
    {
        LOG("[ERROR] IPv4 segment too short: %d bytes (Need %lu)\n", remaining_len, sizeof(IPv4Header));
        return -1;
    }

    IPv4Header *ip = (IPv4Header *)packet_body;

    // Right shift 4 times to get the high 4 bits
    uint8_t version = ip->version_ihl >> 4;
    if (version != 4)
    {
        LOG("[ERROR] IPv4 Parser called on non-IPv4 packet (Version: %d)\n", version);
        return -1; // Return Error
    }

    // Bitwise AND with 00001111 (0x0F) to isolate low 4 bits
    uint8_t ihl = ip->version_ihl & 0x0F;

    // Calculate real Header Length in bytes
    uint16_t ip_header_len = ihl * 4;
    if (ip_header_len < 20)
    {
        LOG("[ERROR] IPv4 Header too short: %d bytes\n", ip_header_len);
        return -1;
    }

    LOG("--- IPv4 Header ---\n");
    LOG("Version: %d\n", version);
    LOG("Header Length: %d bytes\n", ip_header_len);

    // Local buffers for the strings (Thread-safe and Memory-safe)
    char src_str[INET_ADDRSTRLEN];
    char dst_str[INET_ADDRSTRLEN];

    // inet_ntop(Family, Pointer to Address, Destination Buffer, Buffer Size)
    inet_ntop(AF_INET, &ip->src_ip, src_str, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &ip->dest_ip, dst_str, INET_ADDRSTRLEN);

    LOG("Source IP: %s\n", src_str);
    LOG("Dest IP:   %s\n", dst_str);

    // Calculate the transport header start and parse it
    const uint8_t *transport_segment = packet_body + ip_header_len;

    switch (ip->protocol)
    {
    case 6: // TCP
        parse_tcp(transport_segment, remaining_len - ip_header_len);
        break;
    case 17: // UDP
        parse_udp(transport_segment, remaining_len - ip_header_len);
        break;
    case 1: // ICMP
        LOG("  [ICMP] Protocol found.\n");
        break;
    default:
        LOG("  [UNKNOWN] Protocol ID: %d\n", ip->protocol);
        break;
    }

    return 0;
}

int parse_ipv6(const uint8_t *packet_body, uint16_t remaining_len)
{
    // Static Length Check (Fixed 40 bytes)
    if (remaining_len < sizeof(IPv6Header))
    {
        LOG("[ERROR] IPv6 segment too short: %d bytes (Need 40)\n", remaining_len);
        return -1;
    }

    IPv6Header *ip6 = (IPv6Header *)packet_body;

    // Extract Fields from the 32-bit 'ver_tc_flow'
    uint32_t vtf = ntohl(ip6->ver_tc_flow);

    uint8_t version = (vtf >> 28) & 0x0F;
    uint8_t traffic_class = (vtf >> 20) & 0xFF;
    uint32_t flow_label = vtf & 0xFFFFF;

    if (version != 6)
    {
        LOG("[ERROR] IPv6 Parser called on non-IPv6 packet (Version: %d)\n", version);
        return -1;
    }

    // Extract Details
    uint16_t payload_len = ntohs(ip6->payload_len);
    uint8_t next_header = ip6->next_header;
    uint8_t hop_limit = ip6->hop_limit;

    LOG("--- IPv6 Header ---\n");
    LOG("Version:     %d\n", version);
    LOG("Traffic Class: 0x%02X\n", traffic_class);
    LOG("Flow Label:    0x%05X\n", flow_label);
    LOG("Payload Len: %d\n", payload_len);
    LOG("Hop Limit:   %d\n", hop_limit);

    // Address Conversion
    char src_str[INET6_ADDRSTRLEN];
    char dst_str[INET6_ADDRSTRLEN];

    inet_ntop(AF_INET6, ip6->src_ip, src_str, INET6_ADDRSTRLEN);
    inet_ntop(AF_INET6, ip6->dest_ip, dst_str, INET6_ADDRSTRLEN);

    LOG("Source IP:   %s\n", src_str);
    LOG("Dest IP:     %s\n", dst_str);

    // EXTENSION HEADER LOOP
    // We must skip over "Extension Headers" to find the Transport Layer.

    uint16_t current_offset = sizeof(IPv6Header);

    // Safety: ensure we don't loop forever or buffer read
    // We limit the number of headers to avoid DoS attacks (e.g., infinite loops)
    int hops = 0;
    const int MAX_EXTENSION_HEADERS = 10;

    while (hops < MAX_EXTENSION_HEADERS)
    {
        // Check if we have reached the end of the packet data
        if (current_offset >= remaining_len)
        {
            LOG("[WARN] IPv6 Packet truncated while parsing headers.\n");
            return -1;
        }

        // Is the current header a Transport Protocol?
        if (next_header == 6 || next_header == 17 || next_header == 58)
        {
            break; // Found it! Exit loop and parse.
        }

        if (next_header == 59)
        {
            LOG("  [IPv6] No Next Header (End of chain).\n");
            return 0;
        }

        // --- Handle Extension Headers ---
        // Most extension headers (Hop-by-Hop(0), Routing(43), DestOpt(60))
        // have the generic format: [ NextHeader(1B) | Len(1B) | Data... ]
        // Len is in 8-byte units, NOT including the first 8 bytes.

        // Safety: Can we read the 2 bytes (NextHeader + Len)?
        if (current_offset + 2 > remaining_len)
        {
            return -1;
        }

        const uint8_t *ext_hdr = packet_body + current_offset;
        uint8_t ext_next_header = ext_hdr[0];
        uint8_t ext_len_units = ext_hdr[1];

        // RFC 8200: Length in 8-octet units, not including the first 8 octets.
        // Actual Size = (ext_len_units + 1) * 8
        uint16_t ext_byte_len = (ext_len_units + 1) * 8;

        LOG("  [IPv6 Ext] Skipping Header Type %d (Len: %d bytes)\n", next_header, ext_byte_len);

        // Advance
        current_offset += ext_byte_len;
        next_header = ext_next_header; // Update type for the next iteration
        hops++;
    }

    // Dispatch to Transport Layer
    const uint8_t *transport_segment = packet_body + current_offset;
    uint16_t transport_len = 0;
    if (remaining_len > current_offset)
    {
        transport_len = remaining_len - current_offset;
    }
    else
    {
        LOG("[ERROR] IPv6 Header chain exceeded packet length.\n");
        return -1;
    }

    // Optional: Warn if payload is smaller than declared
    if (transport_len < payload_len)
    {
        LOG("[WARN] IPv6 captured length (%d) < declared payload (%d)\n", transport_len, payload_len);
    }

    switch (next_header)
    {
    case 6: // TCP
        return parse_tcp(transport_segment, transport_len);
    case 17: // UDP
        return parse_udp(transport_segment, transport_len);
    case 58: // ICMPv6
        LOG("  [ICMPv6] Control Message found.\n");
        return 0;
    default:
        LOG("  [UNKNOWN] Final Protocol: %d\n", next_header);
        return 0;
    }
}

int parse_tcp(const uint8_t *segment, uint16_t remaining_len)
{
    //  Minimum Standard Header Check
    if (remaining_len < sizeof(TCPHeader))
    {
        LOG("[ERROR] TCP segment too short: %d bytes (Need 20)\n", remaining_len);
        return -1;
    }

    TCPHeader *tcp = (TCPHeader *)segment;

    // 1. Convert to Host Byte Order to read the value correctly
    uint16_t raw_offset_flags = ntohs(tcp->data_offset_flags);

    // 2. Extract Data Offset (First 4 bits)
    // Shift right by 12 positions to move the top 4 bits to the bottom
    uint8_t data_offset = (raw_offset_flags >> 12);

    // 3. Calculate Real Header Length in Bytes
    // The offset counts 32-bit words (4 bytes chunks), just like IP IHL
    uint16_t tcp_header_len = data_offset * 4;

    // Validate Dynamic Header Length
    if (tcp_header_len < 20)
    {
        LOG("[ERROR] TCP Header Length invalid: %d bytes\n", tcp_header_len);
        return -1;
    }

    if (remaining_len < tcp_header_len)
    {
        LOG("[ERROR] TCP Header truncated. Calc Len: %d, Actual: %d\n", tcp_header_len, remaining_len);
        return -1;
    }

    LOG("--- TCP Header ---\n");
    LOG("Source Port: %u\n", ntohs(tcp->src_port)); // ntohs for 16 bit
    LOG("Destination Port: %u\n", ntohs(tcp->dest_port));
    LOG("Sequence Number: %u\n", ntohl(tcp->seq_num)); // ntohl for 32 bit
    LOG("Acknowledgment Number: %u\n", ntohl(tcp->ack_num));

    LOG("TCP Header Length: %d bytes\n", tcp_header_len);

    if (tcp_header_len > 20)
    {
        LOG("  [!] TCP Options present (%d bytes)\n", tcp_header_len - 20);
        // TODO Logic to parse options would go here
    }

    // 5. Extract Reserved (from bit 4 to 6)
    // Mask with 0x0E00 (binary 0000 1110 0000 0000)
    uint8_t reserved = (raw_offset_flags & 0x0E00) >> 9;

    LOG("Reserved: %u\n", reserved);

    // 5. Extract Flags (The lower 9 bits)
    // Mask with 0x01FF (binary 0000 0001 1111 1111)
    uint16_t flags = raw_offset_flags & 0x01FF;
    LOG("Flags:");

    if (flags & 0x0002)
        LOG("  [SYN] "); // SYN flag is bit 1 (value 2)
    if (flags & 0x0010)
        LOG("  [ACK] "); // ACK flag is bit 4 (value 16)
    if (flags & 0x0001)
        LOG("  [FIN] "); // FIN flag is bit 0 (value 1)
    LOG("\n");

    /*     // 6. Calculate Payload Position
        const uint8_t *payload = segment + tcp_header_len; */

    return 0;
}

int parse_udp(const uint8_t *segment, uint16_t remaining_len)
{
    // Minimum Length Check
    if (remaining_len < sizeof(UDPHeader))
    {
        LOG("[ERROR] UDP segment too short: %d bytes (Need 8)\n", remaining_len);
        return -1;
    }

    UDPHeader *udp = (UDPHeader *)segment;
    uint16_t declared_len = ntohs(udp->length);

    // Logical Consistency Check
    // The declared length in the UDP header includes the header itself (8 bytes).
    if (declared_len < 8)
    {
        LOG("[ERROR] Malformed UDP Length: %d\n", declared_len);
        return -1;
    }

    // Strict check: Do we actually have all the data the header claims?
    // Note: Sometimes we capture truncated packets intentionally (snaplen), so allow this warning
    // instead of a hard error if you prefer, but usually -1 is safer.
    if (remaining_len < declared_len)
    {
        LOG("[WARN] UDP Payload truncated (Captured: %d, Declared: %d)\n", remaining_len, declared_len);
        // return -1; // Uncomment if you want strict enforcement
    }

    LOG("--- UDP Header ---\n");
    LOG("Source Port: %u\n", ntohs(udp->src_port));
    LOG("Dest Port:   %u\n", ntohs(udp->dest_port));
    LOG("Length:      %u\n", declared_len);

    return 0; // Success
}
