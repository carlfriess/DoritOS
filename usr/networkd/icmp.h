//
//  icmp.h
//  DoritOS
//
//  Created by Carl Friess on 17/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#ifndef icmp_h
#define icmp_h

#include <stdio.h>
#include <stdint.h>


#define ICMP_MSG_TYPE_ECHO_REQ      8
#define ICMP_MSG_TYPE_ECHO_REPLY    0


struct icmp_header {
    uint8_t type;       // Type of ICMP message
    uint8_t code;       // Code
    uint16_t checksum;  // Header checksum
    uint32_t data;      // Header data
};

// Parse and validate an ICMP header
int icmp_parse_header(uint8_t *buf, size_t len, struct icmp_header *header);

// Encode an ICMP header
void icmp_encode_header(struct icmp_header *header, uint8_t *buf, size_t len);

void icmp_handle_packet(uint32_t src_ip, uint8_t *buf, size_t len);


#endif /* icmp_h */
