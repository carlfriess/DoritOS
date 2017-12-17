//
//  ip.h
//  DoritOS
//
//  Created by Carl Friess on 16/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#ifndef ip_h
#define ip_h

#include <stdio.h>
#include <stdint.h>


struct ip_packet_header {
    uint8_t version;    // Version
    uint8_t ihl;        // Internet Header Length
    uint16_t dscp;      // Differentiated Services Code Point
    uint8_t ecn;        // Explicit Congestion Notification
    uint16_t length;    // Length of entire IP Packet
    uint16_t ident;     // Identification number
    uint8_t flags;      // Flags
    uint16_t offset;    // Fragment Offset
    uint8_t ttl;        // Time to Live
    uint8_t protocol;   // Protocol
    uint16_t checksum;  // Header Checksum
    uint32_t src;       // Source IP
    uint32_t dest;      // Destination IP
    uint32_t options;   // Optional options field
};

// Parse and validate an IP header
int ip_parse_packet_header(uint8_t *buf, struct ip_packet_header *header);

// Encode an IP header
void ip_encode_packet_header(struct ip_packet_header *header, uint8_t *buf);

void ip_handle_packet(uint8_t *buf, size_t len);


#endif /* ip_h */
