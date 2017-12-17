//
//  ip.c
//  DoritOS
//
//  Created by Carl Friess on 16/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#include "ip.h"
#include "icmp.h"

#include <aos/aos.h>

#include <assert.h>

#include <netutil/htons.h>
#include <netutil/checksum.h>


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
    header->offset = lwip_ntohs((*(uint16_t *)(buf + 6)) & 0x1F);
    header->ttl = buf[8];
    header->protocol = buf[9];
    header->checksum = lwip_ntohs(*(uint16_t *)(buf + 10));
    header->src = lwip_ntohl(*(uint32_t *)(buf + 12));
    header->dest = lwip_ntohl(*(uint32_t *)(buf + 16));
    if (header->ihl > 5) {
        header->options = lwip_ntohl(*(uint32_t *)(buf + 20));
    }
    
    // Compute checksum
    *(uint16_t *)(buf + 10) = 0;
    if (header->checksum == inet_checksum((void *) buf,
                                          header->ihl > 5 ? 28 : 20)) {
        return 1;
    }
    
    // Check for fragmentation
    if (header->flags & 0x04) {
        return 2;
    }
    
    return 0;
    
}

void ip_handle_packet(uint8_t *buf, size_t len) {
    
    // Sanity check: minimum packet size
    assert(len > 20);
    
    // Parse and check the IP header
    struct ip_packet_header header;
    int e;
    if ((e = ip_parse_packet_header(buf, &header))) {
        debug_printf("INVALID PACKET: %d\n", e);
        return;
    }
    
    debug_printf("VALID PACKET\n");
    
    switch (header.protocol) {
        case 1: // ICMP
            icmp_handle_packet(&header,
                               buf + (header.ihl > 5 ? 28 : 20),
                               len - (header.ihl > 5 ? 28 : 20));
            break;
            
        default:
            debug_printf("Unknown protocol (%d) in received packet!\n",
                         (int) header.protocol);
            break;
    }
    
}
