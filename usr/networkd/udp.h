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
#include <aos/waitset.h>

#include <ip.h>

#include <net/common.h>


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

enum udp_socket_state {
    UDP_SOCKET_STATE_CLOSED,
    UDP_SOCKET_STATE_OPEN
};

struct udp_socket {
    struct udp_socket_common pub;
    struct urpc_chan chan;
    enum udp_socket_state state;
};


// Register handler for waitset events
//  When waitset is NULL, only reregisters
void udp_register_event_queue(struct waitset *waitset);

// Cancel waitset events
void udp_cancel_event_queue(void);

// Initialize the UDP module
void udp_init(void);

// Parse and validate a UDP header
int udp_parse_header(struct ip_packet_header *ip, uint8_t *buf, size_t len,
                     struct udp_header *header);

// Handle an incoming UDP packet
void udp_handle_packet(struct ip_packet_header *ip, uint8_t *buf, size_t len);

#endif /* udp_h */
