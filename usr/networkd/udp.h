//
//  udp.h
//  DoritOS
//
//  Created by Carl Friess on 18/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#ifndef udp_h
#define udp_h

#include <stdio.h>
#include <stdint.h>

#include <aos/aos.h>

#include <ip.h>


struct udp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
};

struct __attribute__ ((__packed__)) udp_checksum_ip_pseudo_header {
    uint32_t src_addr;
    uint32_t dest_addr;
    uint8_t zeros;
    uint8_t protocol;
    uint16_t udp_length;
    uint16_t checksum;  // Checksum of UDP header and payload
};
STATIC_ASSERT_SIZEOF(struct udp_checksum_ip_pseudo_header, 14);

// Parse and validate a UDP header
int udp_parse_header(struct ip_packet_header *ip, uint8_t *buf, size_t len,
                     struct udp_header *header);

void udp_handle_packet(struct ip_packet_header *ip, uint8_t *buf, size_t len);

#endif /* udp_h */
