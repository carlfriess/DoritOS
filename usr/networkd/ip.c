//
//  ip.c
//  DoritOS
//
//  Created by Carl Friess on 16/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#include "ip.h"
#include "icmp.h"
#include "slip.h"

#include <aos/aos.h>

#include <assert.h>

#include <netutil/htons.h>
#include <netutil/checksum.h>


// IP address of this host
static uint32_t host_ip;

// Identification number for outgoing packets
static uint32_t ident_counter = 0;

// Set the IP address of this host
void ip_set_ip_address(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    
    uint8_t *p = (uint8_t *) &host_ip;
    
    p[3] = a;
    p[2] = b;
    p[1] = c;
    p[0] = d;
    
}

// Parse and validate an IP header
int ip_parse_packet_header(uint8_t *buf, struct ip_packet_header *header) {
    
    // Parse each field
    header->version = (buf[0] >> 4) & 0x0F;
    header->ihl = buf[0] & 0x0F;
    header->dscp = (buf[1] >> 2) & 0x3F;
    header->ecn = buf[1] & 0x03;
    header->length = lwip_ntohs(*(uint16_t *)(buf + 2));
    header->ident = lwip_ntohs(*(uint16_t *)(buf + 4));
    header->flags = (buf[6] >> 5) & 0x07;
    header->offset = lwip_ntohs((*(uint16_t *)(buf + 6)) & 0xFF1F);
    header->ttl = buf[8];
    header->protocol = buf[9];
    header->checksum = lwip_ntohs(*(uint16_t *)(buf + 10));
    header->src = lwip_ntohl(*(uint32_t *)(buf + 12));
    header->dest = lwip_ntohl(*(uint32_t *)(buf + 16));
    if (header->ihl > 5) {
        header->options = lwip_ntohl(*(uint32_t *)(buf + 20));
    }
    
    // Compute checksum
    uint16_t checksum = inet_checksum((void *) buf, header->ihl * 4);
    if (checksum != 0xFFFF && checksum != 0x0) {
        return 1;
    }
    
    // Check for fragmentation
    if (header->flags & 0x04) {
        return 2;
    }
    
    return 0;
    
}

// Encode an IP header
void ip_encode_packet_header(struct ip_packet_header *header, uint8_t *buf) {
    
    // Set data fields
    buf[0] = header->version << 4;
    buf[0] |= header->ihl & 0x0F;
    buf[1] = header->dscp << 2;
    buf[1] |= header->ecn & 0x03;
    *(uint16_t *)(buf + 2) = lwip_htons(header->length);
    *(uint16_t *)(buf + 4) = lwip_htons(header->ident);
    buf[6] = header->flags << 5;
    buf[7] = 0;
    *(uint16_t *)(buf + 6) |= lwip_htons(header->offset) & 0xFF1F;
    buf[8] = header->ttl;
    buf[9] = header->protocol;
    *(uint32_t *)(buf + 12) = lwip_htonl(header->src);
    *(uint32_t *)(buf + 16) = lwip_htonl(header->dest);
    
    // Compute and set checksum
    *(uint16_t *)(buf + 10) = 0;
    header->checksum = inet_checksum((void *) buf, header->ihl * 4);
    *(uint16_t *)(buf + 10) = header->checksum;
    
}

// Send an IP protocol protocol header
void ip_send_header(uint32_t dest_ip, uint8_t protocol, size_t total_len) {
    
    struct ip_packet_header header;
    
    // Set header fields
    header.version = 4;
    header.ihl = 5;
    header.dscp = 0;
    header.ecn = 0;
    header.length = header.ihl * 4 + total_len;
    header.ident = ident_counter++;
    header.flags = 0x2;
    header.offset = 0;
    header.ttl = 32;
    header.protocol = protocol;
    header.src = host_ip;
    header.dest = dest_ip;
    
    // Encode header and send it
    uint8_t *buf_header = malloc(header.ihl * 4);
    assert(buf_header);
    ip_encode_packet_header(&header, buf_header);
    slip_send(buf_header, header.ihl * 4, false);
    free(buf_header);
    
}

// Send a buffer (following a header)
void ip_send(uint8_t *buf, size_t len, bool end) {
    
    // Send the payload
    slip_send(buf, len, end);
    
}

void ip_handle_packet(uint8_t *buf, size_t len) {
    
    // Discard too small packets
    if (len < 20) {
        debug_printf("INVALID PACKET: TOO SHORT\n");
        return;
    }
    
    // Parse and check the IP header
    struct ip_packet_header header;
    int e;
    if ((e = ip_parse_packet_header(buf, &header))) {
        debug_printf("INVALID PACKET: %d\n", e);
        return;
    }
        
    switch (header.protocol) {
        case IP_PROTOCOL_ICMP:
            icmp_handle_packet(header.src,
                               buf + (header.ihl * 4),
                               len - (header.ihl * 4));
            break;
            
        default:
            debug_printf("Unknown protocol (%d) in received packet!\n",
                         (int) header.protocol);
            break;
    }
    
}
