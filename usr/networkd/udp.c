//
//  udp.c
//  DoritOS
//
//  Created by Carl Friess on 18/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#include "udp.h"

#include <aos/aos.h>

#include <netutil/htons.h>
#include <netutil/checksum.h>


// Parse and validate a UDP header
int udp_parse_header(struct ip_packet_header *ip, uint8_t *buf, size_t len,
                     struct udp_header *header) {
    
    uint16_t *buf16 = (uint16_t *) buf;
    
    // Parse all fields
    header->src_port = lwip_ntohs(buf16[0]);
    header->dest_port = lwip_ntohs(buf16[1]);
    header->length = lwip_ntohs(buf16[2]);
    header->checksum = lwip_ntohs(buf16[3]);

    // Compute and verify checksum
    struct udp_checksum_ip_pseudo_header ph;
    ph.src_addr = lwip_htonl(ip->src);
    ph.dest_addr = lwip_htonl(ip->dest);
    ph.zeros = 0;
    ph.protocol = IP_PROTOCOL_UDP;
    ph.udp_length = buf16[2];
    ph.checksum = ~inet_checksum((void *) buf, len);
    
    uint16_t checksum = inet_checksum((void *) &ph, sizeof(struct udp_checksum_ip_pseudo_header));
    if (checksum != 0xFFFF && checksum != 0x0) {
        return 1;
    }
    
    return 0;
    
}

void udp_handle_packet(struct ip_packet_header *ip, uint8_t *buf, size_t len) {
    
    // Discard too small messages
    if (len < 8) {
        debug_printf("INVALID UDP PACKET: TOO SHORT\n");
        return;
    }
    
    // Parse and check the UDP header
    struct udp_header header;
    int e;
    if ((e = udp_parse_header(ip, buf, len, &header))) {
        debug_printf("INVALID UDP PACKET: %d\n", e);
        return;
    }
    
}
