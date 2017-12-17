//
//  icmp.h
//  DoritOS
//
//  Created by Carl Friess on 17/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#ifndef icmp_h
#define icmp_h

#include "ip.h"

#include <stdio.h>
#include <stdint.h>

struct icmp_header {
    uint8_t type;       // Type of ICMP message
    uint8_t code;       // Code
    uint16_t checksum;  // Header checksum
    uint32_t data;      // Header data
};

// Parse and validate an ICMP header
int icmp_parse_header(uint8_t *buf, struct icmp_header *header);

void icmp_handle_packet(struct ip_packet_header *ip, uint8_t *buf, size_t len);

#endif /* icmp_h */
