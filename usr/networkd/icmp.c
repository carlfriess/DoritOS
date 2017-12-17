//
//  icmp.c
//  DoritOS
//
//  Created by Carl Friess on 17/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#include "icmp.h"

#include <aos/aos.h>

#include <assert.h>

#include <netutil/htons.h>
#include <netutil/checksum.h>

// Parse and validate an ICMP header
int icmp_parse_header(uint8_t *buf, struct icmp_header *header) {
    
    // Parse all fields
    header->type = buf[0];
    header->code = buf[1];
    header->checksum = lwip_ntohs(*(uint16_t *)(buf + 2));
    header->data = lwip_ntohl(*(uint32_t *)(buf + 4));
    
    // Compute and verify checksum
    *(uint16_t *)(buf + 2) = 0;
    if (header->checksum == inet_checksum((void *) buf, 8)) {
        return 1;
    }
    
    return 0;
    
}

// Encode an ICMP header
void icmp_encode_header(struct icmp_header *header, uint8_t *buf) {
    
    // Set data fields
    buf[0] = header->type;
    buf[1] = header->code;
    *(uint32_t *)(buf + 4) = lwip_htonl(header->code);
    
    // Compute and set checksum
    *(uint16_t *)(buf + 2) = 0;
    header->checksum = inet_checksum((void *) buf, 8);
    *(uint16_t *)(buf + 2) = header->checksum;
    
}

void icmp_handle_packet(uint32_t src_ip, uint8_t *buf, size_t len) {
    
    // Sanity check: minimum packet size
    assert(len >= 8);
    
    // Parse and check the ICMP header
    struct icmp_header header;
    int e;
    if ((e = icmp_parse_header(buf, &header))) {
        debug_printf("INVALID ICMP MESSAGE: %d\n", e);
        return;
    }
    
    debug_printf("VALID ICMP MESSAGE\n");
    
    switch (header.type) {
        case ICMP_MSG_TYPE_ECHO_REQ:
            assert(header.code == 0);
            debug_printf("Received echo request with %x\n", header.data);
            break;
            
        default:
            debug_printf("Unknown ICMP message type (%d)!\n",
                         (int) header.type);
            break;
    }
    
}
